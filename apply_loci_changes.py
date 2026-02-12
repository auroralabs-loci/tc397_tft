import os, re, json, subprocess, pathlib, sys

REPO = pathlib.Path(".").resolve()

def run(cmd):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, shell=False)
    if p.returncode != 0:
        raise RuntimeError(f"Command failed: {cmd}\n{p.stderr}")
    return p.stdout

def find_c_files():
    # Prefer FreeRTOS-like layout if present; otherwise any *.c
    preferred = [
        "FreeRTOS/Source/tasks.c",
        "FreeRTOS/Source/queue.c",
        "FreeRTOS/Source/timers.c",
        "FreeRTOS/Source/event_groups.c",
        "FreeRTOS/Source/stream_buffer.c",
    ]
    files = []
    for p in preferred:
        if (REPO / p).exists():
            files.append(REPO / p)
    if len(files) >= 3:
        return files[:3]

    # fallback: first 3 C files excluding build/output dirs
    skip = {"build", ".git", ".vs", ".vscode", "out", "dist", "node_modules"}
    for path in REPO.rglob("*.c"):
        parts = set(path.parts)
        if parts & skip:
            continue
        files.append(path)
        if len(files) >= 3:
            break
    return files

FUNC_DEF = re.compile(r'^\s*(?:static\s+)?(?:inline\s+)?(?:\w[\w\s\*\(\)]*?)\s+([A-Za-z_]\w*)\s*\([^;]*\)\s*\{', re.M)

def insert_helper_and_call(path: pathlib.Path, idx: int):
    txt = path.read_text(encoding="utf-8", errors="ignore")

    helper_name = f"loci_qa_new_helper_{idx}"
    helper_code = (
        f"\n/* LOCI_QA_LAB: new helper */\n"
        f"static int {helper_name}(int x) {{\n"
        f"    return x + {10+idx};\n"
        f"}}\n"
    )

    # Insert helper after last include if possible
    include_iter = list(re.finditer(r'^\s*#\s*include[^\n]*\n', txt, flags=re.M))
    if include_iter:
        insert_at = include_iter[-1].end()
        txt2 = txt[:insert_at] + helper_code + txt[insert_at:]
    else:
        txt2 = helper_code + txt

    # Pick a function to modify: first non-static OR first function definition
    defs = list(FUNC_DEF.finditer(txt2))
    if not defs:
        return txt2, {"modified_function": None, "helper": helper_name}

    target = None
    for d in defs:
        name = d.group(1)
        if not name.startswith("loci_qa_"):
            target = d
            break
    if target is None:
        target = defs[0]

    fn_name = target.group(1)
    body_start = target.end()

    # Insert a call at the very beginning of function body
    injection = (
        f"\n    /* LOCI_QA_LAB: call new helper */\n"
        f"    volatile int loci_qa_sink_{idx} = {helper_name}({idx});\n"
        f"    (void)loci_qa_sink_{idx};\n"
    )

    txt3 = txt2[:body_start] + injection + txt2[body_start:]
    return txt3, {"modified_function": fn_name, "helper": helper_name}

def find_unused_static_function_to_delete(paths):
    # find a truly-unused static function: name appears only in its definition within that file
    static_def = re.compile(r'^\s*static\s+(?:\w[\w\s\*]*?)\s+([A-Za-z_]\w*)\s*\([^;]*\)\s*\{', re.M)
    for path in paths:
        txt = path.read_text(encoding="utf-8", errors="ignore")
        for m in static_def.finditer(txt):
            name = m.group(1)
            # Skip anything that looks important
            if name.startswith("vTask") or name.startswith("xTask") or name.startswith("vPort") or name.startswith("xPort"):
                continue
            # Count occurrences of the identifier in file
            occ = len(re.findall(rf'\b{name}\b', txt))
            if occ == 1:
                # delete the entire function block (best-effort brace matching)
                start = m.start()
                i = m.end()
                depth = 1
                while i < len(txt) and depth > 0:
                    if txt[i] == "{":
                        depth += 1
                    elif txt[i] == "}":
                        depth -= 1
                    i += 1
                if depth == 0:
                    end = i
                    return path, name, start, end
    return None, None, None, None

def compute_manifest(base_sha, head_sha):
    diff = run(["git","diff","-U0",f"{base_sha}..{head_sha}"])
    manifest = {"base": base_sha, "head": head_sha, "files": []}

    current_file = None
    for line in diff.splitlines():
        if line.startswith("diff --git "):
            current_file = None
        if line.startswith("+++ b/"):
            current_file = line[len("+++ b/"):].strip()
            if current_file == "/dev/null":
                current_file = None
        if line.startswith("@@") and current_file:
            m = re.match(r'^@@\s*-(\d+)(?:,(\d+))?\s+\+(\d+)(?:,(\d+))?\s*@@', line)
            if not m:
                continue
            old_start = int(m.group(1)); old_len = int(m.group(2) or "1")
            new_start = int(m.group(3)); new_len = int(m.group(4) or "1")
            manifest["files"].append({
                "file": current_file,
                "removed_range": [old_start, old_start + max(old_len-1,0)],
                "added_range": [new_start, new_start + max(new_len-1,0)]
            })
    return manifest

def main():
    base = run(["git","rev-parse","HEAD"]).strip()

    files = find_c_files()
    if len(files) < 1:
        raise RuntimeError("No C files found to modify.")

    changes = {"touched_files": [], "intent": {"modified": [], "new_helpers": [], "deleted": []}}

    # Apply modifications + add helpers
    for idx, path in enumerate(files, start=1):
        new_txt, info = insert_helper_and_call(path, idx)
        path.write_text(new_txt, encoding="utf-8")
        changes["touched_files"].append(str(path.relative_to(REPO)))
        changes["intent"]["new_helpers"].append(info["helper"])
        if info["modified_function"]:
            changes["intent"]["modified"].append({"file": str(path.relative_to(REPO)), "function": info["modified_function"]})

    # Try safe deletion of unused static function
    del_path, del_name, s, e = find_unused_static_function_to_delete(files)
    if del_path and del_name:
        txt = del_path.read_text(encoding="utf-8", errors="ignore")
        del_path.write_text(txt[:s] + f"\n/* LOCI_QA_LAB: deleted unused static function {del_name} */\n" + txt[e:], encoding="utf-8")
        changes["intent"]["deleted"].append({"file": str(del_path.relative_to(REPO)), "function": del_name})
    else:
        changes["intent"]["deleted"].append({"skipped": True, "reason": "No safely-unused static function found in selected files."})

    # Stage changes so git diff later reflects them
    run(["git","add","-A"])

    # Save planned changes summary now
    (REPO / "loci_planned_changes.json").write_text(json.dumps(changes, indent=2), encoding="utf-8")

    print("Applied code edits.")
    print(json.dumps(changes, indent=2))

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import re
import sys

def extract_function_calls(filename, function_name):
    """Extract all function calls made by a specific function."""
    calls = []
    in_function = False

    with open(filename, 'r') as f:
        for line in f:
            # Check if this is the start of our target function
            if re.match(rf'^<{re.escape(function_name)}>:\s*$', line):
                in_function = True
                continue

            # If we're in the function, collect call instructions
            if in_function:
                # Stop if we hit a new function or section
                if re.match(r'^<[^>]+>:\s*$', line) or line.startswith('Disassembly of section'):
                    break
                # Stop on empty line
                if line.strip() == '':
                    break

                # Look for call instructions
                call_match = re.search(r'call\s+<([^>]+)>', line)
                if call_match:
                    called_func = call_match.group(1)
                    # Remove address offsets like "function+0x14"
                    called_func = re.sub(r'\+0x[0-9a-f]+', '', called_func)
                    calls.append(called_func)

    return calls

def build_call_tree(filename, root_function, max_depth=10):
    """Build a complete call tree starting from root_function."""
    visited = set()
    call_tree = {}

    def traverse(func, depth=0):
        if depth >= max_depth or func in visited:
            return
        visited.add(func)

        calls = extract_function_calls(filename, func)
        call_tree[func] = calls

        for called_func in calls:
            traverse(called_func, depth + 1)

    traverse(root_function)
    return call_tree

def print_call_tree(call_tree, root, indent=0):
    """Print the call tree in a readable format."""
    if root in call_tree:
        for called in call_tree[root]:
            print("  " * indent + f"├─ {called}")
            print_call_tree(call_tree, called, indent + 1)

def get_all_functions(call_tree, root):
    """Get all unique functions in the call tree."""
    all_funcs = set()

    def collect(func):
        if func in all_funcs:
            return
        all_funcs.add(func)
        if func in call_tree:
            for called in call_tree[func]:
                collect(called)

    collect(root)
    return sorted(all_funcs)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: trace_calls.py <assembly_file> <root_function>")
        sys.exit(1)

    asm_file = sys.argv[1]
    root_func = sys.argv[2]

    # Build call tree
    call_tree = build_call_tree(asm_file, root_func)

    # Print tree
    print(f"\nCall tree for {root_func}:")
    print(root_func)
    print_call_tree(call_tree, root_func)

    # Print all unique functions
    all_funcs = get_all_functions(call_tree, root_func)
    print(f"\n\nAll unique functions ({len(all_funcs)}):")
    for func in all_funcs:
        print(f"  - {func}")

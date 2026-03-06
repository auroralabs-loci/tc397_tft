#!/usr/bin/env python3
import re
import sys

def extract_function(filename, function_name):
    """Extract a specific function's assembly code from the dump file."""
    function_code = []
    in_function = False

    with open(filename, 'r') as f:
        for line in f:
            # Check if this is the start of our target function
            if re.match(rf'^<{re.escape(function_name)}>:\s*$', line):
                in_function = True
                function_code.append(line.rstrip())
                continue

            # If we're in the function, collect lines
            if in_function:
                # Stop if we hit a new section or another function
                if line.startswith('Disassembly of section') or \
                   (re.match(r'^<[^>]+>:\s*$', line) and not line.startswith(f'<{function_name}')):
                    break
                # Stop on empty line (end of function)
                if line.strip() == '':
                    break
                function_code.append(line.rstrip())

    return '\n'.join(function_code)

if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: extract_functions.py <assembly_file> <function_name>")
        sys.exit(1)

    asm_file = sys.argv[1]
    func_name = sys.argv[2]

    code = extract_function(asm_file, func_name)
    if code:
        print(code)
    else:
        print(f"Function '{func_name}' not found", file=sys.stderr)
        sys.exit(1)

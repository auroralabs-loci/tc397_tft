#!/usr/bin/env python3
import re
import sys

def parse_assembly_file(filename):
    """Parse assembly file and extract functions with their assembly code."""
    functions = {}
    current_function = None
    current_code = []

    with open(filename, 'r') as f:
        for line in f:
            # Check if this is a function header (e.g., <functionName>:)
            func_match = re.match(r'^<([^>]+)>:\s*$', line)
            if func_match:
                # Save previous function if exists
                if current_function is not None:
                    functions[current_function] = '\n'.join(current_code)

                # Start new function
                current_function = func_match.group(1)
                current_code = [line.rstrip()]
            elif current_function is not None:
                # Check if we've reached the next section or another function
                if line.startswith('Disassembly of section'):
                    # Save current function and reset
                    functions[current_function] = '\n'.join(current_code)
                    current_function = None
                    current_code = []
                elif line.strip() == '':
                    # Empty line might indicate end of function
                    if current_code:
                        functions[current_function] = '\n'.join(current_code)
                        current_function = None
                        current_code = []
                else:
                    # Add line to current function
                    current_code.append(line.rstrip())

        # Save last function if exists
        if current_function is not None:
            functions[current_function] = '\n'.join(current_code)

    return functions

def main():
    if len(sys.argv) < 2:
        print("Usage: parse_assembly.py <assembly_file>")
        sys.exit(1)

    functions = parse_assembly_file(sys.argv[1])

    # Print function names and their assembly code
    for func_name, code in functions.items():
        print(f"\n{'='*60}")
        print(f"Function: {func_name}")
        print(f"{'='*60}")
        print(code)

if __name__ == '__main__':
    main()

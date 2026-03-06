#!/usr/bin/env python3
import re
import sys

def parse_assembly_file(filename):
    """Parse assembly file and extract functions with their assembly code."""
    functions = {}
    current_function = None
    current_asm = []

    with open(filename, 'r') as f:
        for line in f:
            # Match function definition lines like: "<functionName>:"
            func_match = re.match(r'^<([^>]+)>:\s*$', line.strip())
            if func_match:
                # Save previous function if exists
                if current_function:
                    functions[current_function] = '\n'.join(current_asm)

                # Start new function
                current_function = func_match.group(1)
                current_asm = []
            elif current_function:
                # Add instruction line if it's not empty and not a section marker
                stripped = line.strip()
                if stripped and not stripped.startswith('Disassembly'):
                    current_asm.append(stripped)

        # Save last function
        if current_function:
            functions[current_function] = '\n'.join(current_asm)

    return functions

def main():
    assembly_file = '/home/melisa/tc397_tft/assembly_output.txt'
    target_functions = ['simulateCpuWorkload', 'blinkLED', 'largeFunction']

    functions = parse_assembly_file(assembly_file)

    print(f"Total functions found: {len(functions)}")
    print(f"Target functions: {target_functions}")
    print()

    for func_name in target_functions:
        if func_name in functions:
            print(f"=== {func_name} ===")
            print(f"Lines: {len(functions[func_name].splitlines())}")
            print()
        else:
            print(f"WARNING: {func_name} not found in assembly!")
            print()

    # Write each function to a separate file for inspection
    for func_name in target_functions:
        if func_name in functions:
            with open(f'/home/melisa/tc397_tft/asm_{func_name}.txt', 'w') as f:
                f.write(functions[func_name])

if __name__ == '__main__':
    main()

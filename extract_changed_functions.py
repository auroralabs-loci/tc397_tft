#!/usr/bin/env python3
import re

def parse_assembly_file(filename):
    """Parse assembly file and extract all functions."""
    functions = {}
    current_function = None
    current_assembly = []

    with open(filename, 'r') as f:
        for line in f:
            # Check for function start (e.g., "<functionName>:")
            func_match = re.match(r'^<(.+?)>:', line)
            if func_match:
                # Save previous function if exists
                if current_function and current_assembly:
                    functions[current_function] = '\n'.join(current_assembly)

                current_function = func_match.group(1)
                current_assembly = []
            elif current_function:
                # Skip empty lines and section headers
                stripped = line.strip()
                if stripped and not stripped.startswith('Disassembly') and not stripped.startswith('build/'):
                    # Add the instruction line
                    if stripped and not re.match(r'^<.+?>:', stripped):
                        current_assembly.append(stripped)

        # Save last function
        if current_function and current_assembly:
            functions[current_function] = '\n'.join(current_assembly)

    return functions

# Changed functions based on git diff
changed_functions = ['blinkLED', 'simulateCpuWorkload', 'largeFunction']

# Parse assembly
functions = parse_assembly_file('assembly_dump_current.txt')

# Find and save changed functions
found_functions = {}
for func_name in changed_functions:
    if func_name in functions:
        found_functions[func_name] = functions[func_name]
        print(f"Found function: {func_name}")
    else:
        print(f"Function not found: {func_name}")

# Print summary
print(f"\nTotal functions in assembly: {len(functions)}")
print(f"Changed functions found: {len(found_functions)}")

# Save to separate files for debugging
for func_name, assembly in found_functions.items():
    with open(f'function_{func_name}.asm', 'w') as f:
        f.write(assembly)
    print(f"Saved {func_name} to function_{func_name}.asm")

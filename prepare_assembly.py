#!/usr/bin/env python3

# Read the largeFunction assembly
with open('function_largeFunction.asm', 'r') as f:
    lines = f.readlines()

# Extract just the assembly instructions (remove line numbers and arrow)
assembly_lines = []
for line in lines:
    # Split by the arrow and take the second part
    parts = line.split('→', 1)
    if len(parts) == 2:
        assembly_lines.append(parts[1].rstrip())

# Join into single string
assembly_code = '\n'.join(assembly_lines)

# Save to file
with open('largeFunction_assembly.txt', 'w') as f:
    f.write(assembly_code)

print(f"Extracted {len(assembly_lines)} lines of assembly")
print(f"Total characters: {len(assembly_code)}")

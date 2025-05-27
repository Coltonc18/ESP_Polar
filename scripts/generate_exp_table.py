import numpy as np
import math
import os
import sys

# Initialize constants
num_samples = 0
FREQ_BINS = 0
FREQ_START = 0
FREQ_END = 0

# Constants from MEM.h
try:
    with open("src/utils/BoundedQueue.hpp", "r", encoding='utf-8') as file:
        for line in file:
            if "NUM_SAMPLES" in line and "#define" in line:
                num_samples = int(line.split()[2])

    with open("src/core/Parameters.h", "r", encoding='utf-8') as params:
        for line in params:
            if "FREQ_BINS" in line and "#define" in line:
                FREQ_BINS = int(line.split()[2])
            elif "FREQ_START" in line and "#define" in line:
                FREQ_START = float(line.split()[2])
            elif "FREQ_END" in line and "#define" in line:
                FREQ_END = float(line.split()[2])

    # Validate constants
    if num_samples == 0 or FREQ_BINS == 0 or FREQ_START == 0 or FREQ_END == 0:
        raise ValueError("Failed to read all required constants")

    # Calculate MODEL_ORDER based on NUM_SAMPLES
    MODEL_ORDER = int(num_samples / math.log(2 * num_samples))

except Exception as e:
    print(f"Error reading constants: {e}")
    sys.exit(1)

def generate_exp_table():
    # Generate frequency points
    freqs = np.linspace(FREQ_START, FREQ_END, FREQ_BINS)
    
    output_lines = []
    output_lines.append("// Generated with command: python scripts/generate_exp_table.py > src/utils/exp_table.h")
    output_lines.append("// Re-run command to update.")
    output_lines.append("")
    
    # Generate the header guards
    output_lines.append("#ifndef EXP_TABLE_H")
    output_lines.append("#define EXP_TABLE_H")
    
    # Include the Parameters.h file
    output_lines.append("\n#include \"Constants.h\"\n")
    
    # Generate the table
    output_lines.append("static const struct {")
    output_lines.append("  float real[FREQ_BINS];")
    output_lines.append("  float imag[FREQ_BINS];")
    output_lines.append("} exp_table[MODEL_ORDER] __attribute__((section(\".rodata\"))) = {")
    
    for i in range(MODEL_ORDER):
        output_lines.append(f"  {{  // i = {i}")
        output_lines.append("    {  // real parts")
        real_parts = []
        for f in freqs:
            real = math.cos(-2 * math.pi * f * i)
            real_parts.append(f"{real:.6f}f")
        output_lines.append("      " + ", ".join(real_parts) + ",")
        output_lines.append("    },")
        
        output_lines.append("    {  // imaginary parts")
        imag_parts = []
        for f in freqs:
            imag = math.sin(-2 * math.pi * f * i)
            imag_parts.append(f"{imag:.6f}f")
        output_lines.append("      " + ", ".join(imag_parts) + ",")
        output_lines.append("    }")
        output_lines.append("  }," if i < MODEL_ORDER - 1 else "  }")
    
    output_lines.append("};")
    output_lines.append("#endif  // EXP_TABLE_H")
    
    # Write to file with UTF-8 encoding
    output_path = "src/utils/exp_table.h"
    try:
        # First check if file exists and is read-only
        if os.path.exists(output_path):
            if not os.access(output_path, os.W_OK):
                print(f"Error: File {output_path} is read-only. Please make it writable and try again.")
                sys.exit(1)
        
        # Try to write the file
        with open(output_path, "w", encoding='utf-8') as f:
            f.write("\n".join(output_lines))
        print(f"Successfully generated {output_path}")
        
    except PermissionError:
        print(f"Error: Permission denied when writing to {output_path}")
        print("Please make sure the file is not open in another program and you have write permissions.")
        sys.exit(1)
    except Exception as e:
        print(f"Error writing to file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    generate_exp_table()

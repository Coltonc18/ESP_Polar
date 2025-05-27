import numpy as np
import math

# Constants from MEM.h
with open("src/utils/BoundedQueue.hpp", "r", encoding='utf-8') as f:
    for line in f:
        if "NUM_SAMPLES" in line and "#define" in line:
            num_samples = int(line.split()[2])

with open("src/core/MEM.h", "r", encoding='utf-8') as f:
    for line in f:
        if "FREQ_BINS" in line and "#define" in line:
            FREQ_BINS = int(line.split()[2])
        elif "FREQ_START" in line and "#define" in line:
            FREQ_START = float(line.split()[2])
        elif "FREQ_END" in line and "#define" in line:
            FREQ_END = float(line.split()[2])

# Calculate MODEL_ORDER based on NUM_SAMPLES
MODEL_ORDER = int(num_samples / math.log(2 * num_samples))

def generate_exp_table():
    # Generate frequency points
    freqs = np.linspace(FREQ_START, FREQ_END, FREQ_BINS)
    
    print("// Generated with command: python scripts/generate_exp_table.py > src/utils/exp_table.h")
    print("// Re-run command to update.")
    
    # Generate the header guards
    print("#ifndef EXP_TABLE_H")
    print("#define EXP_TABLE_H")
    
    # Generate the table
    print("static const struct {")
    print("  float real[FREQ_BINS];")
    print("  float imag[FREQ_BINS];")
    print("} exp_table[MODEL_ORDER] PROGMEM = {")
    
    for i in range(MODEL_ORDER):
        print("  {  // i =", i)
        print("    {  // real parts")
        real_parts = []
        for f in freqs:
            real = math.cos(-2 * math.pi * f * i)
            real_parts.append(f"{real:.6f}f")
        print("      " + ", ".join(real_parts) + ",")
        print("    },")
        
        print("    {  // imaginary parts")
        imag_parts = []
        for f in freqs:
            imag = math.sin(-2 * math.pi * f * i)
            imag_parts.append(f"{imag:.6f}f")
        print("      " + ", ".join(imag_parts) + ",")
        print("    }")
        print("  }," if i < MODEL_ORDER - 1 else "  }")
    
    print("};")
    print("#endif  // EXP_TABLE_H")

if __name__ == "__main__":
    generate_exp_table()

#!/usr/bin/env python3
import sys

if len(sys.argv) != 3:
    print("Usage: make_banner.py <input.txt> <output.h>")
    sys.exit(1)

inp = sys.argv[1]
out = sys.argv[2]

with open(inp, "r") as f_in, open(out, "w") as f_out:
    # Write the C declaration header
    f_out.write("static const char banner[] =\n")

    for line in f_in:
        # Strip newline, escape backslashes, wrap in quotes, add \n
        escaped = line.rstrip("\n").replace("\\", "\\\\")
        f_out.write(f"\"{escaped}\\n\"\n")

    # Close the declaration
    f_out.write(";\n")

print(f"Generated {out}")

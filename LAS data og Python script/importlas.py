import laspy

# All input files
input_files = [
    'las/lasdata1.laz',
    'las/lasdata2.laz',
    'las/lasdata3.laz',
    'las/lasdata4.laz'
]

# Output file
output_file = 'las/lasdata_final.txt'

# Open output file once
with open(output_file, 'w') as f:
    # f.write("X Y Z R G B U V\n")  # Optional header

    # Loop through each LAZ file
    for input_file in input_files:
        print(f"Leser {input_file}...")

        # Read LAZ file
        las = laspy.read(input_file, laz_backend=laspy.LazBackend.Laszip)

        # Set color for all points (red)
        r = [65535] * len(las.x)
        g = [0] * len(las.x)
        b = [0] * len(las.x)

        # Default U and V
        u = [0] * len(las.x)
        v = [0] * len(las.x)

        # Write each point to ASCII
        for x, y, z, rr, gg, bb, uu, vv in zip(las.x, las.y, las.z, r, g, b, u, v):
            f.write(f"{x} {y} {z} {rr} {gg} {bb} {uu} {vv}\n")

print(f"Alle filer eksportert til: {output_file}")

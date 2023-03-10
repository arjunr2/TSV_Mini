import sys
import struct

# Read binary data from standard input into a list of integers
int_list = []
while True:
    int_bytes = sys.stdin.buffer.read(4)
    if not int_bytes:
        break
    int_val = struct.unpack('<i', int_bytes)[0]
    int_list.append(int_val)

# Sort and remove duplicates from the list
sorted_ints = sorted(set(int_list))

# Write sorted and unique integers to new binary file
with open(sys.argv[1], 'wb') as outfile:
    for int_val in sorted_ints:
        outfile.write(struct.pack('<i', int_val))


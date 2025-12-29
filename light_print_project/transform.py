from PIL import Image
import struct

in_path = input("picture path: ")
out_path = input("output path: ")

f = Image.open(in_path).convert("1")

with open(out_path, "wb") as op :
    op.write(struct.pack("<h", f.width))
    op.write(struct.pack("<h", f.height))

    for i in range(f.height) :
        for j in range(f.width) :
            op.write(bytes([1 if 255 == f.getpixel((j, i)) else 0]))

f.close()
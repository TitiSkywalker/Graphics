from PIL import Image

oldFile = input("enter your input file name(with extension): ")
newFile = input("enter your output file name(with extension .bmp): ")

old_image = Image.open(oldFile)

# Convert to BMP with 24-bit depth
new_image = old_image.convert('RGB')
new_image.save(newFile)

print("done")
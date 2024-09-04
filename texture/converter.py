from PIL import Image

filename1 = input("enter your input file name(with extension): ")
filename2 = output("enter your output file name(with extension): ")

old_image = Image.open(filename1)

# Convert to BMP with 24-bit depth
bmp_image = old_image.convert('RGB')
bmp_image.save(filename2)

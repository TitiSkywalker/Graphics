#pragma once
#include "BitmapImage.hpp"
#include "Vector3f.h"

///@brief helper class that stores a texture and faciliates lookup
///assume 4byte RGBA image data
class Texture 
{
public:
	Texture() :bimg(0), width(0), height(0)
	{}
	~Texture()
	{
		if (bimg)
		{
			delete bimg;
		}
	}

	bool valid()
	{
		return bimg != 0;
	}

	void load(const char* filename)
	{
		//load texture image
		bimg = new bitmap_image(filename);
		height = bimg->height();
		width = bimg->width();
	}

	void operator()(int x, int y, unsigned char* color)
	{
		//get color at given pixel, store it in "color"
		x = (x < 0) ? 0 : x;
		x = (x > width - 1) ? (width - 1) : x;
		y = (y < 0) ? 0 : y;
		y = (y > height - 1) ? (height - 1) : y;
		bimg->get_pixel(x, y, color[0], color[1], color[2]);
	}

	///@param x assumed to be between 0 and 1
	Vector3f operator()(float x, float y)
	{
		//get RGB color at given pixel
		Vector3f color;
		int ix, iy;
		x = x * width;
		y = (1 - y) * height;
		ix = (int)x;
		iy = (int)y;
		unsigned char pixels[4][3];
		float alpha = x - ix;
		float beta = y - iy;
		operator()(ix, iy, pixels[0]);
		operator()(ix + 1, iy, pixels[1]);
		operator()(ix, iy + 1, pixels[2]);
		operator()(ix + 1, iy + 1, pixels[3]);
		for (int ii = 0; ii < 3; ii++) {
			color[ii] = (1 - alpha) * (1 - beta) * pixels[0][ii]
				+ alpha * (1 - beta) * pixels[1][ii]
				+ (1 - alpha) * beta * pixels[2][ii]
				+ alpha * beta * pixels[3][ii];
		}
		return color / 255;
	}


	bitmap_image* bimg;
	int width, height;
};

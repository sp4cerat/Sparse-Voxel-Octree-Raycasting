///////////////////////////////////////////
#pragma once
///////////////////////////////////////////
#include "core.h"
#include "mathlib/Vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
///////////////////////////////////////////
class Bmp
{
public:

	Bmp();
	Bmp(int x,int y,int bpp,unsigned char*data);
	Bmp(const char*filename);
	~Bmp();

	void load(const char*filename);
	void save(const char*filename);
	void set(int x,int y,int bpp,unsigned char*data);
	void crop(int x,int y);
	void scale(int x,int y);
	void blur(int count);
	void hblur(int count);
	void vblur(int count);
	void set_pixel(int x,int y,int r,int g,int b)
	{
		data[(x+y*width)*(bpp/8)+2]=r;
		data[(x+y*width)*(bpp/8)+1]=g;
		data[(x+y*width)*(bpp/8)+0]=b;
	}	
	int get_pixel(int x,int y)
	{
		return
			data[(x+y*width)*(bpp/8)+2]+
			data[(x+y*width)*(bpp/8)+1]*256+
			data[(x+y*width)*(bpp/8)+0]*256*256;
	}
	vec3f get_pixel3f(int x,int y)
	{
		int color=get_pixel(x,y);
		float r=float(color&255)/255.0f;
		float g=float((color>>8)&255)/255.0f;
		float b=float((color>>16)&255)/255.0f;
		return vec3f(r,g,b);
	}

public:

	unsigned char*data;
	int width;
	int height;
	int bpp;

private:

	unsigned char bmp[54];
};


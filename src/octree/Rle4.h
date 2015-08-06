#pragma once
#include "core.h"
extern void set_voxel(uint x,uint y,uint z,uchar4 color);

struct RLE4
{	
	struct Map4
	{
		int    sx,sy,sz,slabs_size;
		uint   *map;
		ushort *slabs;
	};
	/*------------------------------------------------------*/
	Map4 map [16];int nummaps;	
	/*------------------------------------------------------*/
	void load(char *filename,int palette, int addx,int addy,int addz);
	/*------------------------------------------------------*/
};

#pragma once
#define _USE_MATH_DEFINES
#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopj(start_l,end_l) for ( int j=start_l;j<end_l;++j )
#define loopk(start_l,end_l) for ( int k=start_l;k<end_l;++k )
#define loopl(start_l,end_l) for ( int l=start_l;l<end_l;++l )
#define loopm(start_l,end_l) for ( int m=start_l;m<end_l;++m )
#define loop(a_l,start_l,end_l) for ( int a_l = start_l;a_l<end_l;++a_l )
/*------------------------------------------------------*/
#define f_min min
#define f_max max
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
/*------------------------------------------------------*/

//#ifndef byte
//#define byte unsigned char
//#endif

#ifndef ushort
#define ushort unsigned short
#endif

#ifndef uint
#define uint unsigned int
#endif

#ifndef uchar
#define uchar unsigned char
#endif

struct intfloat
{
	union {int i;float f; unsigned int ui;};
};

class uchar4 
{ 
	public:

	uchar x,y,z,w;

	uchar4(){};
	uchar4(uint x,uint y,uint z,uint w)
	{
		this->x=x;
		this->y=y;
		this->z=z;
		this->w=w;
	}
	
	uint to_uint()
	{
		return *((uint*)this);
	};
};

/*------------------------------------------------------*/
#include <windows.h>
#include "stdio.h"
#include <vector>
/*------------------------------------------------------*/
#include "Rle4.h"
/*------------------------------------------------------*/
void RLE4::load(char *filename,int palette, int addx,int addy,int addz)
{
	FILE* fn; 

	if ((fn = fopen (filename,"rb")) == NULL)
	{
		MessageBox(0,filename,"File not found - creating started",0);
		exit(0);
	}

	fread(&nummaps,1,4,fn);

	printf("Loading %s with %d mipmaps\n",filename,nummaps);

	int total_numrle=0;
	int total_numtex=0;
	int numtex_0=0;

	for (int m=0;m<nummaps;m++)
	{
		printf("MipVol %d ------------------------\n",m);

		fread(&map[m].sx,1,4,fn);
		fread(&map[m].sy,1,4,fn);
		fread(&map[m].sz,1,4,fn);
		fread(&map[m].slabs_size,1,4,fn);

		printf("sx %d\n",map[m].sx);
		printf("sy %d\n",map[m].sy);
		printf("sz %d\n",map[m].sz);
		printf("slabs_size %d\n",map[m].slabs_size);

		map[m].map = (uint*) malloc ( map[m].sx*map[m].sz*4*2 );
		map[m].slabs = (ushort*)malloc ( map[m].slabs_size*2 );

		fread(map[m].slabs,1,map[m].slabs_size*2,fn);

		int x=0,z=0;

		memset(map[m].map,0,map[m].sx*map[m].sz*4*2);
		map[m].map[0]=0;

		int ofs=0;

		int numrle = 0, numtex=0;

		while(1)
		{
			map[m].map[(x+z*map[m].sx)*2]=ofs;

			uint count    = map[m].slabs[ofs]; 
			uint firstrle = 0;
			if(ofs+2<map[m].slabs_size) firstrle = map[m].slabs[ofs+2]; 

			map[m].map[(x+z*map[m].sx)*2+1]= count + (firstrle<<16);

			x=(x+1)%map[m].sx;
			if(x==0)
			{
				z=(z+1)%map[m].sz;
				if(z==0)break;
			}
			numrle += map[m].slabs[ofs]+2;
			numtex += map[m].slabs[ofs+1];
			ofs+=map[m].slabs[ofs]+map[m].slabs[ofs+1]+2;
		}

		total_numrle+=numrle;
		total_numtex+=numtex;

		if (m==0) numtex_0 = numtex;

		printf("Number of RLE Elements:%d\n",numrle);
		printf("Number of Voxels:%d\n",numtex);
		printf("Bits per Voxel:%2.2f (RLE)+ 16 (Color)\n",float(numrle*16)/float(numtex));
	}

	printf("Total Number of RLE Elements:%d\n",total_numrle);
	printf("Total Number of Voxels:%d\n",total_numtex);
	printf("Total Bits per Voxel:%2.2f (RLE)+ %2.2f (Color)\n",float(total_numrle*16)/float(numtex_0),float(total_numtex*16)/float(numtex_0));

	fclose(fn);

	loopi(0,1)//nummaps)
	{
		Map4 map4=map[i];
		
		for (int slice=0;slice<map4.sz;slice++)
		for (int x=0;x<map4.sx;x++)
		{
			if (x==0)printf("Adding Voxels:%d     \r",slice);

			uint ofs						= map4.map[(x+(map4.sx*slice))*2+0];
			uint count_firstRLE	= map4.map[(x+(map4.sx*slice))*2+1];
			ushort count    = count_firstRLE & 65535;
			ushort firstRLE = count_firstRLE >> 16;

			ushort *p = map4.slabs + ofs + 2;
			ushort *pt= p+count;//+2;

			uint y1=0,y2=0;

			float color_tab[4][3]={
				{0.8,1.0,0.3},
				{1.2,0.7,0.3},
				{1.5,0.8,0.1},
				{0.2,0.8,0.2}};

			for (int s=0;s<count;s++,p++)
			{
				ushort slab = *p;
			
				y1+=slab&1023;
				y2=y1+(slab>>10);
			
				for (;y1<y2;y1++,pt++) if (y1<map4.sy)
				{
					ushort tcol=*pt;

					int color = ( tcol >> 8 ) & 3;
					float intensity = 8*(tcol & 31);//
					intensity = ((tcol & 255)*(tcol & 255))/255+0;
					//intensity=(intensity-64)*1.5;
					if(intensity<2)intensity=2;
					if(intensity>255)intensity=255;

					uchar4 rgba;

					rgba.z = f_max(f_min( 255,  float (color_tab[color][0] * intensity )),0);//( tcol     &31)*8;
					rgba.y = f_max(f_min( 255,  float (color_tab[color][1] * intensity )),0) ;;//((tcol>>5) &31)*8;
					rgba.x = f_max(f_min( 255,  float (color_tab[color][2] * intensity )),0) ;;//((tcol>>10)&31)*8;
					rgba.w = 0;
					
					rgba.x = tcol<<3;
					rgba.y =(tcol>>5)<<3;
					rgba.z =(tcol>>10)<<3;
					
					int yv=map4.sy-1-y1;
					rgba.x=color+((int(intensity)>>3)<<3);

					//buddha
					if(!palette)
					{
						rgba.x=1+((255-(tcol&255))&0xfc) ;//(int((tcol & 31)<<3)&0xfc);
						//rgba.x+=1; //+(((x^yv^slice)>>7)&3);//=      ((int(intensity)>>3)<<3);
					}
			//		if(y1<900) rgba.x+=1; else rgba.x+=(((x^yv^slice)>>7)&3);//2;
					
					//if(intensity<0)intensity=0;
					//if(intensity>255)intensity=255;
					//rgba.x=color+((intensity>>3)<<3);
					//if(y1<768)
					set_voxel(x+addx,yv+addy,slice+addz, rgba );
				}
			}
		}
	}
}
/*------------------------------------------------------*/

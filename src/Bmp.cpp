//#################################################################//
#include "Bmp.h"
#include "error.h"
//#################################################################//
Bmp::Bmp()
{
	width=height=0;
	data=NULL;
}
//#################################################################//
Bmp::Bmp(const char*filename)
{
	width=height=0;
	data=NULL;
	load(filename);
}
//#################################################################//
Bmp::Bmp(int x,int y,int b,unsigned char*buffer)
{
	width=height=0;
	data=NULL;
	set(x,y,b,buffer);
}
//#################################################################//
Bmp::~Bmp()
{
	if (data) free(data);
}
//#################################################################//
void Bmp::save(const char*filename)
{
	unsigned char bmp[58]=
			{0x42,0x4D,0x36,0x30,0,0,0,0,0,0,0x36,0,0,0,0x28,0,0,0,
	           	0x40,0,0,0, // X-Size
	           	0x40,0,0,0, // Y-Size
                   	1,0,0x18,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	bmp[18]	=width;
	bmp[19]	=width>>8;
	bmp[22]	=height;
	bmp[23]	=height>>8;
	bmp[28]	=bpp;

	FILE* fn;
	if ((fn = fopen (filename,"wb")) != NULL)
	{
		fwrite(bmp ,1,54   ,fn);
		fwrite(data,1,width*height*(bpp/8),fn);
		fclose(fn);
	}
	else error_stop("Bmp::save %s",filename);
	/*
ILuint imageID;
ilGenImages(1, &imageID);
ilBindImage(imageID);

ilTexImage(imageWidth, imageHeight, 1, bytesperpixel, IL_RGB, IL_UNSIGNED_BYTE, pixels); //the third agrument is usually always 1 (its depth. Its never 0 and I can't think of a time when its 2)

ilSaveImage("image.jpg");	
	*/
	
}
//#################################################################//
void  Bmp::blur(int count)
{
	int x,y,b,c;
	int bytes=bpp/8;
	for(c=0;c<count;c++)
		for(x=0;x<width-1;x++)
			for(y=0;y<height-1;y++)
				for(b=0;b<bytes;b++)
					data[(y*width+x)*bytes+b]=
					    (	(int)data[((y+0)*width+x+0)*bytes+b]+
					      (int)data[((y+0)*width+x+1)*bytes+b]+
					      (int)data[((y+1)*width+x+0)*bytes+b]+
					      (int)data[((y+1)*width+x+1)*bytes+b] ) /4;

}
//#################################################################//
void Bmp::crop(int x,int y)
{
	if(data==NULL)return;

	unsigned char* newdata;
	int i,j;

	int bytes=bpp/8;

	newdata=(unsigned char*)malloc(x*y*bytes);

	if(!newdata) error_stop("Bmp::crop : out of memory");

	memset(newdata,0,x*y*bytes);

	for(i=0;i<y;i++)
		if(i<height)
			for(j=0;j<x*bytes;j++)
				if(j<width*bytes)
					newdata[i*x*bytes+j]=data[i*width*bytes+j];
	free(data);
	data=NULL;
	set(x,y,bpp,newdata);
}
//#################################################################//
void Bmp::scale(int x,int y)
{
	if(data==NULL)return ;
	if(x==0)return ;
	if(y==0)return ;

	unsigned char* newdata;
	int i,j,k;

	int bytes=bpp/8;
	newdata=(unsigned char*)malloc(x*y*bytes);
	if(!newdata) error_stop("Bmp::scale : out of memory");

	memset(newdata,0,x*y*bytes);

	for(i=0;i<y;i++)
		for(j=0;j<x;j++)
			for(k=0;k<bytes;k++)
				newdata[i*x*bytes+j*bytes+k]=data[(i*height/y)*(width*bytes)+(j*width/x)*bytes+k];

	free(data);
	data=NULL;
	set(x,y,bpp,newdata);
}
//#################################################################//
void Bmp::set(int x,int y,int b,unsigned char*buffer)
{
	width=x;
	height=y;
	bpp=b;
	if(data) free(data);
	data=buffer;
	if(data==NULL)
	{
		data=(unsigned char*) malloc(width*height*(bpp/8));
		if(!data) error_stop("Bmp::set : out of memory");
		memset(data,0,width*height*(bpp/8));
	}

	bmp[18]	=width;
	bmp[19]	=width>>8;
	bmp[22]	=height;
	bmp[23]	=height>>8;
	bmp[28]	=bpp;
}
//#################################################################//
void Bmp::load(const char *filename)
{
	FILE* handle;

	if(filename==NULL)		
		{error_stop("File not found %s !\n",filename);}
	if((char)filename[0]==0)	
		{error_stop("File not found %s !\n",filename);}

	if ((handle = fopen(filename, "rb")) == NULL)
		{error_stop("File not found %s !\n",filename);}
		
	if(!fread(bmp, 11, 1, handle))
	{
		error_stop("Error reading file %s!\n",filename);
	}
	if(!fread(&bmp[11], (int)((unsigned char)bmp[10])-11, 1, handle))
	{
		error_stop("Error reading file %s!\n",filename);
	}

	width	=(int)((unsigned char)bmp[18])+((int)((unsigned char)(bmp[19]))<<8);
	height	=(int)((unsigned char)bmp[22])+((int)((unsigned char)(bmp[23]))<<8);
	bpp		=bmp[28];

	//printf("%s : %dx%dx%d Bit \n",filename,width,height,bpp);
	
	if(data)free(data);

	int size=width*height*(bpp/8);

	data=(unsigned char*)malloc(size+1);
	fread(data,size,1,handle);

	fclose(handle);
	printf("read successfully %s ; %dx%dx%d Bit \n",filename,width,height,bpp);

}

//#################################################################//

///////////////////////////////////////////
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
///////////////////////////////////////////
__kernel void memset(__global uint *dst, uint dstofs, uint val ) {  dst[get_global_id(0)+dstofs] = val; }
__kernel void memcpy(__global uint *dst, uint dstofs, __global uint *src, uint srcofs) {  dst[dstofs+get_global_id(0)] = src[srcofs+get_global_id(0)]; }	
///////////////////////////////////////////
#define loopi(start_l,end_l) for ( int i=start_l;i<end_l;++i )
#define loopj(start_l,end_l) for ( int j=start_l;j<end_l;++j )
#define loopk(start_l,end_l) for ( int k=start_l;k<end_l;++k )
///////////////////////////////////////////
#define OCTREE_DEPTH 11
#define OCTREE_DEPTH_AND ((1<<OCTREE_DEPTH)-1)
#define COARSE_SCALE 2
#define VIEW_DIST_MAX 400000
#define LOD_ADJUST_COARSE 2
#define LOD_ADJUST 1
#define SCENE_LOOP 0
#define SCALE_MAX (1 << (OCTREE_DEPTH + 1))
#define Z_CONTINUITY (0.00625*1.0)
#define HOLE_PIXEL_THRESHOLD 4
///////////////////////////////////////////
inline uint get_bitcount(uint b)
{
	b = (b & 0x55) + (b >> 1 & 0x55);
	b = (b & 0x33) + (b >> 2 & 0x33);
	b = (b + (b >> 4)) & 0x0f;
	return b;
}	

// Variable used by persistent threads
inline uint fetchChildOffset(
	__global uint *octree,
	uint const a_node, 
	uint const a_node_before, 
	uint* local_root, 
	uint a_child,
	uint child_test,
//	uint* bitlookup,
	uint rekursion)
{
	uint n=a_node>>9;
	uint nadd=a_child;

	if (a_node&256)
	{
		if (!(a_node_before&256))		//if (rekursion==6)
		{
			(*local_root)=n<<6; 
			n=n^n;
		}
		n+=(*local_root);
		nadd = get_bitcount(a_node & ((child_test<<1)-1));
		//bitlookup[a_node & ((child_test<<1)-1)] ; 

		if(rekursion==2)
		{
			return ((octree[n+(nadd>>2)]>>((nadd&3)<<3))&255)+256+(nadd<<9);
		}		
	}	
	return octree[n+nadd];//octree[(n+nadd)%10842491] ;  // ptr+
}

inline uint fetchColor(__global uint *octree,
				uint const a_node,
				uint const a_node_before, 
				uint const a_node_before2, 
				uint *local_root, 
				uint rekursion,
				uint child_test//, 
							//uint* bitlookup
							)
{
	uint n=a_node>>9;
	uint nadd=8;

	if(rekursion==1)  
	{
			n=(a_node_before>>9)&15;
			uint ofs=(a_node_before2>>9)+(*local_root);
			nadd=0+get_bitcount(a_node_before2&255);
			loopi(1,n)
			{
				nadd+=get_bitcount((octree[ofs+(i>>2)]>>((i&3)<<3))&255);
			}
			nadd += get_bitcount(a_node_before & ((child_test<<1)-1));

			return octree[ofs+(nadd>>2)]>>((nadd&3)<<3);
	}

	if (a_node&256)
	{
		if (!(a_node_before&256))		//if (rekursion==6)
		{
			(*local_root)=n<<6; 
			n=n^n;
		}
		nadd = (*local_root); 

		if(rekursion==2)
		{
			uint ofs=(a_node_before>>9)+(*local_root);
			nadd=1+get_bitcount(a_node_before&255);
			loopi(1,n)
			{
				nadd+=get_bitcount((octree[ofs+(i>>2)]>>((i&3)<<3))&255);
			}
			return octree[ofs+(nadd>>2)]>>((nadd&3)<<3);
		}
	}	
	return octree[n+nadd];//octree[(n+nadd)%10842491] ; // ptr +  
}

#define CAST_RAY(BREAK_COND) \
const int CUBESIZE=SCALE_MAX;\
sign_x = 0, sign_y = 0, sign_z = 0;\
rekursion= OCTREE_DEPTH;\
float epsilon = pow(2.0f,-20.0f);\
if(fabs(delta.x)<epsilon)delta.x=sign(delta.x)*epsilon;\
if(fabs(delta.y)<epsilon)delta.y=sign(delta.y)*epsilon;\
if(fabs(delta.z)<epsilon)delta.z=sign(delta.z)*epsilon;\
\
if (delta.x < 0){ sign_x = 1; raypos.x = SCALE_MAX-raypos.x; delta.x= -delta.x; }\
if (delta.y < 0){ sign_y = 1; raypos.y = SCALE_MAX-raypos.y; delta.y= -delta.y; }\
if (delta.z < 0){ sign_z = 1; raypos.z = SCALE_MAX-raypos.z; delta.z= -delta.z; }\
\
grad0.x = 1;\
grad0.y = delta.y/delta.x;\
grad0.z = delta.z/delta.x;\
\
grad1.x = delta.x/delta.y;\
grad1.y = 1;\
grad1.z = delta.z/delta.y;\
\
grad2.x = delta.x/delta.z;\
grad2.y = delta.y/delta.z;\
grad2.z = 1;\
\
len0.x = length(grad0);\
len0.y = length(grad1);\
len0.z = length(grad2);\
\
float3 raypos_orig=raypos;\
raypos_int = convert_int3(raypos);\
nodeid = octree_root;\
nodeid_before = octree_root;\
local_root = 0;\
x0r = 0; x0ry = 0;\
sign_xyz = sign_x | (sign_y<<1) | (sign_z<<2) ;\
raypos_before=raypos_int ;\
\
do\
{\
	int3 p0i = raypos_int >> rekursion;    \
	int3 bit = p0i & 1;\
\
	int node_index =(bit.x|(bit.y<<1)|(bit.z<<2))^sign_xyz;\
	    node_test = nodeid /*childBits 0..8*/ & (1<<node_index);\
\
	if ( node_test )\
	{\
		uint tmp = nodeid ;\
\
		nodeid = fetchChildOffset( a_octree, \
			nodeid , nodeid_before, &local_root, \
			node_index, node_test, /*bitlookup*/\
			rekursion);\
\
		nodeid_before = tmp;\
\
		/*hit*/\
		if(rekursion<=lod)break;\
\
		/* step down the tree*/\
		rekursion--;\
\
		stack[stack_add+rekursion]=nodeid;\
		continue;\
	}\
\
	float3 mul0=convert_float3((p0i+1)<<rekursion)-raypos;\
	\
	/* Backup position*/\
	raypos_before = convert_int3(raypos);\
\
	/* Next intersections*/\
	float3 isec_dist = mul0 * len0;\
	float3 isec0 = grad0*mul0.x;\
	if (isec_dist.y<isec_dist.x){isec_dist.x=isec_dist.y;isec0=grad1*mul0.y;}\
	if (isec_dist.z<isec_dist.x){isec_dist.x=isec_dist.z;isec0=grad2*mul0.z;}\
\
	raypos    += isec0;	\
	raypos_int = convert_int3(raypos);\
	distance  += isec_dist.x;\
\
	/* Compute modified bits */\
	x0rx=raypos_before.x^raypos_int.x;\
	x0ry=raypos_before.y^raypos_int.y;\
	x0rz=raypos_before.z^raypos_int.z;\
	x0r =x0rx^x0ry^x0rz;\
\
	int rekursion_new=log2(convert_float(x0r&OCTREE_DEPTH_AND));\
	if(distance > VIEW_DIST_MAX)break; \
	if(rekursion_new<rekursion) continue;\
	rekursion		= rekursion_new+1;\
	nodeid			= stack[stack_add+rekursion];\
	nodeid_before	= stack[stack_add+rekursion+1];\
	if(rekursion == OCTREE_DEPTH) nodeid_before = nodeid = octree_root;\
	if(distance > lodswitch) { lodswitch*=2;++lod;}\
\
}while(!(BREAK_COND&(2*OCTREE_DEPTH_AND+2)));\
if(sign_x){raypos.x = SCALE_MAX-raypos.x; delta.x= -delta.x; };\
if(sign_y){raypos.y = SCALE_MAX-raypos.y; delta.y= -delta.y; };\
if(sign_z){raypos.z = SCALE_MAX-raypos.z; delta.z= -delta.z; };



struct BVHNode  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	uint childs[2];        // offset of each child
	float p[12];
	//float3 p[4];
};
struct BVHChild  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	float3 min,max;
	float4 m0;
	float4 m1;
	float4 m2;
	float4 m3;
	uint octreeroot;
};

__kernel    void raycast_counthole(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame)
			//int wx1,int wx2,int wy1,int wy2)
{
	const int idx = get_global_id(0);
	const int idy = get_global_id(1);
	if(idx >= res_x/16 ) return;
	if(idy >= res_y/16 ) return;
	
	int count=0;

	int x=idx*16;
	int y=idy*16;
	/*
	if(x >=wx1 && x <wx2 && y >=wy1 && y <wy2)
	{
		a_idBuffer[idx+idy*(res_x/16)]=0;
		return;
	}
	*/

	int ofs=x+y*res_x;
	int dx=min(res_x-x,16)/2;
	int dy=min(res_y-y,16)/2;

	loopj(0,dy) 
	loopi(0,dx) 
	{
		int o=ofs+i*2+j*2*res_x;
		int a=0;
		if(a_screenBuffer[o]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+1]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+1+res_x]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a_screenBuffer[o+res_x]==0xffffff00 )a++;   // || ((i^j)==(frame&15))) 
		if(a>=HOLE_PIXEL_THRESHOLD)count+=4;
	}
	a_idBuffer[idx+idy*(res_x/16)]=count;
}

__kernel    void raycast_sumids(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame)
{
	const int idx = get_global_id(0);

	if(idx > 0 ) return;
	
	int ofs=0;
	int size=(res_x/16)*(res_y/16);

	loopi(0,size)
	{
		int len=a_idBuffer[i];
		a_idBuffer[i+size]=ofs;
		ofs+=len;
	}
	a_idBuffer[0]=ofs;	
}

__kernel    void raycast_writeids(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_idBuffer,
			int  res_x, int res_y, int frame)
			//int wx1,int wx2,int wy1,int wy2)
{
	const int idx = get_global_id(0);
	const int idy = get_global_id(1);
	if(idx >= res_x/16 ) return;
	if(idy >= res_y/16 ) return;

	int size=(res_x/16)*(res_y/16);
	int count=0;
	int x=idx*16;
	int y=idy*16;
	//if(x >=wx1 && x <wx2 && y >=wy1 && y <wy2) return;

	int ofs=x+y*res_x;	
	int dstofs=a_idBuffer[idx+idy*(res_x/16)+size]+size*2;
	int dx=min(res_x-x,16)/2;
	int dy=min(res_y-y,16)/2;

	loopj(0,dy) 
	loopi(0,dx) 
	{
		int a=0,o=ofs+i*2+j*2*res_x,val=x+i*2+((j*2+y)<<16);
		int v=val;

		if(a_screenBuffer[o]==0xffffff00 )			a++;
		if(a_screenBuffer[o+1]==0xffffff00 )		a++;
		if(a_screenBuffer[o+1+res_x]==0xffffff00 )	a++;
		if(a_screenBuffer[o+res_x]==0xffffff00 )	a++;

		if(a>=HOLE_PIXEL_THRESHOLD)
		{
			a_idBuffer[dstofs]=val;	dstofs++;
			a_idBuffer[dstofs]=val+1;	dstofs++;
			a_idBuffer[dstofs]=val+1+(1<<16);dstofs++;
			a_idBuffer[dstofs]=val+(1<<16);	dstofs++;
		}
	}
}

__kernel    void raycast_fillhole(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global int *a_xbuffer,
			__global int *a_ybuffer,
			__global float *a_zbuffer,
			int  res_x, int res_y, int frame)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x-4 || idy >= res_y-4 || idx<3 || idy<3 ) return;

	    
	uint  of=idy*res_x+idx;

	uint  c0=a_screenBuffer[of];
	if(c0==0xffffff00) return;

	float z0=a_backBuffer  [of*4+3];
	float z1=z0;
	uint  c1=c0;

	// todo add x/y velocity check
	
	int xi,yj;

	int add_x=0;//frame&3;
	int add_y=0;//(frame>>2)&3;
	//int ofs_add=of+add_y*res_x+add_x;

	loopi(-1+add_x,1+add_x)
	loopj(-1+add_y,1+add_y)
	{
		uint ofs=of+j*res_x+i;
		float z=a_backBuffer[ofs*4+3];
		if(z<z1){ xi=i;yj=j;z1=z; }		
	}
	int dx0=a_xbuffer[of];
	int dy0=a_ybuffer[of];

	int oij=of+yj*res_x+xi;
	int dx=a_xbuffer[oij];
	int dy=a_ybuffer[oij];

	if(z0<=z1)return;

	if(fabs(z1-z0)>min(z0,z1)*Z_CONTINUITY*4.0)
	//if (z1<80 )	if (z1+5<z0) 
	if (abs(dx-dx0)>0 || abs(dy-dy0)>0 )
	//if ((dx>=0 && xi>=0)||(dx<=0 && xi<=0)||(dy>=0 && yj>=0)||(dy<=0 && yj<=0))
	{
		a_screenBuffer[of]=0xffffff00;//a_screenBuffer[oij];//0xffffff00;
		//a_backBuffer[of*4+0]=999999999999;//a_backBuffer[oij*4+0];
		//a_backBuffer[of*4+1]=999999999999;//a_backBuffer[of+1]=a_backBuffer[oij+1];
		//a_backBuffer[of*4+2]=999999999999;//a_backBuffer[of+2]=a_backBuffer[oij+2];
		//a_backBuffer[of*4+3]=999999999999;//a_backBuffer[of+3]=a_backBuffer[oij+3];
	}
}


__kernel    void raycast_fillhole2(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			int  res_x, int res_y, int frame
			//int wx1,int wx2,int wy1,int wy2
			//,float4 vx,float4 vy
			)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x-1 || idy >= res_y-1 || idx<=1 || idy<=1 ) return;
	//if(idx >=wx1 && idx <wx2 && idy >=wy1 && idy <wy2) return;

	int ofs=idy*res_x+idx;
	uint col=a_screenBuffer[ofs];
	if(col!=0xffffff00 ) return;

	int col1=a_screenBuffer[ofs+1];
	int col2=a_screenBuffer[ofs-1];
	int col3=a_screenBuffer[ofs+res_x];
	int col4=a_screenBuffer[ofs-res_x];

	if(col1!=0xffffff00 ) 
	if(col2!=0xffffff00 ) 
	if(col3!=0xffffff00 ) 
	if(col4!=0xffffff00 ) 
	{
		a_screenBuffer[ofs]=(col1&3)+
			((((col1&0xfc)+(col2&0xfc)+(col3&0xfc)+(col4&0xfc))>>2)&0xfc);
		return;
	}

	if(col1!=0xffffff00 ) 
	if(col2!=0xffffff00 ) 
	{
		a_screenBuffer[ofs]=(col1&3)+((((col1&0xfc)+(col2&0xfc))>>1)&0xfc);
		return;
	}

	if(col3!=0xffffff00 ) 
	if(col4!=0xffffff00 ) 
	{
		a_screenBuffer[ofs]=(col3&3)+((((col3&0xfc)+(col4&0xfc))>>1)&0xfc);
		return;
	}


	loopi(0,4)
	{
		int o=ofs+(i&1)+((i>>1)&1)*res_x;
		col=a_screenBuffer[o];
		if(col!=0xffffff00)break;
	}
	
	if(col==0xffffff00 )
	loopi(-2,3)
	loopj(-2,3)
	{
		int o=ofs+i+j*res_x;
		if(col!=0xffffff00)break;
		col=a_screenBuffer[o];
	}

	a_screenBuffer[ofs]=col;
}

__kernel    void raycast_proj(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global int *a_xbuffer,
			__global int *a_ybuffer,
			__global int *a_zbuffer,
			int  res_x, int res_y, int frame,int ofs_add,
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz//,
		//	int save_coord
			//int wx1,int wx2,int wy1,int wy2
			)
{
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	const int idx = get_global_id(0)	,idy = get_global_id(1);
	if(idx >= res_x || idy >= res_y ) return;
	
	int tid=ty*bw+tx;

	int ofs=idy*res_x+idx;

	ofs+=ofs_add; //backbuffer
	uint srcofs=ofs;
	uint col=a_screenBuffer[ofs];
	if(col==0xffffff00) return;

	if(0)
	if(col>100*256*256 )
	if(idx>0)if(idy>0)
	{
		loopi(0,4)
		{
			int addx=0,addy=0;

			if(i==0){addx=1;}
			if(i==1){addy=1;}
			if(i==2){addx=-1;}
			if(i==3){addy=-1;}
			int add=addx+addy*res_x;
			uint col0=a_screenBuffer[ofs+add];

		/*loopi(-1,2)
		loopj(-1,2)
		{
			uint col0=a_screenBuffer[ofs_src+i+j*res_x];*/

			if( col0<100*256*256)
			{
				return;
				//col=col0;
				//break;
				{a_screenBuffer[ofs]=0xffffff00; return;}
			}
		}
		//col=a_screenBuffer[ofs_src]=0xffffff00;//col;
	}

	float3 pcache;

	pcache.x = a_backBuffer[ofs*4+0];//-a_m0.x;

	//if(pcache.x>999999999){ a_screenBuffer[srcofs]=0xffffff00; return;}

	pcache.y = a_backBuffer[ofs*4+1];//-a_m0.y;
	pcache.z = a_backBuffer[ofs*4+2];//-a_m0.z;
	float pcache_z_screen=a_backBuffer[ofs*4+3];//-a_m0.z;
	ofs-=ofs_add; //frontbuffer

	float3 p=pcache.xyz-a_m0.xyz;
	float3 phit; 
	phit.x = dot(p,a_mx.xyz);
	phit.y = dot(p,a_my.xyz);
	phit.z = dot(p,a_mz.xyz);
	
	if (phit.z<0.05) { a_screenBuffer[srcofs]=0xffffff00; return;}

	float3 scrf;
	scrf.x=(phit.x*convert_float(res_y)+0.0)/phit.z+convert_float(res_x)/2-0.0;
	scrf.y=(phit.y*convert_float(res_y)+0.0)/phit.z+convert_float(res_y)/2-0.0;
	scrf.z=phit.z;
	int2 scr;
	scr.x=scrf.x;
	scr.y=scrf.y;
	
	if(scr.x>=res_x-1)	{ a_screenBuffer[srcofs]=0xffffff00; return;}
	if(scr.x<0     )	{ a_screenBuffer[srcofs]=0xffffff00; return;}
	if(scr.y>=res_y-1)	{ a_screenBuffer[srcofs]=0xffffff00; return;}
	if(scr.y<0     )	{ a_screenBuffer[srcofs]=0xffffff00; return;}
//	if(scr.x>=wx1 && scr.x<wx2)
//	if(scr.y>=wy1 && scr.y<wy2) { a_screenBuffer[srcofs]=0xffffff00; return;}

	ofs=scr.y*res_x+scr.x;									
	int sz=phit.z*1000; sz<<=8; int val=sz+(col&255);
	
	//barrier(CLK_GLOBAL_MEM_FENCE);

	if((a_screenBuffer[ofs]&0xffffff00) <= sz) return;//{ a_screenBuffer[srcofs]=0xffffff00; return;}
	if(atom_min(&a_screenBuffer[ofs],val)<val) return;//{ a_screenBuffer[srcofs]=0xffffff00; return;}

	//	barrier(CLK_GLOBAL_MEM_FENCE); /// < add this to remove error pixels

	//a_zbuffer[ofs]=srcofs;//scrf.z;//pcache_z_screen;
	//return;

	if(a_screenBuffer[ofs]!=val) return;//{ a_screenBuffer[srcofs]=0xffffff00; return;}
	//barrier(CLK_GLOBAL_MEM_FENCE);

	a_backBuffer[ofs*4+0] = pcache.x;
	a_backBuffer[ofs*4+1] = pcache.y;
	a_backBuffer[ofs*4+2] = pcache.z;
	a_backBuffer[ofs*4+3] = phit.z;

	//a_xbuffer[ofs]=convert_int(scr.x)-convert_int(idx);
	//a_ybuffer[ofs]=convert_int(scr.y)-convert_int(idy);
	//a_zbuffer[ofs]=pcache_z_screen;//scrf.z;//pcache_z_screen;

	//barrier(CLK_GLOBAL_MEM_FENCE);
}

__kernel    void raycast_holes(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			__global struct BVHNode *a_bvh_nodes,
			__global struct BVHChild *a_bvh_childs,
			__global uint *a_stack,
			__global uint *a_idbuffer,
			uint octree_root,
			int  res_x, int res_y, int frame, int idbuf_size,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	//const int idx = get_global_id(0)	,idy = get_global_id(1);
	const int id = get_global_id(0);
	if(id>=idbuf_size) return;

	int idsize=(res_x/16)*(res_y/16);
	uint idxy=a_idbuffer[id+idsize*2];

	int idx=idxy&0xffff;
	int idy=idxy>>16;

	if(idx >= res_x || idy >= res_y ) return;
	
	// Anything to do?
	//if((frame&3)!=(idx&3))

	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * (OCTREE_DEPTH-1))-1;

	float3 delta1;
	delta1.x=convert_float(idx-res_x/2+0.5)*fovx/convert_float(res_y);
	delta1.y=convert_float(idy-res_y/2+0.5)*fovy/convert_float(res_y);
	delta1.z=1;

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);//a_mx*delta1.x;
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);

	float rayLength=0,distance=0;
	uint  col=0xffff0002;
	float3 phit;

	// Params
	//float3 delta    = dir;//a_origin.xyz + a_dx.xyz * convert_float(idx) + a_dy.xyz * convert_float(idy);
	float3 raypos = a_m0.xyz*16.0f;//+normalize(delta)*distance)*16.0f;

	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=1.0, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint nodeid, nodeid_before, local_root, node_test ;
	float lodswitch = res_x*LOD_ADJUST*2;//3/2;
	CAST_RAY(x0ry);
	// --------- // Raycast > End
	{
		col = fetchColor(  a_octree,
							nodeid,
							nodeid_before, 
							stack[stack_add+rekursion+1],
							&local_root, 
							rekursion,node_test  );
		//col ^=0xffffff;
		//rayLength=distance/16.0f;
		phit=raypos/16.0f;
	}				
	int ofs=idy*res_x+idx;

	//col=convert_int((raypos.x+raypos.y+raypos.z)/10)&0xfc;

	//float3 
	//phit=rayLength*dir+a_cam.xyz;

	//if(rayLength>=30000)col=0xff0000f2;

	loopi(0,1)
	{
		a_screenBuffer[ofs] = 0xff000000+col;
		a_backBuffer[ofs*4+0] = phit.x;
		a_backBuffer[ofs*4+1] = phit.y;
		a_backBuffer[ofs*4+2] = phit.z;
		ofs+=res_x*res_y;
	}
}

__kernel    void raycast_fine(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			uint octree_root,
			int  res_x, int res_y, int frame, 
			int add_x,int add_y,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	
	int idx0= get_global_id(0)  ,idy0= get_global_id(1)  ;
	int idx = idx0*2			,idy = idy0*2;

	idx+=add_x;
	idy+=add_y;

	if(idx >= res_x || idy >= res_y ) return;

	// find empty
	{
		uint col=0;

		loopi(0,4)
		{
			int o=(idy+(i>>1))*res_x+idx+(i&1);
			col=a_screenBuffer[o];
			if(col==0xffffff00)
			{
				idx+=i&1;
				idy+=i>>1;
				break;
			}
		}
		if(col!=0xffffff00)
		{
			idx+=((frame>>2)&1);
			idy+=((frame>>3)&1);
		}
	}

	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * (OCTREE_DEPTH-1))-1;
	

	float3 delta1;
	delta1.x=convert_float(idx-res_x/2+0.5)*fovx/convert_float(res_y);
	delta1.y=convert_float(idy-res_y/2+0.5)*fovy/convert_float(res_y);
	delta1.z=1;

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);
	
	float rayLength=0,distance=0;
		
	uint  col=0xff0000f2;
	float3 phit;

	// Params
	float3 raypos = a_m0.xyz*16.0f;


	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=1.0, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint nodeid, nodeid_before, local_root, node_test ;
	float lodswitch = res_x*LOD_ADJUST*2;//*3/2;
	CAST_RAY(x0ry);
	// --------- // Raycast > End
	{
		col = fetchColor(  a_octree,
							nodeid,
							nodeid_before, 
							stack[stack_add+rekursion+1],
							&local_root, 
							rekursion,node_test  );
		//col ^=0xffffff;
		rayLength=distance;
		phit=raypos/16.0f;
	}				

	//col=convert_int((raypos.x+raypos.y+raypos.z)/10)&0xfc;

	//if(col==0xff0000f2) return;

	int ofs=idy*res_x+idx;

	//if(rayLength>=30000)col=0xff0000f2;
	//float3 
	//phit=rayLength*dir+a_cam.xyz;
	loopi(0,1)
	{	
	a_screenBuffer[ofs] = 0xff000000+col;//^35345;
	a_backBuffer[ofs*4+0] = phit.x;
	a_backBuffer[ofs*4+1] = phit.y;
	a_backBuffer[ofs*4+2] = phit.z;
	ofs+=res_x*res_y;
	}
	return;

	if(a_screenBuffer[idy*res_x+idx]==0xffffff00)
	{
		loopi(0,4)
		{
			a_screenBuffer[ofs] = col;//^35345;
			a_backBuffer[ofs*4+0] = phit.x;
			a_backBuffer[ofs*4+1] = phit.y;
			a_backBuffer[ofs*4+2] = phit.z;
			ofs+=res_x*res_y;
		}
	}
	else
	{
		loopi(0,4)
		{
			float3 p;
			p.x=a_backBuffer[ofs*4+0];
			p.y=a_backBuffer[ofs*4+1];
			p.z=a_backBuffer[ofs*4+2];
			if ( length(p-phit) > 3  )
			{
				a_screenBuffer[ofs] = col;//^35345;
				a_backBuffer[ofs*4+0] = phit.x;
				a_backBuffer[ofs*4+1] = phit.y;
				a_backBuffer[ofs*4+2] = phit.z;
			}
			ofs+=res_x*res_y;
		}
	}
}


__kernel    void raycast_fine_2(
			__global uint *a_screenBuffer,
			__global float *a_backBuffer,
			__global uint *a_octree,
			uint octree_root,
			int  res_x, int res_y, int frame, 
			int add_x,int add_y,
			float4 a_cam, 
			float4 a_origin, 
            float4 a_dx, 
			float4 a_dy,
	
			float4 a_m0, 
			float4 a_mx, 
            float4 a_my, 
			float4 a_mz,
			float fovx,
			float fovy)
{
	//__local 
	const int tx = get_local_id(0)		,ty = get_local_id(1);
	const int bw = get_local_size(0)	,bh = get_local_size(1);
	
	int idx0= get_global_id(0)  ,idy0= get_global_id(1)  ;
	int idx = idx0			,idy = idy0;

	idx+=add_x;
	idy+=add_y;

	if(idx >= res_x || idy >= res_y ) return;
	
	// 16x16 threads?
	__local uint stack[16000/4]; // octree stack
	int thread_num = tx + ty * bw;
	uint stack_add=(thread_num * (OCTREE_DEPTH-1))-1;
	

	float3 delta1;
	delta1.x=convert_float(idx-res_x/2+0.5)*fovx/convert_float(res_y);
	delta1.y=convert_float(idy-res_y/2+0.5)*fovy/convert_float(res_y);
	delta1.z=1;

	float3 delta;
	delta.x=dot(delta1,a_mx.xyz);
	delta.y=dot(delta1,a_my.xyz);
	delta.z=dot(delta1,a_mz.xyz);
	
	float rayLength=0,distance=0;
		
	uint  col=0xff0000f2;
	float3 phit;

	// Params
	float3 raypos = a_m0.xyz*16.0f;


	// --------- // Raycast > Begin
	int sign_x, sign_y, sign_z,rekursion;
	float3  grad0,grad1,grad2,len0;
	int3 raypos_int,raypos_before; 
	int lod=1.0, x0r, x0rx, x0ry, x0rz, sign_xyz;
	uint nodeid, nodeid_before, local_root, node_test ;
	float lodswitch = res_x*LOD_ADJUST*2;//*3/2;
	CAST_RAY(x0ry);
	// --------- // Raycast > End
	{
		col = fetchColor(  a_octree,
							nodeid,
							nodeid_before, 
							stack[stack_add+rekursion+1],
							&local_root, 
							rekursion,node_test  );
		//col ^=0xffffff;
		rayLength=distance;
		phit=raypos/16.0f;
	}				

	//col=convert_int((raypos.x+raypos.y+raypos.z)/10)&0xfc;

	//if(col==0xff0000f2) return;

	int ofs=idy*res_x+idx;

	//if(rayLength>=30000)col=0xff0000f2;
	//float3 
	//phit=rayLength*dir+a_cam.xyz;
	loopi(1,2)
	{	
	a_screenBuffer[ofs] = 0xff000000+col;//^35345;
	a_backBuffer[ofs*4+0] = phit.x;
	a_backBuffer[ofs*4+1] = phit.y;
	a_backBuffer[ofs*4+2] = phit.z;
	ofs+=res_x*res_y;
	}
	return;

}

__kernel    void raycast_colorize(
			__global uint *p_screenBuffer,
			__global uint *p_screenBuffer_tex,
			int p_width, 
			int p_height
			)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	if( x >= p_width ) return;
	if( y >= p_height ) return;

	int of=x+y*p_width;
	int a=p_screenBuffer[of];//&0xff;

	float3 color_tab[4]={
		{1.0,1.0,1.0},
		{1.0,0.7,0.3},
		{1.5,0.8,0.1},
		{0.2,0.8,0.2}};

	int col=a&3;
	float3 rgb=color_tab[col];
	float  i=convert_float((a&(255-7)));
	int    r=i*rgb.x;if(r>255)r=255;
	int    g=i*rgb.y;if(g>255)g=255;
	int    b=i*rgb.z;if(b>255)b=255;
	p_screenBuffer_tex[of]=b+g*256+r*65536;

	//if((a&0xffff00)==0xffff00) p_screenBuffer[of]=0x0088cc;
}

#include "Rle4.h"
#define OCTREE_DEPTH 11

struct OctreeNodeNew  // if members of this structure are changed, most cuda fetch-functions (and normal shaders) must be adjusted, too!
{
	unsigned int childOffset[8];        // offset of each child
	uchar4 color;
};

//
/*
ptr+=bits_ptr24 & (0xffffff>>((iter&4)<<1));

struct node128
{
struct Ptr128 { uchar bits,col;ushort ptr16;}; // iter 0..3
struct Ptr128 { uint  bits_ptr24;}; // iter 4..7
Ptr128 [0..7];
}

std::vector<uint> nodes_colors;
*/

uint octree_root = 0;
uint octree_root_normal = 0;
uint octree_size = OCTREE_DEPTH;
uint num_voxels = 0;
std::vector<OctreeNodeNew>		octree_array;
std::vector<uint>				octree_array_compact;
std::vector<OctreeNodeNew>		octree_array_normal;

void set_voxel(uint x,uint y,uint z,uchar4 color)
{
//	if(y<250) return;
//	if(num_voxels>=(1<<23)-1) return;
	num_voxels++;
	uint offset = 0;
	uint parent_offset = 0;
	uint parent_childbit = 0;
	uint bit = octree_size-1;
	uint child_offset  =0 ;
	uint childbit = 0;

	OctreeNodeNew node;

	memset (&node,0,sizeof(	OctreeNodeNew ));

	node.color=color;

	while(bit>=0)
	{
		uint bitx = (x >> bit)&1;
		uint bity = (y >> bit)&1;
		uint bitz = (z >> bit)&1;

		parent_childbit=childbit;

		childbit =  (bitx+bity*2+bitz*4);

		child_offset = octree_array[offset>>8].childOffset[childbit];
		
		if (bit<octree_size-1) 
			octree_array[parent_offset>>8].childOffset[parent_childbit] |= 1<<childbit;
		else
			octree_root |= 1<<childbit;

		if (bit == 0 )
		{
			octree_array[offset>>8].childOffset[childbit]=*((int*)(&color));
			break;
		}

		parent_offset=offset;

		if (child_offset==0)
		{	
			//node.color.w=bit;
			uint insert_ofs = octree_array.size()<<8;
			octree_array[offset>>8].childOffset[childbit]=insert_ofs;
			octree_array.push_back( node );
			offset = insert_ofs;
		}
		else
		{
			offset = child_offset;
		}
		bit --;
	}
}
///////////////////////////////////////////
int _iteration = 0;

std::vector<uchar> color_block;

uint convert_tree(uint offset,uint local_root)
{
	/*
	if (_iteration == octree_size )
	{
//		printf("color follows\n");
		return offset;
	}
	*/
	_iteration++;

	if (_iteration == octree_size-1 )
	{
	//if(rekursion==1)
	//{
	//	return ((octree[n+lroot+(nadd>>2)]>>(nadd&3))&255)+(nadd<<8);
	//}
		uint current_ofs = ((octree_array_compact.size()-local_root) << 9) + 256 + (offset&0xff) ;

		OctreeNodeNew node = octree_array[offset>>8];

		color_block.clear();
		color_block.push_back(node.color.to_uint());

		for (int j=0;j<8;j++)
		{
			uint childofs = node.childOffset[j];
			if ( childofs > 0 )
				color_block.push_back(childofs);
		}
		for (int j=0;j<8;j++)
		{
			uint childofs = node.childOffset[j];
			if ( childofs > 0 )
			{
				OctreeNodeNew n = octree_array[childofs>>8];
				for (int i=0;i<8;i++)
				{
					uint childofs = n.childOffset[i];
					if ( childofs > 0 )
						color_block.push_back(childofs);
				}
			}
		}
		int size= color_block.size();// + 3 ) & 0xfffffffc;
		//color_block.resize(size);

		uint dw=0;
		int i03=0;
		loopi (0,size)
		{
			i03=i&3;
			dw|=uint(color_block[i])<<(i03*8);
			if(i03==3)
			{
				octree_array_compact.push_back(dw);
				dw=0;
			}
		}
		if(i03<3)
		{
			octree_array_compact.push_back(dw);
		}

				
		//		printf("color follows\n");
	
		_iteration--;
		return current_ofs;//(offset&255)+((size>>2)<<8);
	}

	OctreeNodeNew node = octree_array[offset>>8];

	for (int j=0;j<8;j++)
	{
		uint childofs = node.childOffset[j];
		if ( childofs > 0 )
		{
			node.childOffset[j] = convert_tree(childofs,local_root);
//			printf("node.childOffset[%d]=%d (%d)\n",j,(node.childOffset[j]>>9),((node.childOffset[j]>>8)&1));
		}
	}

	uint current_ofs = ((octree_array_compact.size()-local_root) << 9) + 256 + (offset&0xff) ;

	octree_array_compact.push_back(*((uint*)&node.color));

	for (int j=0;j<8;j++) if ( node.childOffset[j] > 0 )
	{						   
		octree_array_compact.push_back( node.childOffset[j] );
	}
	//octree_array_compact.push_back(*((uint*)&node.normal));

	_iteration--;

	return current_ofs ;
}
///////////////////////////////////////////
uint get_bytebitcount(uint b)
{
	b = (b & 0x55) + (b >> 1 & 0x55);
	b = (b & 0x33) + (b >> 2 & 0x33);
	b = (b + (b >> 4)) & 0x0f;
	return b;
}			 

uint lastblockoffset=0;
uint blockcount=0;

uint octree_normal_offset=0;

std::vector<uint> octree1b;
/*
uint convert_octree1b(uint offset)
{
	static int iteration=0;
	iteration++;

	OctreeNodeNew node = octree_array[offset>>8];

	std::vector <uint> colors;

	for (int j=0;j<8;j++)
	if ( uint childofs = node.childOffset[j] > 0 )
	{
		//uint childofs = node.childOffset[j];
	
	}
	_iteration--;

	return 0;
}
*/


//convert_tree_blocks(childofs);

uint convert_tree_blocks(uint offset)
{
	_iteration++;

	OctreeNodeNew node = octree_array[offset>>8];

	for (int j=0;j<8;j++)
	{
		uint childofs = node.childOffset[j];

		/* mixed octree */
		if ( childofs > 0 )
		{
			if (_iteration < octree_size-6 ) 
			{
				// old tree structure				
				node.childOffset[j] = convert_tree_blocks(childofs);
				//printf("blocks node.childOffset[%d]=%d (%d)\n",j,(node.childOffset[j]>>9),((node.childOffset[j]>>8)&1));
			}			
			else
			{
				// attach new tree structure
				uint size64=((octree_array_compact.size()+63)>>6)<<6;
				int local_root=size64;
			//printf("block %d size:%d kb\n",blockcount,(size64*4/1024)-lastblockoffset);

				lastblockoffset=(size64*4/1024);
				blockcount++;

				int num_entries = get_bytebitcount(childofs&255)+1;

				octree_array_compact.resize(size64+num_entries);

				node.childOffset[j] = ((local_root>>6)<<9) | (1<<8) | (childofs&255);

				//if (j&4) node.childOffset[j] =0;
//				printf("blocks local root node.childOffset[%d]=%d (%d)\n",j,(node.childOffset[j]>>9),((node.childOffset[j]>>8)&1));

				int subtree_root = convert_tree(childofs,local_root) >> 9;

				for (int i=0;i<num_entries;i++)
					octree_array_compact [  local_root + i ] =
						octree_array_compact  [ local_root + subtree_root + i ];					

				octree_array_compact.resize(size64+ subtree_root );
			}
		}
	}
	octree_array_compact[octree_normal_offset+8] = *((int*)(&node.color)) ;
	octree_array_compact[octree_normal_offset+9] = *((int*)(&node.color)) ;

	for (int j=0;j<8;j++)
	{
		uint o = node.childOffset[j];
		octree_array_compact[octree_normal_offset+j]=o;
	}
	octree_normal_offset+=10;

	_iteration--;

	return ((octree_normal_offset-10)<< 9) | (0<<8) | (offset & 255);
}

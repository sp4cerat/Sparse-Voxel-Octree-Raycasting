
cl_mem	mem_octree;
cl_mem	mem_bvh_nodes;
cl_mem	mem_bvh_childs;
cl_mem	mem_backbuffer;
cl_mem	mem_screenbuffer;
cl_mem	mem_screenbuffer_tex;
cl_mem	mem_stack;
int		mem_pbo_out;

int octree_dim=1<<OCTREE_DEPTH;

void octree_init()
{
	OctreeNodeNew node;
	memset (&node,0,sizeof(	OctreeNodeNew ));
	octree_array.push_back(node);

	RLE4 rle;
	rle.load("../data/Imrodh.rle4",0,0,0,0);

	if(0)
	loopi(0,2)
	loopj(0,2)
	if(i>0 || j>0)
	{
		int id=i+j*4;
		octree_array[octree_root>>8].childOffset[id]=octree_array[octree_root>>8].childOffset[0];
		octree_root|=1<<id;
		octree_array[octree_root>>8].childOffset[id+2]=octree_array[octree_root>>8].childOffset[2];
		octree_root|=1<<(id+2);
	}
//	Tree tree(octree_dim,octree_dim,octree_dim);
//	tree.loadPLY("../../data/imrod.ply");
//	tree.loadPLY("../../data/Imrodh.ply");
	
	// Main tree
	octree_array_compact.resize(2097152);
	octree_root_normal = convert_tree_blocks( octree_root );
    printf("octree_root_normal =%d (%d)\n",(octree_root_normal>>9),((octree_root_normal>>8)&1));	
	printf("Conversion complete. %2.2f Bit Pos/Voxel  %2.2f Bit Col/Voxel \n"
		, float(octree_array_compact.size())*32/float(num_voxels)-32.0f*1.12f,
		float(32.0f*1.12f)	);
    printf("Original size: %2.2f MB\n",float(octree_array.size()*sizeof(OctreeNodeNew))/(1024*1024));	
    printf("New size: %2.2f MB\n",float(octree_array_compact.size()*4)/(1024*1024));	
}

void calcViewPlaneGradients(
	float res_x,float res_y,
	vec3f right,vec3f up,vec3f direction,
	vec3f& a_leftTop,vec3f& a_dx,vec3f& a_dy)
{
   float fov=float(WINDOW_WIDTH)/float(WINDOW_HEIGHT);
   up=up/fov;
//   a_leftTop=direction*2.0f/fov-right-up;
   a_leftTop=direction*2.0f-right-up;
   a_dx = right *2.0f/float(res_x);
   a_dy = up    *2.0f/float(res_y);
}

void raycast_init()
{
	octree_init();

	// >> Memory

	// octree
	mem_octree=ocl_malloc(octree_array_compact.size()*4,&octree_array_compact[0] );

	// bvh nodes
	//mem_bvh_nodes=ocl_malloc(bvh_nodes.size()*sizeof(BVHNode), &bvh_nodes[0]);

	// bvh chlds
	//mem_bvh_childs=ocl_malloc(bvh_childs.size()*sizeof(BVHChild), &bvh_childs[0]);

	// stack
	mem_stack=ocl_malloc(32*4*1024*1024);

	int size=WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX /*4 buffers*/;//WINDOW_WIDTH*WINDOW_HEIGHT*4;

	// backbuffer
	mem_backbuffer=ocl_malloc(size*16*4); 

	// screenbuffers
	mem_screenbuffer=ocl_malloc(size*4*4); 

	// >> PBO
	mem_pbo_out=ogl_pbo_new(size*4);
	mem_screenbuffer_tex=ocl_pbo_map(mem_pbo_out);

}

void raycast_draw(int res_x, int res_y)
{
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	int COARSE_SCALE=1;	
	float vsize=2<<OCTREE_DEPTH;
	static int frame=-1; frame++;

	int size_col=res_x*res_y*4; // color buffer size in bytes
	int size_xyz=size_col*4;	// xyz   buffer size in bytes

	float fovx = 1.0;//float (WINDOW_WIDTH) / float (res_x);
	float fovy = 1.0;//float (WINDOW_HEIGHT)/ float (res_y);

	// screen params
	static vec3f pos(1,50,1);
	vec3f rot(0.0001,0,0);

	rot.y=(MOUSE_X-WINDOW_WIDTH/2)*0.01;
	rot.x=(MOUSE_Y-WINDOW_HEIGHT/2)*0.01;

	//printf("pos %.2f %.2f %.2f rot  %.2f %.2f %.2f\r",pos.x,pos.y,pos.z,rot.x,rot.y,rot.z);

	matrix44 m;
	m.rotate_z( rot.z );
	m.rotate_x( rot.x );
	m.rotate_y( rot.y );
	
	vec3f front	= m * vec3f( 0, 0, 1);
	vec3f side	= m * vec3f( 1, 0, 0);
	vec3f up	= m * vec3f( 0, 1, 0);

	if(KEYTAB[SDLK_w]) pos+=front*20/fps;
	if(KEYTAB[SDLK_s]) pos-=front*20/fps;
	if(KEYTAB[SDLK_a]) pos-=side*20/fps;
	if(KEYTAB[SDLK_d]) pos+=side*20/fps;

	// loop
	{
		vec3f p=pos*16.0;
		while(p.x<0)p.x+=octree_dim*2;
		while(p.y<0)p.y+=octree_dim*2;
		while(p.z<0)p.z+=octree_dim*2;
		while(p.x>=octree_dim*2)p.x-=octree_dim*2;
		while(p.y>=octree_dim*2)p.y-=octree_dim*2;
		while(p.z>=octree_dim*2)p.z-=octree_dim*2;
		pos=p/16.0;
	}

	ocl_begin_all_kernels();

	// init
	if(frame<2)
	{
		//ocl_memset(mem_screenbuffer, 0, 0xffffff00, res_x*res_y*4);
		ocl_memset(mem_screenbuffer, 0, 0xffffff00, res_x*res_y*4*4);
	}

	/*fill*/
	ocl_memset(mem_screenbuffer,0,0xffffff00,res_x*res_y*4);

	vec4f v0=pos;//-pos0;//m_delta*vec4f(0,0,0,0);
	vec4f vx=m*vec4f(1,0,0,0);
	vec4f vy=m*vec4f(0,1,0,0);
	vec4f vz=m*vec4f(0,0,1,0);

	int t[100];int t_cnt=0;
//	if(frame==0)printf("%d:project\n",t_cnt);
//	t[t_cnt++]=timeGetTime();

	// raycasted part
	int numpixels=WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX;
	static cl_mem mem_x=ocl_malloc(4*numpixels); 
	static cl_mem mem_y=ocl_malloc(4*numpixels); 
	static cl_mem mem_z=ocl_malloc(4*numpixels); 
	ocl_memset(mem_z,0,0xffffffff,res_x*res_y*4);

	/*pro*/
	if(1)
	loopi(0,2)
	// todo more buffers
	{
		int buffer_src_ofs = (i+1)*res_x*res_y;

		static cl_kernel kernel_proj = ocl_get_kernel("raycast_proj");
		ocl_begin(&kernel_proj,res_x,res_y,16,16);
		ocl_param(sizeof(cl_mem), (void *) &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), (void *) &mem_backbuffer);
		ocl_param(sizeof(cl_mem), (void *) &mem_x);
		ocl_param(sizeof(cl_mem), (void *) &mem_y);
		ocl_param(sizeof(cl_mem), (void *) &mem_z);
		ocl_param(sizeof(cl_int), (void *) &res_x );
		ocl_param(sizeof(cl_int), (void *) &res_y );
		ocl_param(sizeof(cl_int), (void *) &frame );
		ocl_param(sizeof(cl_int), (void *) &buffer_src_ofs ); // buffer
		ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
		ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
		ocl_end();
	}

	/*fillhole*/
//	if(frame==0)printf("%d:fillhole\n",t_cnt);
//	t[t_cnt++]=timeGetTime();

	// slow !!! fix
	if(0)
	if((frame&3)==0)
	{		
		static cl_kernel kernel_fillhole = ocl_get_kernel("raycast_fillhole");
		ocl_begin(&kernel_fillhole,res_x,res_y,16,16);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_mem), &mem_x);
		ocl_param(sizeof(cl_mem), &mem_y);
		ocl_param(sizeof(cl_mem), &mem_z);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &frame);
		ocl_end();		
	}


//	if(frame==0)printf("%d:raycast window\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	{
		m.transpose();//.invert_simpler();
		vec4f vx=m*vec4f(1,0,0,0);
		vec4f vy=m*vec4f(0,1,0,0);
		vec4f vz=m*vec4f(0,0,1,0);
		m.transpose();//.invert_simpler();

		vec3f a_leftTop,a_dx,a_dy;
		calcViewPlaneGradients(	res_x,res_y,  side,up,front,a_leftTop,a_dx,a_dy);

		if(0)
		{
			int add_x=(res_x/2)*(frame&1);
			int add_y=(res_y/2)*((frame>>1)&1);
			static cl_kernel kernel = ocl_get_kernel("raycast_fine");
			ocl_begin(&kernel,res_x/4,res_y/4,16,16);
			ocl_param(sizeof(cl_mem), &mem_screenbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), (void *) &mem_octree);
			ocl_param(sizeof(cl_int), (void *) &octree_root_normal);
			ocl_param(sizeof(cl_int), (void *) &res_x );
			ocl_param(sizeof(cl_int), (void *) &res_y );
			ocl_param(sizeof(cl_int), (void *) &frame );
			ocl_param(sizeof(cl_int), (void *) &add_x );
			ocl_param(sizeof(cl_int), (void *) &add_y );
			ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
			ocl_param(sizeof(cl_float)*1, (void *) &fovx );
			ocl_param(sizeof(cl_float)*1, (void *) &fovy );
			ocl_end();
		}
	}

	// -- count holes -- //
//	if(frame==0)printf("%d:count holes\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	
	int idbuf_size=0;
	static cl_mem mem_idbuffer=ocl_malloc(
		( WINDOW_WIDTH_MAX*WINDOW_HEIGHT_MAX+
		 (WINDOW_WIDTH_MAX/16)*(WINDOW_HEIGHT_MAX/16))*4);

	{
		static cl_kernel kernel_counthole = ocl_get_kernel("raycast_counthole");
		ocl_begin(&kernel_counthole,res_x/16,res_y/16,16,16);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_mem), &mem_idbuffer);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &frame);
		ocl_end();
	}

	// -- sum ids -- //
//	if(frame==0)printf("%d:sum ids\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	{
		static cl_kernel kernel = ocl_get_kernel("raycast_sumids");
		ocl_begin(&kernel,1,1,1,1);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_mem), &mem_idbuffer);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &frame);
		ocl_end();

		ocl_copy_to_host(&idbuf_size,mem_idbuffer,4);
		//printf("\nnum_pixels:%d\n",idbuf_size);
	}
	
	// -- write ids -- //
//	if(frame==0)printf("%d:write ids\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	{
		static cl_kernel kernel = ocl_get_kernel("raycast_writeids");
		ocl_begin(&kernel,res_x/16,res_y/16,16,16);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_mem), &mem_idbuffer);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &frame);
		ocl_end();		
	}
	
	// -- fill empty pass -- //
//	if(frame==0)printf("%d:fill empty pass\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	//if((frame&7)==0)
	{
		m.transpose();//.invert_simpler();
		vec4f vx=m*vec4f(1,0,0,0);
		vec4f vy=m*vec4f(0,1,0,0);
		vec4f vz=m*vec4f(0,0,1,0);

		vec3f a_leftTop,a_dx,a_dy;
		calcViewPlaneGradients(	res_x,res_y,  side,up,front,a_leftTop,a_dx,a_dy);

		static cl_kernel kernel_fine = ocl_get_kernel("raycast_holes");

		if(1)
		if(idbuf_size>0)
		{
			ocl_begin(&kernel_fine,idbuf_size,1,256,1);
			ocl_param(sizeof(cl_mem), &mem_screenbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), (void *) &mem_octree);
			ocl_param(sizeof(cl_mem), (void *) &mem_bvh_nodes);
			ocl_param(sizeof(cl_mem), (void *) &mem_bvh_childs);
			ocl_param(sizeof(cl_mem), (void *) &mem_stack);
			ocl_param(sizeof(cl_mem), (void *) &mem_idbuffer);
			ocl_param(sizeof(cl_int), (void *) &octree_root_normal);
			ocl_param(sizeof(cl_int), (void *) &res_x );
			ocl_param(sizeof(cl_int), (void *) &res_y );
			ocl_param(sizeof(cl_int), (void *) &frame );
			ocl_param(sizeof(cl_int), (void *) &idbuf_size );
			ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
			ocl_param(sizeof(cl_float)*1, (void *) &fovx );
			ocl_param(sizeof(cl_float)*1, (void *) &fovy );
			ocl_end();
		}

		if(1)
		{
			int add_x=(res_x/8)*(frame&7);
			int add_y=(res_y/4)*((frame>>3)&3);
			static cl_kernel kernel = ocl_get_kernel("raycast_fine_2");
			ocl_begin(&kernel,res_x/8,res_y/4,16,16);
			ocl_param(sizeof(cl_mem), &mem_screenbuffer);
			ocl_param(sizeof(cl_mem), &mem_backbuffer);
			ocl_param(sizeof(cl_mem), (void *) &mem_octree);
			ocl_param(sizeof(cl_int), (void *) &octree_root_normal);
			ocl_param(sizeof(cl_int), (void *) &res_x );
			ocl_param(sizeof(cl_int), (void *) &res_y );
			ocl_param(sizeof(cl_int), (void *) &frame );
			ocl_param(sizeof(cl_int), (void *) &add_x );
			ocl_param(sizeof(cl_int), (void *) &add_y );
			ocl_param(sizeof(cl_float)*4, (void *) &pos.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_leftTop.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &a_dy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &v0.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vx.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vy.x );
			ocl_param(sizeof(cl_float)*4, (void *) &vz.x );
			ocl_param(sizeof(cl_float)*1, (void *) &fovx );
			ocl_param(sizeof(cl_float)*1, (void *) &fovy );
			ocl_end();
		}
	}
		
	/*copy*/
//	if(frame==0)printf("%d:copy\n",t_cnt);
//	t[t_cnt++]=timeGetTime();
	//if((frame&1)==0)
	{
		int target=2;//((frame>>4)%2)+1;
		ocl_memcpy(
			mem_screenbuffer,size_col*target,
			mem_screenbuffer,size_col*0,
			size_col);
		
		ocl_memcpy(
			mem_backbuffer,size_xyz*target,
			mem_backbuffer,size_xyz*0,
			size_xyz);
	}

	//if(frame==0)printf("%d:fillhole 2\n",t_cnt);
	//t[t_cnt++]=timeGetTime();

	/*fillhole 2*/
	if(1)
	loopi(0,1)
	{		
		static cl_kernel kernel_fillhole2 = ocl_get_kernel("raycast_fillhole2");
		ocl_begin(&kernel_fillhole2,res_x,res_y,16,16);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_backbuffer);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_param(sizeof(cl_int), &frame);
		ocl_end();		
	}
	// -- color -- //

	ocl_pbo_begin(mem_screenbuffer_tex);

	//if(frame==0)printf("%d:color\n",t_cnt);
	//t[t_cnt++]=timeGetTime();
	{
		static cl_kernel kernel = ocl_get_kernel("raycast_colorize");
		ocl_begin(&kernel,res_x,res_y,16,16);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer);
		ocl_param(sizeof(cl_mem), &mem_screenbuffer_tex);
		ocl_param(sizeof(cl_int), &res_x);
		ocl_param(sizeof(cl_int), &res_y);
		ocl_end();	
	}
	ocl_end_all_kernels();

	ogl_check_error();
	ocl_pbo_end(mem_screenbuffer_tex);
	
	// -- screen pass -- //

//	if(frame==0)printf("%d:download tex 2\n",t_cnt);
//	t[t_cnt++]=timeGetTime();

    // download texture from PBO
	static int tex_screen=ogl_tex_new(2048,2048,GL_NEAREST);
	glEnable(GL_TEXTURE_2D);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, mem_pbo_out);
    glBindTexture(GL_TEXTURE_2D, tex_screen);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 
                    res_x, res_y, 
                    GL_BGRA, GL_UNSIGNED_BYTE, NULL);
	ogl_check_error();
	

    glBegin( GL_QUADS );  
	float tx=float(res_x)/2048.0;
	float ty=float(res_y)/2048.0;
	glColor3f(1,1,1);
    glTexCoord2f(0 ,  0); glVertex3f(-1.0, -1.0, 1.0);
    glTexCoord2f(tx,  0); glVertex3f( 1.0, -1.0, 1.0);
    glTexCoord2f(tx, ty); glVertex3f( 1.0,  1.0, 1.0);
    glTexCoord2f(0 , ty); glVertex3f(-1.0,  1.0, 1.0);
    glEnd();

	glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
	ogl_check_error();

	
	// time
	{
		t[t_cnt++]=timeGetTime();
		static float tt[100];
		if(frame==0)loopi(0,t_cnt)tt[i]=0;
		loopi(0,t_cnt-1)tt[i]=tt[i]*0.9+0.1*float(t[i+1]-t[i]);
		if((frame&15)==0)
		{
			float s=0;
			loopi(0,t_cnt-1)
			{
				printf("%d:%1.2f ",i,tt[i]);
				s+=tt[i];
			}
			printf("S:%1.2f\r",s);
			fpscl=1000.0/s;
		}
	}
	
	return;
	
	Bmp bmp(res_x,res_y,32,0);

	if(1)
	clEnqueueReadBuffer(ocl_command_queue    /* command_queue */,
                    mem_screenbuffer              /* buffer */,
                    CL_TRUE             /* blocking_read */,
                    0              /* offset */,
                    res_x*res_y*4              /* cb */, 
                    (void *)bmp.data              /* ptr */,
                    0             /* num_events_in_wait_list */,
                    0    /* event_wait_list */,
                    0          /* event */) ;

	bmp.save("../test.bmp");
}
void raycast_exit()
{
	ocl_pbo_unmap(mem_screenbuffer);
	CL_CHECK(clReleaseMemObject(mem_screenbuffer));

	ocl_exit();
}
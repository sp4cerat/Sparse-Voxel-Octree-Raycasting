class FBO {

	public:

	enum Type { COLOR=1 , DEPTH=2 }; // Bits

	int color_tex;
	int color_bpp;
	int depth_tex;
	int depth_bpp;
	Type type;

	int width;
	int height;

	int tmp_viewport[4];

	FBO (int texWidth,int texHeight, int format=GL_RGBA,int datatype=GL_UNSIGNED_BYTE)
	{
		color_tex = -1;
		depth_tex = -1;
		fbo = -1;
		dbo = -1;
		init (texWidth, texHeight,format,datatype);
	}

	void clear ()
	{		
		if(color_tex!=-1)
		{
			// destroy objects
			glDeleteRenderbuffersEXT(1, &dbo);
			glDeleteTextures(1, (GLuint*)&color_tex);
			glDeleteTextures(1, (GLuint*)&depth_tex);
			glDeleteFramebuffersEXT(1, &fbo);
		}
	}

	void enable(int customwidth=-1,int customheight=-1)
	{
		//glBindTexture(GL_TEXTURE_2D, 0);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, dbo);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color_tex, 0);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
			GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, depth_tex, 0);

		glGetIntegerv(GL_VIEWPORT, tmp_viewport);

		if(customwidth>0)
			glViewport(0, 0, customwidth, customheight);		// Reset The Current Viewport And Perspective Transformation
		else
			glViewport(0, 0, width, height);		// Reset The Current Viewport And Perspective Transformation

		//glMatrixMode(GL_PROJECTION);
		//glPushMatrix();
		//glLoadIdentity();
		//gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);
		//glMatrixMode(GL_MODELVIEW);

	}

	void disable()
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
		glViewport(
			tmp_viewport[0],
			tmp_viewport[1],
			tmp_viewport[2],
			tmp_viewport[3]);
		//glMatrixMode(GL_PROJECTION);
		//glPopMatrix();
		//glMatrixMode(GL_MODELVIEW);
	}

	void init (int texWidth,int texHeight, int format=GL_RGBA,int datatype=GL_UNSIGNED_BYTE)//,Type type = Type(COLOR | DEPTH),int color_bpp=32,int depth_bpp=24)
	{
	//	clear ();
		this->width = texWidth;
		this->height = texHeight;

		glGenFramebuffersEXT(1, &fbo); 
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);    
		get_error();

			// init texture
			glGenTextures(1, (GLuint*)&color_tex);
			glBindTexture(GL_TEXTURE_2D, color_tex);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
			//	texWidth, texHeight, 0, 
			//	GL_RGBA, GL_FLOAT, NULL);

			glTexImage2D(GL_TEXTURE_2D, 0, format, texWidth, texHeight, 0,
				    format, datatype, NULL);

			//GL_TEXTURE_2D,GL_RGBA, bmp.width, bmp.height,
			//	/*GL_RGBA*/GL_BGRA_EXT, GL_UNSIGNED_BYTE, bmp.data );

			get_error();
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glFramebufferTexture2DEXT(
				GL_FRAMEBUFFER_EXT, 
				GL_COLOR_ATTACHMENT0_EXT, 
				GL_TEXTURE_2D, color_tex, 0);
			get_error();

		glGenRenderbuffersEXT(1, &dbo);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, dbo);

			glGenTextures(1, (GLuint*)&depth_tex);
			glBindTexture(GL_TEXTURE_2D, depth_tex);
			//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 
			//	texWidth, texHeight, 0, 
			//	GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			get_error();
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTexParameteri (GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			/*
			//Use generated mipmaps if supported
			if(GLEE_SGIS_generate_mipmap)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, true);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
			}

			//Use maximum anisotropy if supported
			if(GLEE_EXT_texture_filter_anisotropic)
			{
				GLint maxAnisotropy=1;
				glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
			}
			*/

			glTexImage2D (GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, 
				texWidth, texHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT/*GL_UNSIGNED_INT*/, NULL);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, 
								GL_TEXTURE_2D, depth_tex, 0);

			get_error();
			glBindTexture(GL_TEXTURE_2D, 0);// don't leave this texture bound or fbo (zero) will use it as src, want to use it just as dest GL_DEPTH_ATTACHMENT_EXT

		get_error();

		check_framebuffer_status();
	    
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
	}

	private:

	GLuint fbo; // frame buffer object ref
	GLuint dbo; // depth buffer object ref

	void get_error()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) 
		{
			printf("GL FBO Error: %s\n",gluErrorString(err));
			printf("Programm Stopped!\n");
			while(1);;
		}
	}

	void check_framebuffer_status()
	{
		GLenum status;
		status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		switch(status) {
			case GL_FRAMEBUFFER_COMPLETE_EXT:
				return;
				break;
			case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
				printf("Unsupported framebuffer format\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
				printf("Framebuffer incomplete, missing attachment\n");
				break;
//			case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
//				printf("Framebuffer incomplete, duplicate attachment\n");
//				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
				printf("Framebuffer incomplete, attached images must have same dimensions\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
				printf("Framebuffer incomplete, attached images must have same format\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				printf("Framebuffer incomplete, missing draw buffer\n");
				break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
				printf("Framebuffer incomplete, missing read buffer\n");
				break;
			case 0:
				printf("Not ok but trying...\n");
				return;
				break;
			default:;
				printf("Framebuffer error code %d\n",status);
				break;
		};
		printf("Programm Stopped!\n");
		while(1)Sleep(100);;
	}
};

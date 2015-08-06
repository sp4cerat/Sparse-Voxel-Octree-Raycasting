#define ogl_check_error() { \
	GLenum error = glGetError();\
    if( error != GL_NO_ERROR )\
		error_stop( "OpenGL Error: %s\n", gluErrorString( error ) ); }

#include "SDL/SDL_opengl.h"
#include "glsl.h"
#include "fbo.h"

void ogl_init()
{
    //Initialize Projection Matrix
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    //Initialize Modelview Matrix
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
	
	glEnable(GL_DEPTH_TEST);
    glClearColor(0, 0, 0, 1);

	if( glewInit() != GLEW_OK)error_stop("GLEW initialization failed\n");

	ogl_check_error();
}

int ogl_tex_new(unsigned int size_x, unsigned int size_y, int filter=GL_LINEAR,int repeat=GL_CLAMP_TO_EDGE,int type1=GL_RGBA,int type2=GL_RGBA,unsigned char* data=0)
{
	int id=0;

    // create a texture
    glGenTextures(1, (GLuint *)&id);
    glBindTexture(GL_TEXTURE_2D, id);

    // set basic parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    // buffer data
    glTexImage2D(GL_TEXTURE_2D, 0, type1, size_x, size_y, 0, type2, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

int ogl_tex_bmp(char * name, int filter=GL_LINEAR,int repeat=GL_CLAMP_TO_EDGE)
{
	Bmp bmp(name);
	return ogl_tex_new(bmp.width, bmp.height,filter,repeat, GL_RGB, GL_BGR_EXT,bmp.data);
}

GLuint ogl_pbo_new(int size)
{
	GLuint pbo;
    glGenBuffers(1, &pbo);
    glBindBuffer(GL_ARRAY_BUFFER, pbo);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
	return pbo;
}

void ogl_pbo_del(const GLuint pbo)
{
    glBindBuffer(GL_ARRAY_BUFFER,pbo);
    glDeleteBuffers(1, &pbo);
}

float ogl_read_z(int x, int y)
{
    GLint viewport[4];
    GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;
 
    glGetIntegerv( GL_VIEWPORT, viewport );
 
    winX = (float)x;
    winY = (float)viewport[3] - (float)y;

	glReadPixels( x, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );

	return winZ;
}
vec3f ogl_unproject(int x, int y, float winZ=-10)
{
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY;
    GLdouble posX, posY, posZ;
 
    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );
 
    winX = (float)x;
    winY = (float)viewport[3] - (float)y;

	if(winZ<-5) winZ=ogl_read_z(x,y);
 
    gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
 
    return vec3f(posX, posY, posZ);
}

void ogl_drawline(	float x0,float y0,float z0,float x1,float y1,float z1)
{
	glBegin(GL_LINES);
	glVertex3f(x0,y0,z0);
	glVertex3f(x1,y1,z1);
	glEnd();
}
void ogl_drawline( vec3f p1, vec3f p2 )
{
	ogl_drawline(p1.x,p1.y,p1.z,p2.x,p2.y,p2.z);
}

void ogl_drawquad(	float x0,float y0,float x1,float y1,
					float tx0=0,float ty0=0,float tx1=0,float ty1=0)
{
	glBegin(GL_QUADS);
	glTexCoord2f(tx0,ty0);glVertex2f(x0,y0);
	glTexCoord2f(tx1,ty0);glVertex2f(x1,y0);
	glTexCoord2f(tx1,ty1);glVertex2f(x1,y1);
	glTexCoord2f(tx0,ty1);glVertex2f(x0,y1);
	glEnd();
}
void ogl_drawlinequad(float x0,float y0,float x1,float y1)
{
	glBegin(GL_LINE_LOOP);
	glVertex2f(x0,y0);
	glVertex2f(x1,y0);
	glVertex2f(x1,y1);
	glVertex2f(x0,y1);
	glEnd();
}

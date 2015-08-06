//#define WGL_EXT_swap_control
#include "core.h"
#include "libs.h" // linker libs
#include "math.h"
#include "SDL/SDL.h"
#include "SDL/SDL_opengl.h"
//#include "SDL/SDL_audio.h"
#include "SDL/SDL_opengl.h"
//#include "SDL/SDL_mixer.h"
#include "SDL/SDL_syswm.h"

#include "win.h"

#include <vector>
#include "Bmp.h"
#include "error.h"
#include "file.h"

float raycast_width  =1280;//1280;//1280;//1920;// 1280;//1920;//1280;///2;
float raycast_height =720;//720;//720;//1024;// 720;// 1024;//768;///2;

float fps=1.0;
float fpscl=1.0;

#include "mathlib/Vector.h"
#include "mathlib/Matrix.h"

#include "ui.h"
#include "ocl.h"
#include "ogl.h"
#include "mmsystem.h"


#define clamp(a_,b_,c_) { if(a_<b_) a_=b_;if(a_>c_) a_=c_;}

int FRAME=0;
int FRAMETIME=0;

char  KEYTAB[512];

float MOUSE_X=0;
float MOUSE_Y=0;
float MOUSE_DX=0;
float MOUSE_DY=0;
float MOUSE_WHEEL=0;
float MOUSE_DWHEEL=0; // delta wheel
char  MOUSE_BUTTON[32];
char  MOUSE_BUTTON_DOWN[32];
char  MOUSE_BUTTON_UP  [32];

int WINDOW_WIDTH_MAX  = 2048;//raycast_width;//1920;
int WINDOW_HEIGHT_MAX = 1080;//raycast_height;//1080;
int WINDOW_WIDTH  =raycast_width;//1920/2;
int WINDOW_HEIGHT =raycast_height;//1080/2;
int WINDOW_BPP = 32;
SDL_Surface *sdl_surf=0;

int ACTIVE_SCREEN=1;

#include "octree/octree.h"
#include "octree/tree.h"
#include "raycast.h"

void reshape(GLsizei w,GLsizei h)
{
  glViewport(0,0,w,h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1.0,1.0,-1.0,1.0,-1.0,1.0);
  glMatrixMode(GL_MODELVIEW);
  ui.reshape(w, h); 
}

void sdl_init()
{
    //Initialize SDL
    if( SDL_Init( SDL_INIT_EVERYTHING ) < 0 )
    {
        error_stop("SDL_Init failed\n");
    }

    //Create Window
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

	//SDL_Window *mainwindow = SDL_CreateWindow("huhu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      //  512, 512, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	
	sdl_surf=SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_BPP, SDL_HWSURFACE | SDL_OPENGL | SDL_RESIZABLE );
    if( sdl_surf == NULL )
    {
        error_stop("SDL_SetVideoMode failed\n");
    }

    //Enable unicode
    SDL_EnableUNICODE( SDL_TRUE );
		
    //Set caption
    SDL_WM_SetCaption( "Voxel Master", NULL );
}

void ui_init()
{
	if (!ui.init())
	{
        error_stop("UI initialization failed\n");
	}
	ui.reshape(WINDOW_WIDTH, WINDOW_HEIGHT);
}

void handleKeys( int sym, int key, int x, int y )
{
	//printf("key %d sym %d\n",key,sym);
    if( sym == SDLK_ESCAPE ) exit(0);
    if( sym == SDLK_F2 )
    {
		static RECT r;
		static char fullscreen=0;
		static HWND hwnd =FindWindow(NULL, "Voxel Master");

		if(!fullscreen)
		{
			GetWindowRect(hwnd,&r);
			SetWindowLong(hwnd,GWL_STYLE, WS_VISIBLE | WS_EX_TOPMOST | WS_POPUP);
			ShowWindow(hwnd,SW_MAXIMIZE);
		}
		else
		{
			SetWindowLong(hwnd,GWL_STYLE, WS_VISIBLE | WS_EX_TOPMOST | WS_POPUP | WS_CAPTION |WS_SYSMENU|  WS_THICKFRAME);
			MoveWindow(hwnd,r.top,r.left,r.right-r.left,r.bottom-r.top,0);
		}
		fullscreen^=1;
		return;
    }
	ui.keyboard(key, x, y);
}

void main_fps()
{
	// calc fps
	static int time0=timeGetTime(); 
	int time1=FRAMETIME=timeGetTime();
	float delta=float(int(time1-time0));
	time0=time1;
	if(delta>0)	fps=fps*0.9+100.0/delta;
}

void update_mouse_keyb()
{
	static float mx=MOUSE_X;
	static float my=MOUSE_Y;
	MOUSE_DX=MOUSE_X-mx;
	MOUSE_DY=MOUSE_Y-my;
	mx=MOUSE_X;
	my=MOUSE_Y;

	MOUSE_DWHEEL=MOUSE_WHEEL;
	MOUSE_WHEEL=0;

	static int mb[32]={0,0,0,0,0};

	loopi(0,32)
	{
		MOUSE_BUTTON_DOWN[i]=( MOUSE_BUTTON[i]) & (!mb[i]);
		MOUSE_BUTTON_UP  [i]=(!MOUSE_BUTTON[i]) & ( mb[i]);
		mb[i]=MOUSE_BUTTON[i];
	}

	if(KEYTAB[SDLK_F3]) ACTIVE_SCREEN=0;
	if(KEYTAB[SDLK_F4]) ACTIVE_SCREEN=1;
}

void render()
{
	main_fps();
	update_mouse_keyb();
	
	raycast_draw(raycast_width,raycast_height);

	//doUI();

    //Update screen
	wglSwapIntervalEXT(0);
    SDL_GL_SwapBuffers();
	FRAME++;
}

int main( int argc, char *argv[] )
{
	// Init > Begin
	memset(MOUSE_BUTTON,0,32);
	memset(KEYTAB,0,512);

	sdl_init();
    ogl_init();
	ocl_init();
	ui_init();
	raycast_init();

	// Init > End
	
	//SDL_ShowCursor(0);

	//mainloop
	bool run=1;
	while(run)
	{
        //While there are events to handle
		static SDL_Event event;
		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT ) run=0;
            if( event.type == SDL_KEYUP )
            {
				KEYTAB[event.key.keysym.sym]=0;
			}
            if( event.type == SDL_KEYDOWN )
            {
				KEYTAB[event.key.keysym.sym]=1;
                //Handle keypress with current mouse position
                int x = 0, y = 0;
                SDL_GetMouseState( &x, &y );
				handleKeys( event.key.keysym.sym, event.key.keysym.unicode, x, y );
            }
            if( event.type == SDL_MOUSEMOTION )
            {
                ui.mouseMotion(event.motion.x, event.motion.y);
				MOUSE_X=event.motion.x;
				MOUSE_Y=event.motion.y;
            }
            if( event.type == SDL_MOUSEBUTTONDOWN )
            {
				MOUSE_BUTTON[event.button.button-1]=1;
				ui.mouse(event.button.button-1, GLUT_DOWN, event.motion.x, event.motion.y);

				if( event.button.button == SDL_BUTTON_WHEELUP )
				{
					MOUSE_WHEEL-=1;
				}

			}
            if( event.type == SDL_MOUSEBUTTONUP )
            {
				MOUSE_BUTTON[event.button.button-1]=0;
				ui.mouse(event.button.button-1, GLUT_UP, event.motion.x, event.motion.y);

				if( event.button.button == SDL_BUTTON_WHEELDOWN )
				{
					MOUSE_WHEEL+=1;
				}		
			
			}
			
            if( event.type == SDL_VIDEORESIZE )
			{        
				SDL_FreeSurface(sdl_surf);
				sdl_surf = SDL_SetVideoMode(event.resize.w,event.resize.h,32,SDL_HWSURFACE |SDL_OPENGL|SDL_RESIZABLE);
				reshape(static_cast<GLsizei>(event.resize.w),static_cast<GLsizei>(event.resize.h));
				WINDOW_WIDTH =event.resize.w;
				WINDOW_HEIGHT=event.resize.h;
			}			
		}
        //Render frame
        render();

	}

	//Clean up
	raycast_exit();
    SDL_Quit();

	return 0;
}


/*
HWND FindMyTopMostWindow()
{
    DWORD dwProcID = GetCurrentProcessId();
    HWND hWnd = GetTopWindow(GetDesktopWindow());
    while(hWnd)
    {
        DWORD dwWndProcID = 0;
        GetWindowThreadProcessId(hWnd, &dwWndProcID);
        if(dwWndProcID == dwProcID)
		{
			MessageBox(hWnd, "found", "found", MB_OK | MB_ICONINFORMATION);
            return hWnd;            
		}
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT);
    }
    return NULL;
 }
*/

//
// nvGlutWidgets
//
//  Adaptor classes to integrate the nvWidgets UI library with the GLUT windowing
// toolkit. The adaptors convert native GLUT UI data to native nvWidgets data. All
// adaptor classes are implemented as in-line code in this header. The adaptor
// defaults to using the standard OpenGL painter implementation.
//
// Author: Ignacio Castano, Samuel Gateau, Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "nvGlutWidgets.h"

#include <stdlib.h> // exit, needed by GL/glut.h
#include <GL/glew.h>
#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

using namespace nv;


bool GlutUIContext::init()
{
	// @@ Remove glew dependency. Load extensions explicitely and pass pointers to painter.

	if (glewInit() != GLEW_OK)
	{
		return false;
	}

    if (!glewIsSupported(
        "GL_VERSION_2_0 "
        "GL_ARB_vertex_program "
        "GL_ARB_fragment_program "
        ))
    {
        return false;
    }

	return true;
}

void GlutUIContext::mouse(int button, int state, int modifier, int x, int y)
{ 
	int modifierMask = 0;

	if ( button == GLUT_LEFT_BUTTON) button = MouseButton_Left;
	else if ( button == GLUT_MIDDLE_BUTTON) button = MouseButton_Middle;
	else if ( button == GLUT_RIGHT_BUTTON) button = MouseButton_Right;

	if ( modifier & GLUT_ACTIVE_ALT) modifierMask |= ButtonFlags_Alt;
	if ( modifier & GLUT_ACTIVE_SHIFT) modifierMask |= ButtonFlags_Shift;
	if ( modifier & GLUT_ACTIVE_CTRL) modifierMask |= ButtonFlags_Ctrl;

	if ( state == GLUT_DOWN) state = 1; else state = 0;

	UIContext::mouse( button, state, modifierMask, x, y);
}
		
void GlutUIContext::mouse(int button, int state, int x, int y)
{
	mouse(button, state, 0/*glutGetModifiers()*/, x, y);
}

void GlutUIContext::specialKeyboard(int k, int x, int y)
{
	UIContext::keyboard( translateKey(k), x, y);
}

unsigned char GlutUIContext::translateKey( int k )
{
	switch (k)
	{
	case GLUT_KEY_F1 :
		return Key_F1;
	case GLUT_KEY_F2 :
		return Key_F2;
	case GLUT_KEY_F3 :
		return Key_F3;
	case GLUT_KEY_F4 :
		return Key_F4;
	case GLUT_KEY_F5 :
		return Key_F5;
	case GLUT_KEY_F6 :
		return Key_F6;
	case GLUT_KEY_F7 :
		return Key_F7;
	case GLUT_KEY_F8 :
		return Key_F8;
	case GLUT_KEY_F9 :
		return Key_F9;
	case GLUT_KEY_F10 :
		return Key_F10;
	case GLUT_KEY_F11 :
		return Key_F11;
	case GLUT_KEY_F12 :
		return Key_F12;
	case GLUT_KEY_LEFT :
		return Key_Left;
	case GLUT_KEY_UP :
		return Key_Up;
	case GLUT_KEY_RIGHT :
		return Key_Right;
	case GLUT_KEY_DOWN :
		return Key_Down;
	case GLUT_KEY_PAGE_UP :
		return Key_PageUp;
	case GLUT_KEY_PAGE_DOWN :
		return Key_PageDown;
	case GLUT_KEY_HOME :
		return Key_Home;
	case GLUT_KEY_END :
		return Key_End;
	case GLUT_KEY_INSERT :
		return Key_Insert;
	default:
		return 0;
	} 
}


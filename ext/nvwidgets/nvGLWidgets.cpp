//
// nvGLWidgets.cpp - User Interface library specialized for OpenGL
//
//
// Author: Ignacio Castano, Samuel Gateau, Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "nvGLWidgets.h"

#include <math.h> // sqrtf
#include <stdlib.h> // exit (required by glut.h)
#include <string.h> // strlen

#include <GL/glew.h>

#ifdef __APPLE_CC__
#include <GLUT/glut.h>
#else
#include <GL/glut.h> // glutBitmapWidth, glutBitmapCharacter
#endif

#define NV_REPORT_COMPILE_ERRORS
#include <nvglutils/nvShaderUtils.h>


using namespace nv;

#define norm255( i ) ( (float) ( i ) / 255.0f )

enum Color
{
    cBase = 0,
    cBool = 4,
    cOutline = 8,
    cFont = 12,
    cFontBack = 16,
    cTranslucent = 20,
    cNbColors = 24,
};

const static float s_colors[cNbColors][4] =
{
	// cBase
	{ norm255(89), norm255(89), norm255(89), 0.7f },
	{ norm255(166), norm255(166), norm255(166), 0.8f },
	{ norm255(212), norm255(228), norm255(60), 0.5f },
	{ norm255(227), norm255(237), norm255(127), 0.5f },

    // cBool
    { norm255(99), norm255(37), norm255(35), 1.0f },
    { norm255(149), norm255(55), norm255(53), 1.0f },
    { norm255(212), norm255(228), norm255(60), 1.0f },
    { norm255(227), norm255(237), norm255(127), 1.0f },

    // cOutline
    { norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },

    // cFont
    { norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },
	{ norm255(255), norm255(255), norm255(255), 1.0f },

    // cFontBack
    { norm255(79), norm255(129), norm255(189), 1.0 },
    { norm255(79), norm255(129), norm255(189), 1.0 },
    { norm255(128), norm255(100), norm255(162), 1.0 },
    { norm255(128), norm255(100), norm255(162), 1.0 },
    
    // cTranslucent
    { norm255(0), norm255(0), norm255(0), 0.0 },
	{ norm255(0), norm255(0), norm255(0), 0.0 },
	{ norm255(0), norm255(0), norm255(0), 0.0 },
	{ norm255(0), norm255(0), norm255(0), 0.0 },
};
    


template <typename T> T max(const T & a, const T & b)
{
	if (a > b) return a;
	return b;
}

const char* cWidgetVSSource = {
    "#version 120\n\
    \n\
    void main()\n\
    {\n\
		gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
        gl_TexCoord[0] = gl_MultiTexCoord0;\n\
    }\n\
    "};

// @@ IC: Use standard GLSL. Do not initialize uniforms.
	
const char* cWidgetFSSource = {
    "#version 120\n\
    uniform vec4 fillColor /*= vec4( 1.0, 0.0,0.0,1.0)*/;\n\
    uniform vec4 borderColor /*= vec4( 1.0, 1.0,1.0,1.0)*/;\n\
    uniform vec2 zones;\n\
    \n\
    void main()\n\
    {\n\
        float doTurn = float(gl_TexCoord[0].y > 0);\n\
        float radiusOffset = doTurn * abs( gl_TexCoord[0].z );\n\
        float turnDir = sign( gl_TexCoord[0].z );\n\
        vec2 uv = vec2(gl_TexCoord[0].x + turnDir*radiusOffset, gl_TexCoord[0].y);\n\
        float l = abs( length(uv) - radiusOffset );\n\
        float a = clamp( l - zones.x, 0.0, 2.0);\n\
        float b = clamp( l - zones.y, 0.0, 2.0);\n\
        b = exp2(-2.0*b*b);\n\
        gl_FragColor = ( fillColor * b + (1.0-b)*borderColor );\n\
        gl_FragColor.a *= exp2(-2.0*a*a);\n\
    }\n\
    "};

const char* cTexViewWidgetFSSource = {
    "#version 120\n\
    uniform float mipLevel /*= 0*/;\n\
    uniform float texelScale /*= 1.0*/;\n\
    uniform float texelOffset /*= 0.0*/;\n\
    uniform ivec4 texelSwizzling /*= ivec4( 0, 1, 2, 3)*/;\n\
    uniform sampler2D samp;\n\
    \n\
    void main()\n\
    {\n\
        vec4 texel;\n\
        if (mipLevel > 0)\n\
            texel = texture2DLod( samp, gl_TexCoord[0].xy, mipLevel);\n\
        else\n\
            texel = texture2D( samp, gl_TexCoord[0].xy);\n\
        texel = texel * texelScale + texelOffset;\n\
        gl_FragColor  = texel.x * vec4( texelSwizzling.x == 0, texelSwizzling.y == 0, texelSwizzling.z == 0, texelSwizzling.w == 0 );\n\
        gl_FragColor += texel.y * vec4( texelSwizzling.x == 1, texelSwizzling.y == 1, texelSwizzling.z == 1, texelSwizzling.w == 1 );\n\
        gl_FragColor += texel.z * vec4( texelSwizzling.x == 2, texelSwizzling.y == 2, texelSwizzling.z == 2, texelSwizzling.w == 2 );\n\
        gl_FragColor += texel.w * vec4( texelSwizzling.x == 3, texelSwizzling.y == 3, texelSwizzling.z == 3, texelSwizzling.w == 3 );\n\
    }\n\
    "};


//*************************************************************************
//* GLUIPainter

GLUIPainter::GLUIPainter():
    UIPainter(),
    m_setupStateDL(0),
    m_restoreStateDL(0),
    m_textListBase(0),
    m_foregroundDL(0),
    m_widgetProgram(0),
    m_fillColorUniform(0),
    m_borderColorUniform(0),
    m_zonesUniform(0),
    m_textureViewProgram(0),
    m_texMipLevelUniform(0),
    m_texelScaleUniform(0),
    m_texelOffsetUniform(0),
    m_texelSwizzlingUniform(0)
{
}

int getWidgetMargin() 
{
    return 3 ; //2  ;
}
int getWidgetSpace() 
{
    return 2 ;
}

int getAutoWidth() 
{
    return 100 ;
}

int getAutoHeight() 
{
    return 12+4;
}

int GLUIPainter::getCanvasMargin() const
{
//    return 5;
    return 3;//5;
}

int GLUIPainter::getCanvasSpace() const
{
    return 3;//5;
}

int GLUIPainter::getFontHeight() const
{
    return 12+4 ;
}

int GLUIPainter::getTextLineWidth(const char * text) const
{
    int w = 0;
    while(*text != '\0' && *text != '\n')
    {
        w += glutBitmapWidth( GLUT_BITMAP_HELVETICA_12, *text);
        text++;
    }
    w += 2;
    return w;
}

int GLUIPainter::getTextSize(const char * text, int& nbLines) const
{
    int w = 0;
    int wLine = 0;
    nbLines = 1;
    while(*text != '\0')
    {
        if (*text != '\n')
        {
            wLine += glutBitmapWidth( GLUT_BITMAP_HELVETICA_12, *text);
        }
        else
        {
            nbLines++;
            w = max(w, wLine);
            wLine = 0;
        }

        text++;
    }
    w = max(w, wLine) + 2;

    return w;
}

int GLUIPainter::getTextLineWidthAt(const char * text, int charNb) const
{
    int w = 1;
    for (int i = 0; i < charNb; i++)
    {
        if (*text != '\0' && *text != '\n')
        {
            w += glutBitmapWidth( GLUT_BITMAP_HELVETICA_12, *text);
        }
        else
        {
            return w+1;
        }
        text++;
    }

    return w;
 }

int GLUIPainter::getPickedCharNb(const char * text, const Point& at) const
{
    const char * textstart = text;
    
    int w = 1;
    if ( at.x < w ) return 0;

    while(*text != '\0' && *text != '\n')
    {
        
        w += glutBitmapWidth( GLUT_BITMAP_HELVETICA_12, *text);
        
        if ( at.x < w ) return (text - textstart); 

        text++;
    }

    return (text - textstart);
}

void GLUIPainter::drawFrame(const Rect & r, int margin, int /*style*/)
{
    drawRoundedRectOutline(Rect(r.x - margin, r.y - margin, r.w + 2*margin, r.h + 2*margin), nv::Point(margin, margin), cOutline);
}

Rect GLUIPainter::getLabelRect(const Rect & r, const char * text, Rect & rt, int& nbLines ) const
{
    Rect rect = r;
    rt.x = getWidgetMargin();
    rt.y = getWidgetMargin();

    // Eval Nblines and max line width anyway.
    rt.w = getTextSize(text, nbLines);

    if (rect.w == 0)
    {
        rect.w = rt.w + 2*rt.x;
    }
    else
    {
        rt.w = rect.w - 2*rt.x;
    }
    if (rect.h == 0)
    {
        rt.h = getFontHeight()*nbLines;
        rect.h = rt.h + 2*rt.y;
    }
    else
    {
        rt.h = rect.h - 2*rt.y;
    }
    return rect;
}

void GLUIPainter::drawLabel(const Rect & r, const char * text, const Rect & rt, const int& nbLines, bool /*isHover*/, int style)
{
    if (style > 0 )
        drawFrame( r, Point( rt.x, rt.y ), false, false, false );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h), text, nbLines);
}

Rect GLUIPainter::getLineEditRect(const Rect & r, const char * text, Rect & rt) const
{
    Rect rect = r;
    rt.x = getWidgetMargin();
    rt.y = getWidgetMargin();


    if (rect.w == 0)
    {
        rt.w = max(getTextLineWidth(text), 100);
        rect.w = rt.w + 2*rt.x;
    }
    else
    {
        rt.w = rect.w - 2*rt.x;
    }
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }
    else
    {
        rt.h = rect.h - 2*rt.y;
    }
    return rect;
}

void GLUIPainter::drawLineEdit(const Rect & r, const char * text, const Rect & rt, int caretPos, bool isSelected, bool /*isHover*/, int /*style*/)
{
    drawFrame( r, Point( rt.x, rt.y ), true, isSelected, false );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h), text, 1, caretPos);
}

Rect GLUIPainter::getButtonRect(const Rect & r, const char * text, Rect & rt) const
{
    Rect rect = r;
    rt.x = /*4* */getWidgetMargin();
    rt.y = /*4* */getWidgetMargin();
    
    if (rect.w == 0)
    {
        rt.w = getTextLineWidth(text);
        rect.w = rt.w + 2*rt.x;
    }
    else
    {
        rt.w = rect.w - 2*rt.x;
    }
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }
    else
    {
        rt.h = rect.h - 2*rt.y;
    }
    return rect;
}

void GLUIPainter::drawButton(const Rect & r, const char * text, const Rect & rt, bool isDown, bool isHover, bool isFocus, int /*style*/)
{
    drawFrame( r, Point( rt.x, rt.y ), isHover, isDown, isFocus );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h) , text);
}

Rect GLUIPainter::getCheckRect(const Rect & r, const char * text, Rect & rt, Rect& rc) const
{
    Rect rect = r;
    
    int rcOffset = (int) (0.125*getAutoHeight());
    rc.h = getAutoHeight() - 2*rcOffset;
    rc.w = rc.h;
    
    rc.x = getWidgetMargin() + rcOffset;
    rc.y = getWidgetMargin() + rcOffset;
        
    rt.x = getAutoHeight() + 2*getWidgetMargin();
    rt.y = getWidgetMargin();

    if (rect.w == 0)
    {
        rt.w = getTextLineWidth(text);
        rect.w = rt.x + rt.w + getWidgetMargin();
    }

    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }

    return rect;
}

void GLUIPainter::drawCheckButton(const Rect & r, const char * text, const Rect & rt, const Rect& rc, bool isChecked, bool isHover, bool isFocus, int style)
{	
    if (style) drawFrame( r, Point( rt.y, rt.y ), isHover, false, isFocus );
    drawBoolFrame( Rect(r.x+rc.x, r.y+rc.y, rc.w, rc.h), Point( rc.w/6, rc.h/6 ), isHover, isChecked, false );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h) , text);
}

Rect GLUIPainter::getRadioRect(const Rect & r, const char * text, Rect & rt, Rect& rr) const
{
    Rect rect = r;
        
    int rrOffset = (int) (0.125*getAutoHeight());
    rr.h = getAutoHeight() - 2*rrOffset;
    rr.w = rr.h;
    
    rr.x = getWidgetMargin() + rrOffset;
    rr.y = getWidgetMargin() + rrOffset;

    rt.x = getAutoHeight() + 2*getWidgetMargin();
    rt.y = getWidgetMargin();

    if (rect.w == 0)
    {
        rt.w = getTextLineWidth(text);
        rect.w = rt.w + rt.x + getWidgetMargin();
    }

    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }

    return rect;
}

void GLUIPainter::drawRadioButton(const Rect & r, const char * text, const Rect & rt, const Rect & rr, bool isOn, bool isHover, bool isFocus, int style)
{
    if (style) drawFrame( r, Point( rt.y, rt.y ), isHover, false, isFocus );
    drawBoolFrame( Rect(r.x+rr.x, r.y+rr.y, rr.w, rr.h), Point( rr.w/2, rr.h/2 ), isHover, isOn, false );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h) , text);
}

Rect GLUIPainter::getHorizontalSliderRect(const Rect & r, Rect& rs, float v, Rect& rc) const
{
    Rect rect = r;

    if (rect.w == 0)
        rect.w = getAutoWidth() + 2 * getWidgetMargin();

    if (rect.h == 0)
        rect.h = getAutoHeight() + 2 * getWidgetMargin();
    
    // Eval the sliding & cursor rect
    rs.y = getWidgetMargin();
    rs.h = rect.h - 2 * rs.y;
    
    rc.y = rs.y;
    rc.h = rs.h;
    
    rs.x = 0;//getWidgetMargin(); 
    rc.w = rc.h;
    rs.w = rect.w - 2 * rs.x - rc.w;
    rc.x = int(v * rs.w);

    return rect;
}

void GLUIPainter::drawHorizontalSlider(const Rect & r, Rect& rs, float /*v*/, Rect& rc, bool isHover, int /*style*/)
{
    int sliderHeight = rs.h/3;
    drawFrame( Rect(r.x + rs.x, r.y + rs.y + sliderHeight, r.w - 2*rs.x, sliderHeight), Point(sliderHeight/2, sliderHeight/2), isHover, false, false );
    drawFrame( Rect(r.x + rs.x + rc.x, r.y + rc.y, rc.w, rc.h), Point(rc.w/2, rc.h/2), isHover, true, false );
}


Rect GLUIPainter::getItemRect(const Rect & r, const char * text, Rect & rt) const
{
    Rect rect = r;
    rt.x = 0;
    rt.y = 0;


    if (rect.w == 0)
    {
        rt.w = getTextLineWidth(text);
        rect.w = rt.w + 2*rt.x;
    }
    else
    {
        rt.w = rect.w - 2*rt.x;
    }
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }
    else
    {
        rt.h = rect.h - 2*rt.y;
    }
    return rect;
}

Rect GLUIPainter::getListRect(const Rect & r, int numOptions, const char * options[], Rect& ri, Rect & rt) const
{
    Rect rect = r;
    ri.x = getWidgetMargin();
    ri.y = getWidgetMargin();
    rt.x = getWidgetMargin();
    rt.y = getWidgetMargin();

    if (rect.w == 0)
    {
        rt.w = 0;
        for (int i = 0; i < numOptions; i++)
        {
            int l = getTextLineWidth(options[i]);
            rt.w = ( l > rt.w ? l : rt.w ); 
        }
        ri.w = rt.w + 2*rt.x;
        rect.w = ri.w + 2*ri.x;
    }
    else
    {
        ri.w = rect.w - 2*ri.x;
        rt.w = ri.w - 2*rt.x;
    }
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        ri.h = rt.h + rt.y;
        rect.h = numOptions * ri.h + 2*ri.y;
    }
    else
    {
        ri.h = (rect.h - 2*ri.y) / numOptions;
        rt.h = ri.h - rt.y;
    }
    return rect;
}

void GLUIPainter::drawListItem(const Rect & r, const char * text, const Rect & rt, bool isSelected, bool isHover, int /*style*/)
{
//	drawFrame( r, Point(0, 0), isHover, isSelected, false );	
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h), text, isHover, isSelected);
}

void GLUIPainter::drawListBox(const Rect & r, int numOptions, const char * options[], const Rect& ri, const Rect & rt, int selected, int hovered, int /*style*/)
{
    drawFrame( r, Point(ri.x, ri.y) );

    Rect ir( r.x + ri.x, r.y + r.h - ri.y - ri.h, ri.w, ri.h );  
    for ( int i = 0; i < numOptions; i++ )
    {	
        if ( (i == hovered) || (i == selected))
        {
            drawFrame( ir, Point(ri.x, ri.y), false, (i == selected));
        }

        drawText( Rect(ir.x + rt.x , ir.y + rt.y, rt.w, rt.h), options[i]);	

        ir.y -= ir.h;
    }
}

Rect GLUIPainter::getComboRect(const Rect & r, int numOptions, const char * options[], int /*selected*/, Rect& rt, Rect& rd) const
{
    Rect rect = r;
    rt.x = getWidgetMargin();
    rt.y = getWidgetMargin();
    
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2*rt.y;
    }
    else
    {
        rt.h = rect.h - 2*rt.y;
    }

    rd.h = rt.h;
    rd.w = rd.h;     
    rd.y = rt.y;

    if (rect.w == 0)
    {
        rt.w = 0;
        for (int i = 0; i < numOptions; i++)
        {
            int l = getTextLineWidth(options[i]);
            rt.w = ( l > rt.w ? l : rt.w ); 
        }
        rect.w = rt.w + 2*rt.x;

        //Add room for drop down button
        rect.w += rt.h + rt.x;
    }
    else
    {
        //Add room for drop down button
        rt.w = rect.w - 3*rt.x - rt.h;
    }
    rd.x = 2*rt.x + rt.w;

    return rect;
}

Rect GLUIPainter::getComboOptionsRect(const Rect & rCombo, int numOptions, const char * options[], Rect& ri, Rect & rit) const
{
    // the options frame is like a list box
    Rect rect = getListRect( Rect(), numOptions, options, ri, rit);
    
    // offset by the Combo box pos itself
    rect.x = rCombo.x;
    rect.y = rCombo.y - rect.h;

    return rect;
}

void GLUIPainter::drawComboBox(const Rect & r, int /*numOptions*/, const char * options[], const Rect & rt, const Rect& rd, int selected, bool isHover, bool isFocus, int /*style*/)
{
    drawFrame( r, Point(rt.x, rt.y), isHover, false, isFocus );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h), options[ selected ] );

    drawDownArrow( Rect(r.x+rd.x, r.y+rd.y, rd.w, rd.h), int(rd.h * 0.15f), cBase + (!isHover) + (isFocus << 2), cOutline);
}

void GLUIPainter::drawComboOptions(const Rect & r, int numOptions, const char * options[], const Rect& ri, const Rect & rt, int selected, int hovered, bool /*isHover*/, bool /*isFocus*/, int style)
{
    m_foregroundDL = glGenLists(1);
    glNewList( m_foregroundDL, GL_COMPILE);
        drawListBox(r, numOptions, options, ri, rt, selected, hovered, style);
    glEndList();
}

Rect GLUIPainter::getPanelRect(const Rect & r, const char * text, Rect& rt, Rect& ra) const
{
    Rect rect = r;
    rt.x = getWidgetMargin();
    rt.y = getWidgetMargin();
    
    if (rect.h == 0)
    {
        rt.h = getFontHeight();
        rect.h = rt.h + 2 * rt.y;
    }
    else
    {
        rt.h = rect.h - 2 * rt.y;
    }   

    ra.h = rt.h;
    ra.w = ra.h;     
    ra.y = rt.y;

    if (rect.w == 0)
    {
        rt.w = getTextLineWidth(text);
        rect.w = rt.w + 2 * rt.x;
        
        // Add room for drop down button
        rect.w += ra.h + rt.x;
    }
    else
    {
        // Add room for drop down button
        rt.w = rect.w - 3 * rt.x - ra.h;
    }
    ra.x = 2 * rt.x + rt.w;

    return rect;

}

void GLUIPainter::drawPanel(const Rect & r, const char * text, const Rect & rt, const Rect & ra, bool isUnfold, bool isHover, bool isFocus, int /*style*/)
{
    drawFrame( r, Point(rt.x, rt.y), isHover, false, isFocus );
    drawText( Rect(r.x+rt.x, r.y+rt.y, rt.w, rt.h), text );
    if (isUnfold)
        drawMinus( Rect(r.x+ra.x, r.y+ra.y, ra.w, ra.h), int(ra.h * 0.15f), cBase + (!isHover) + (isFocus << 2), cOutline);
    else
        drawPlus( Rect(r.x+ra.x, r.y+ra.y, ra.w, ra.h), int(ra.h * 0.15f), cBase + (!isHover) + (isFocus << 2), cOutline);
}

Rect GLUIPainter::getTextureViewRect(const Rect & r, Rect& rt) const
{
    Rect rect = r;

    if (rect.w == 0)
        rect.w = getAutoWidth();

    if (rect.h == 0)
        rect.h = rect.w;

    rt.x = getCanvasMargin();
    rt.y = getCanvasMargin();
    rt.w = rect.w - 2 * getCanvasMargin();
    rt.h = rect.h - 2 * getCanvasMargin();

    return rect;
}

void GLUIPainter::drawTextureView(const Rect & rect, const void* texID, const Rect& rt, const Rect & rz, int mipLevel, 
                                  float texelScale, float texelOffset, int r, int g, int b, int a, 
                                  int /*style*/)
{
    drawFrame( rect, Point(rt.x, rt.y), false, false, false );

    GLuint lTexID = reinterpret_cast<GLuint> ( texID );

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lTexID);
    glColor3f(1.0f, 1.0f, 1.0f);	

    glUseProgram(m_textureViewProgram);
    glUniform1f( m_texMipLevelUniform, (float) mipLevel);
    glUniform1f( m_texelScaleUniform, texelScale);
    glUniform1f( m_texelOffsetUniform, texelOffset);
    glUniform4i( m_texelSwizzlingUniform, r, g, b, a);


    glBegin(GL_QUADS);
        glTexCoord2f( (float) rz.x / (float) rt.w , (float) rz.y / (float) rt.h);
        glVertex2f(rect.x + rt.x, rect.y + rt.y);
        glTexCoord2f((float) rz.x / (float) rt.w , (float) (rz.y + rz.h) / (float) rt.h);
        glVertex2f(rect.x + rt.x, rect.y + rt.y + rt.h);
        glTexCoord2f((float) (rz.x+rz.w) / (float) rt.w , (float) (rz.y + rz.h) / (float) rt.h);
        glVertex2f(rect.x + rt.x + rt.w, rect.y + rt.y + rt.h);
        glTexCoord2f((float) (rz.x+rz.w) / (float) rt.w , (float) (rz.y) / (float) rt.h);
        glVertex2f(rect.x + rt.x + rt.w, rect.y + rt.y);
    glEnd();

    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}


void GLUIPainter::init()
{
    if (m_widgetProgram == 0)
    {
        GLuint vShader = nv::CompileGLSLShader( GL_VERTEX_SHADER, cWidgetVSSource);
        if (!vShader) fprintf(stderr, "Vertex shader compile failed\n");
        GLuint fShader = nv::CompileGLSLShader( GL_FRAGMENT_SHADER, cWidgetFSSource);
        if (!fShader) fprintf(stderr, "Fragment shader compile failed\n");

        m_widgetProgram = nv::LinkGLSLProgram( vShader, fShader );	
        
        m_fillColorUniform = glGetUniformLocation(m_widgetProgram, "fillColor");
        m_borderColorUniform = glGetUniformLocation(m_widgetProgram, "borderColor");
        m_zonesUniform = glGetUniformLocation(m_widgetProgram, "zones");

        if (m_textureViewProgram == 0)
        {
            GLuint fShaderTex = nv::CompileGLSLShader( GL_FRAGMENT_SHADER, cTexViewWidgetFSSource);
            if (!fShaderTex) fprintf(stderr, "Fragment shader compile failed\n");

            m_textureViewProgram = nv::LinkGLSLProgram( vShader, fShaderTex );	
            m_texMipLevelUniform = glGetUniformLocation(m_textureViewProgram, "mipLevel");
            m_texelScaleUniform = glGetUniformLocation(m_textureViewProgram, "texelScale");
            m_texelOffsetUniform = glGetUniformLocation(m_textureViewProgram, "texelOffset");
            m_texelSwizzlingUniform = glGetUniformLocation(m_textureViewProgram, "texelSwizzling");
        }
    }

	if (m_setupStateDL == 0)
	{
        m_setupStateDL = glGenLists(1);
        glNewList( m_setupStateDL, GL_COMPILE);
		{			
    	    // Cache previous state
            glPushAttrib( GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT );
            
            // fill mode always
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		   
            // Stencil / Depth buffer and test disabled
			glDisable(GL_STENCIL_TEST);
			glStencilMask( 0 );
			glDisable(GL_DEPTH_TEST);
			glDepthMask( GL_FALSE );
			
			// Blend on for alpha
            glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Color active
			glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
		
            // Modelview is identity
            glMatrixMode( GL_MODELVIEW );
            glPushMatrix();
            glLoadIdentity();
        }
		glEndList();
	}

    if (m_restoreStateDL == 0)
	{
        m_restoreStateDL = glGenLists(1);
        glNewList( m_restoreStateDL, GL_COMPILE);
		{			
            // Restore state.
	        glPopAttrib();

	        // Restore matrices.
            glMatrixMode( GL_PROJECTION);
            glPopMatrix();
            glMatrixMode( GL_MODELVIEW);
            glPopMatrix();
		}
		glEndList();
	}

    if (m_textListBase == 0)
    {
        //just doing 7-bit ascii
        m_textListBase = glGenLists(128);

        for (int ii = 0; ii < 128; ii++) {
            glNewList( m_textListBase + ii, GL_COMPILE);
            glutBitmapCharacter( GLUT_BITMAP_HELVETICA_12, ii);
            glEndList();
        }
    }
}

void GLUIPainter::begin(const Rect& window)
{
    init();

	// Cache and setup state
    glCallList( m_setupStateDL );

    // Set matrices.
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D( window.x, window.w, window.y, window.h);
}

void GLUIPainter::end()
{
    if ( m_foregroundDL )
    {
        glCallList( m_foregroundDL );
        glDeleteLists( m_foregroundDL, 1 );
        m_foregroundDL = 0;
    }

    // Restore state.
	glCallList( m_restoreStateDL );
}

// Draw Primitive shapes

void GLUIPainter::drawString( int x, int y, const char * text, int nbLines )
{
    glListBase(m_textListBase);

    if (nbLines < 2)
    {
    glRasterPos2i(x+1, y + 4);
    glCallLists((GLsizei)strlen(text), GL_UNSIGNED_BYTE, text);
    }
    else
    {
    int lineNb = nbLines;
    int startNb = 0;
    int endNb = 0;

    const char* textOri = text;
    while (lineNb)
    {
        if (*text != '\0' && *text != '\n')
        {
            endNb++;
        }
        else
        {
            lineNb--;
            glRasterPos2i(x+1, y + 4 + lineNb*getFontHeight());
            glCallLists(endNb - startNb , GL_UNSIGNED_BYTE, textOri + startNb);
            
            endNb++;
            startNb = endNb;
        }
        text++;
    }
    }
}

void GLUIPainter::drawRect( const Rect & rect, int fillColorId, int borderColorId ) const
{
    glUseProgram(m_widgetProgram);

    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, 0, 0);

    float x0 = rect.x;
    float x1 = rect.x + rect.w;
    
    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0);

        glVertex2f( x0, y0);
        glVertex2f( x1, y0);

        glVertex2f( x0, y1);
        glVertex2f( x1, y1);
    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawRoundedRect( const Rect& rect, const Point& corner, int fillColorId, int borderColorId ) const
{
    glUseProgram(m_widgetProgram);
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, corner.x - 1, corner.x - 2);

    float xb = corner.x;
    float yb = corner.y;

    float x0 = rect.x;
    float x1 = rect.x + corner.x;
    float x2 = rect.x + rect.w - corner.x;
    float x3 = rect.x + rect.w;
    
    float y0 = rect.y;
    float y1 = rect.y + corner.y;
    float y2 = rect.y + rect.h - corner.y;
    float y3 = rect.y + rect.h;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(xb, yb);
        glVertex2f( x0, y0);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y0);

        glTexCoord2f(xb, 0);
        glVertex2f( x0, y1);
        glTexCoord2f(0, 0);
        glVertex2f( x1, y1);

        glTexCoord2f(xb, 0);
        glVertex2f( x0, y2);
        glTexCoord2f(0, 0);
        glVertex2f( x1, y2);

        glTexCoord2f(xb, yb);
        glVertex2f( x0, y3);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y3);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y0);
        glTexCoord2f(xb, yb);
        glVertex2f( x3, y0);

        glTexCoord2f(0, 0);
        glVertex2f( x2, y1);
        glTexCoord2f(xb, 0);
        glVertex2f( x3, y1);

        glTexCoord2f(0, 0);
        glVertex2f( x2, y2);
        glTexCoord2f(xb, 0);
        glVertex2f( x3, y2);
 
        glTexCoord2f(0, yb);
        glVertex2f( x2, y3);
        glTexCoord2f(xb, yb);
        glVertex2f( x3, y3);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y0);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y0);

        glTexCoord2f(0, 0);
        glVertex2f( x1, y1);
        glTexCoord2f(0, 0);
        glVertex2f( x2, y1);

        glTexCoord2f(0, 0);
        glVertex2f( x1, y2);
        glTexCoord2f(0, 0);
        glVertex2f( x2, y2);
 
        glTexCoord2f(0, yb);
        glVertex2f( x1, y3);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y3);
    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawRoundedRectOutline( const Rect& rect, const Point& corner, int borderColorId ) const
{
    glUseProgram(m_widgetProgram);
    glUniform4fv( m_fillColorUniform, 1, s_colors[cTranslucent]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, corner.x - 1, corner.x - 2);

    float xb = corner.x;
    float yb = corner.y;

    float x0 = rect.x;
    float x1 = rect.x + corner.x;
    float x2 = rect.x + rect.w - corner.x;
    float x3 = rect.x + rect.w;
    
    float y0 = rect.y;
    float y1 = rect.y + corner.y;
    float y2 = rect.y + rect.h - corner.y;
    float y3 = rect.y + rect.h;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(xb, yb);
        glVertex2f( x0, y0);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y0);

        glTexCoord2f(xb, 0);
        glVertex2f( x0, y1);
        glTexCoord2f(0, 0);
        glVertex2f( x1, y1);

        glTexCoord2f(xb, 0);
        glVertex2f( x0, y2);
        glTexCoord2f(0, 0);
        glVertex2f( x1, y2);

        glTexCoord2f(xb, yb);
        glVertex2f( x0, y3);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y3);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y0);
        glTexCoord2f(xb, yb);
        glVertex2f( x3, y0);

        glTexCoord2f(0, 0);
        glVertex2f( x2, y1);
        glTexCoord2f(xb, 0);
        glVertex2f( x3, y1);

        glTexCoord2f(0, 0);
        glVertex2f( x2, y2);
        glTexCoord2f(xb, 0);
        glVertex2f( x3, y2);
 
        glTexCoord2f(0, yb);
        glVertex2f( x2, y3);
        glTexCoord2f(xb, yb);
        glVertex2f( x3, y3);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, yb);
        glVertex2f( x1, y0);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y0);

        glTexCoord2f(0, 0);
        glVertex2f( x1, y1);
        glTexCoord2f(0, 0);
        glVertex2f( x2, y1);
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0, 0);
        glVertex2f( x1, y2);
        glTexCoord2f(0, 0);
        glVertex2f( x2, y2);
 
        glTexCoord2f(0, yb);
        glVertex2f( x1, y3);
        glTexCoord2f(0, yb);
        glVertex2f( x2, y3);
    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawCircle( const Rect& rect, int fillColorId, int borderColorId ) const
{
    glUseProgram(m_widgetProgram);
    
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, (rect.w / 2) - 1, (rect.w / 2) - 2);


    float xb = rect.w / 2;
    float yb = rect.w / 2;

    float x0 = rect.x;
    float x1 = rect.x + rect.w;

    float y0 = rect.y;
    float y1 = rect.y + rect.h;

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(-xb, -yb);
        glVertex2f( x0, y0);
        glTexCoord2f(xb, -yb);
        glVertex2f( x1, y0);
        glTexCoord2f(-xb, yb);
        glVertex2f( x0, y1);
        glTexCoord2f(xb, yb);
        glVertex2f( x1, y1);
    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawMinus( const Rect& rect, int width, int fillColorId, int borderColorId ) const
{
    float xb = width;
    float yb = width;
    
    float xoff = xb ;
    float yoff = yb ;

    float x0 = rect.x + rect.w * 0.1 ;
    float x1 = rect.x + rect.w * 0.9;

    float y1 = rect.y + rect.h * 0.5;

    glUseProgram(m_widgetProgram);
    
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, (xb) - 1, (xb) - 2);

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x0, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x0, y1 - yoff);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x0 + xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x0 + xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x1 - xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x1 - xoff, y1 - yoff);

        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x1, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x1, y1 - yoff);
    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawPlus( const Rect& rect, int width, int fillColorId, int borderColorId ) const
{
    float xb = width;
    float yb = width;
    
    float xoff = xb ;
    float yoff = yb ;

    float x0 = rect.x + rect.w * 0.1 ;
    float x1 = rect.x + rect.w * 0.5;
    float x2 = rect.x + rect.w * 0.9;

    float y0 = rect.y + rect.h * 0.1;
    float y1 = rect.y + rect.h * 0.5;
    float y2 = rect.y + rect.h * 0.9;

    glUseProgram(m_widgetProgram);
    
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, (xb) - 1, (xb) - 2);

 /*   glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x0, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x0, y1 - yoff);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x0 + xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x0 + xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x1 - xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x1 - xoff, y1 - yoff);

        glTexCoord3f(0, yb, 0);
        glVertex2f( x1, y1);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(0, yb, 0);
        glVertex2f( x1, y1);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x1 + xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x1 + xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x2 - xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x2 - xoff, y1 - yoff);

        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x2, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x2, y1 - yoff);
    glEnd();

/**/    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x0, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x0, y1 - yoff);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x0 + xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x0 + xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x2 - xoff , y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x2 - xoff, y1 - yoff);

        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x2, y1 + yoff);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x2, y1 - yoff);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x1 + yoff, y0);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x1 - yoff, y0);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x1 + yoff, y0 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x1 - yoff, y0 + yoff);

        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x1 + yoff, y2 - yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x1 - yoff, y2 - yoff);

        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x1 + yoff, y2);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x1 - yoff, y2);
    glEnd();
/**/
    glUseProgram(0);
}

void GLUIPainter::drawDownArrow( const Rect& rect, int width, int fillColorId, int borderColorId ) const
{
    float offset = sqrt(2.0)/2.0 ;
   
    float xb = width;
    float yb = width;
    
    float xoff = offset * xb ;
    float yoff = offset * yb ;
    float xoff2 = offset * xb *2.0 ;
    float yoff2 = offset * yb *2.0;

    float x0 = rect.x + xoff2;
    float x1 = rect.x + rect.w * 0.5;
    float x2 = rect.x + rect.w - xoff2;

    float y0 = rect.y + rect.h * 0.1 + yoff2;
    float y1 = rect.y + rect.h * 0.6;

    glUseProgram(m_widgetProgram);
    
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, (xb) - 1, (xb) - 2);

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x0, y1 + yoff2);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x0 - xoff2, y1);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x0 + xoff, y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x0 - xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x1, y0 + yoff2);
        glTexCoord3f(xb, 0, xb);
        glVertex2f( x1 - xoff2, y0);

        glTexCoord3f(xb, 2*yb, xb);
        glVertex2f( x1, y0 - yoff2);

    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x2 + xoff2, y1);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x2, y1 + yoff2);

        glTexCoord3f(xb, 0, xb);
        glVertex2f( x2 + xoff, y1 - yoff);
        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x2 - xoff, y1 + yoff);

        glTexCoord3f(xb, 0, xb);
        glVertex2f( x1 + xoff2, y0);
        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x1, y0 + yoff2);

        glTexCoord3f(xb, 2*yb, xb);
        glVertex2f( x1, y0 - yoff2);

    glEnd();

    glUseProgram(0);
}

void GLUIPainter::drawUpArrow( const Rect& rect, int width, int fillColorId, int borderColorId ) const
{
    float offset = sqrt(2.0)/2.0 ;
   
    float xb = width;
    float yb = width;
    
    float xoff = offset * xb ;
    float yoff = - offset * yb ;
    float xoff2 = offset * xb *2.0 ;
    float yoff2 = - offset * yb *2.0;

    float x0 = rect.x + xoff2;
    float x1 = rect.x + rect.w * 0.5;
    float x2 = rect.x + rect.w - xoff2;

    float y0 = rect.y + rect.h * 0.9 + yoff2;
    float y1 = rect.y + rect.h * 0.4;

    glUseProgram(m_widgetProgram);
    
    glUniform4fv( m_fillColorUniform, 1, s_colors[fillColorId]);
    glUniform4fv( m_borderColorUniform, 1, s_colors[borderColorId]);
    glUniform2f( m_zonesUniform, (xb) - 1, (xb) - 2);

    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x0, y1 + yoff2);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x0 - xoff2, y1);
 
        glTexCoord3f(-xb, 0, 0);
        glVertex2f( x0 + xoff, y1 + yoff);
        glTexCoord3f(xb, 0, 0);
        glVertex2f( x0 - xoff, y1 - yoff);

        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x1, y0 + yoff2);
        glTexCoord3f(xb, 0, xb);
        glVertex2f( x1 - xoff2, y0);

        glTexCoord3f(xb, 2*yb, xb);
        glVertex2f( x1, y0 - yoff2);

    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord3f(xb, -yb, 0);
        glVertex2f( x2 + xoff2, y1);
        glTexCoord3f(-xb, -yb, 0);
        glVertex2f( x2, y1 + yoff2);

        glTexCoord3f(xb, 0, xb);
        glVertex2f( x2 + xoff, y1 - yoff);
        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x2 - xoff, y1 + yoff);

        glTexCoord3f(xb, 0, xb);
        glVertex2f( x1 + xoff2, y0);
        glTexCoord3f(-xb, 0, xb);
        glVertex2f( x1, y0 + yoff2);

        glTexCoord3f(xb, 2*yb, xb);
        glVertex2f( x1, y0 - yoff2);

    glEnd();

    glUseProgram(0);
}

float nv_ui_text_color_r=1;
float nv_ui_text_color_g=1;
float nv_ui_text_color_b=1;

void nv_ui_set_text_color(float r,float g,float b)
{
	nv_ui_text_color_r=r;
	nv_ui_text_color_g=g;
	nv_ui_text_color_b=b;
}

void GLUIPainter::drawText( const Rect& r, const char * text, int nbLines, int caretPos, bool isHover, bool isOn, bool /*isFocus*/ )
{
    if (isHover || isOn /* || isFocus*/)
    {
        drawRect(r, cFontBack + (isHover) + (isOn << 1), cOutline);	
    }

    //glColor4fv(s_colors[cFont]);
	glColor3f(
		nv_ui_text_color_r,
		nv_ui_text_color_g,
		nv_ui_text_color_b);

    drawString(r.x, r.y, text, nbLines);
    
    if (caretPos != -1)
    {
        int w = getTextLineWidthAt( text, caretPos);

        drawRect(Rect( r.x + w, r.y, 2, r.h), cOutline, cOutline);
    }
}

void GLUIPainter::drawFrame( const Rect& rect, const Point& corner, bool isHover, bool isOn, bool /*isFocus*/ ) const
{
    int lColorNb = cBase + (isHover) + (isOn << 1);// + (isFocus << 2);

    if (corner.x + corner.y == 0)
        drawRect( rect , lColorNb, cOutline);
    else
        drawRoundedRect( rect, corner , lColorNb, cOutline );
}

void GLUIPainter::drawBoolFrame( const Rect& rect, const Point& corner, bool isHover, bool isOn, bool /*isFocus*/ ) const
{
    int lColorNb = cBool + (isHover) + (isOn << 1);// + (isFocus << 2);
        
    drawRoundedRect( rect, corner , lColorNb, cOutline );
}

void GLUIPainter::drawDebugRect(const Rect & rect)
{
    glBegin(GL_LINE_STRIP);
        glVertex2i( rect.x + 1, rect.y + 1);
        glVertex2i( rect.x + rect.w, rect.y + 1);
        glVertex2i( rect.x + rect.w, rect.y + rect.h);
        glVertex2i( rect.x + 1, rect.y + rect.h);
        glVertex2i( rect.x + 1, rect.y);
    glEnd();
}


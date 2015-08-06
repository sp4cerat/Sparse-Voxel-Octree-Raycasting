//
// nvGLWidgets.h - User Interface library
//
//
// Author: Ignacio Castano, Samuel Gateau, Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef NV_GL_WIDGETS_H
#define NV_GL_WIDGETS_H

#include "nvWidgets.h"

namespace nv
{

    //*************************************************************************
    // GLUIPainter
    class GLUIPainter : public UIPainter
    {
    public:

        NVSDKENTRY GLUIPainter();

        NVSDKENTRY virtual void begin( const Rect& window );
        NVSDKENTRY virtual void end();

        // These methods should be called between begin/end
        NVSDKENTRY virtual void drawFrame(const Rect & r, int margin, int style);

        NVSDKENTRY virtual Rect getLabelRect(const Rect & r, const char * text, Rect & rt, int& nbLines) const;
        NVSDKENTRY virtual void drawLabel(const Rect & r, const char * text, const Rect & rt, const int& nbLines, bool isHover, int style);
     
        NVSDKENTRY virtual Rect getButtonRect(const Rect & r, const char * text, Rect & rt) const;
        NVSDKENTRY virtual void drawButton(const Rect & r, const char * text, const Rect & rt, bool isDown, bool isHover, bool isFocus, int style);
    
        NVSDKENTRY virtual Rect getCheckRect(const Rect & r, const char * text, Rect & rt, Rect & rc) const;
        NVSDKENTRY virtual void drawCheckButton(const Rect & r, const char * text, const Rect & rt, const Rect & rr, bool isChecked, bool isHover, bool isFocus, int style);

        NVSDKENTRY virtual Rect getRadioRect(const Rect & r, const char * text, Rect & rt, Rect & rr) const;
        NVSDKENTRY virtual void drawRadioButton(const Rect & r, const char * text, const Rect & rt, const Rect & rr, bool isOn, bool isHover, bool isFocus, int style);

        NVSDKENTRY virtual Rect getHorizontalSliderRect(const Rect & r, Rect& rs, float v, Rect& rc) const;
        NVSDKENTRY virtual void drawHorizontalSlider(const Rect & r, Rect& rs, float v, Rect& rc, bool isHover, int style);

        NVSDKENTRY virtual Rect getItemRect(const Rect & r, const char * text, Rect & rt) const;
        NVSDKENTRY virtual Rect getListRect(const Rect & r, int numOptions, const char * options[], Rect& ri, Rect & rt) const;
        NVSDKENTRY virtual void drawListItem(const Rect & r, const char * text, const Rect & rt, bool isSelected, bool isHover, int style);
        NVSDKENTRY virtual void drawListBox(const Rect & r, int numOptions, const char * options[], const Rect& ri, const Rect & rt, int selected, int hovered, int style);

        NVSDKENTRY virtual Rect getComboRect(const Rect & r, int numOptions, const char * options[], int selected, Rect& rt, Rect& ra) const;
        NVSDKENTRY virtual Rect getComboOptionsRect(const Rect & rCombo, int numOptions, const char * options[], Rect& ri, Rect & rit) const;
        NVSDKENTRY virtual void drawComboBox(const Rect & rect, int numOptions, const char * options[], const Rect & rt, const Rect& rd, int selected, bool isHover, bool isFocus, int style);
        NVSDKENTRY virtual void drawComboOptions(const Rect & rect, int numOptions, const char * options[], const Rect& ri, const Rect & rit, int selected, int hovered, bool isHover, bool isFocus, int style);

        NVSDKENTRY virtual Rect getLineEditRect(const Rect & r, const char * text, Rect & rt) const;
        NVSDKENTRY virtual void drawLineEdit(const Rect & r, const char * text, const Rect & rt, int caretPos, bool isSelected, bool isHover, int style);

        NVSDKENTRY virtual Rect getPanelRect(const Rect & r, const char * text, Rect& rt, Rect& ra) const;
        NVSDKENTRY virtual void drawPanel(const Rect & rect, const char * text, const Rect & rt, const Rect & ra, bool isUnfold, bool isHover, bool isFocus, int style);

        NVSDKENTRY virtual Rect getTextureViewRect(const Rect & rect, Rect& rt) const;
        NVSDKENTRY virtual void drawTextureView(const Rect & rect, const void* texID, const Rect& rt, const Rect & rz, int mipLevel, 
                                                float texelScale, float texelOffset, int r, int g, int b, int a, 
                                                int style);

        // Eval widget dimensions
        NVSDKENTRY virtual int getCanvasMargin() const;
        NVSDKENTRY virtual int getCanvasSpace() const;
        NVSDKENTRY virtual int getFontHeight() const;
        NVSDKENTRY virtual int getTextLineWidth(const char * text) const;
        NVSDKENTRY virtual int getTextSize(const char * text, int& nbLines) const;
        NVSDKENTRY virtual int getTextLineWidthAt(const char * text, int charNb) const;
        NVSDKENTRY virtual int getPickedCharNb(const char * text, const Point& at) const;

        NVSDKENTRY virtual void drawDebugRect(const Rect & r);

    protected:

        // Draw primitive shapes
        NVSDKENTRY void drawText( const Rect& r , const char * text, int nbLines = 1, int caretPos = -1, bool isHover = false, bool isOn = false, bool isFocus = false );
        NVSDKENTRY void drawFrame( const Rect& rect, const Point& corner, bool isHover = false, bool isOn = false, bool isFocus = false ) const;
        NVSDKENTRY void drawBoolFrame( const Rect& rect, const Point& corner, bool isHover = false, bool isOn = false, bool isFocus = false ) const;

        NVSDKENTRY void drawString( int x, int y, const char * text, int nbLines );
        NVSDKENTRY void drawRect( const Rect& rect, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawRoundedRect( const Rect& rect, const Point& corner, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawRoundedRectOutline( const Rect& rect, const Point& corner, int borderColorId ) const;
        NVSDKENTRY void drawCircle( const Rect& rect, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawMinus( const Rect& rect, int width, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawPlus( const Rect& rect, int width, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawDownArrow( const Rect& rect, int width, int fillColorId, int borderColorId ) const;
        NVSDKENTRY void drawUpArrow( const Rect& rect, int width, int fillColorId, int borderColorId ) const;

        NVSDKENTRY void init();

    private:

        unsigned int m_setupStateDL;
        unsigned int m_restoreStateDL;
        unsigned int m_textListBase;
        unsigned int m_foregroundDL;

        unsigned int m_widgetProgram;
        unsigned int m_originUniform;
        unsigned int m_sizeUniform;
        unsigned int m_fillColorUniform;
        unsigned int m_borderColorUniform;
        unsigned int m_zonesUniform;
         
        unsigned int m_textureViewProgram;
        unsigned int m_texMipLevelUniform;
        unsigned int m_texelScaleUniform;
        unsigned int m_texelOffsetUniform;
        unsigned int m_texelSwizzlingUniform;

    };
};


#endif 

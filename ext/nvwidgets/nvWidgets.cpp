//
// nvWidgets.cpp - User Interface library
//
//
// Author: Ignacio Castano, Samuel Gateau, Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include "nvWidgets.h"

#include <time.h> // clock, time_t
#include <math.h> // fabs
#include <string.h> // strlen

using namespace nv;

const Rect Rect::null;

template <typename T> T min(const T & a, const T & b)
{
	if (a < b) return a;
	return b;
}

template <typename T> T max(const T & a, const T & b)
{
	if (a > b) return a;
	return b;
}

// Ctor.
UIContext::UIContext(UIPainter& painter) :
    m_painter(&painter),
    m_twoStepFocus(false),
    m_focusCaretPos(-1)
{
}


void UIContext::reshape(int w, int h)
{
    m_window.x = 0;
    m_window.y = 0;
    m_window.w = w;
    m_window.h = h;
}


void UIContext::mouse(int button, int state, int modifier, int x, int y)
{
    setCursor(x, y);

    int idx = -1;
    if (button == MouseButton_Left) idx = 0;
    else if (button == MouseButton_Middle) idx = 1;
    else if (button == MouseButton_Right) idx = 2;

    modifier &= ButtonFlags_Alt|ButtonFlags_Shift|ButtonFlags_Ctrl;

    if (idx >= 0)
    {
        if (state == 1) {
            m_mouseButton[idx].state = ButtonFlags_On|ButtonFlags_Begin|modifier;
            m_mouseButton[idx].time = clock();
            m_mouseButton[idx].cursor.x = x;
            m_mouseButton[idx].cursor.y = m_window.h - y;
        }
        if (state == 0) {
            m_mouseButton[idx].state = ButtonFlags_On|ButtonFlags_End|modifier;
        }
    }
}

void UIContext::mouseMotion(int x, int y)
{
    setCursor(x, y);
}

void UIContext::keyboard(unsigned char k, int x, int y)
{
    setCursor(x, y);
    m_keyBuffer[m_nbKeys] = k;
    m_nbKeys++;
}

void UIContext::begin()
{
    m_painter->begin( m_window );

    m_groupIndex = 0;
    m_groupStack[m_groupIndex].flags = GroupFlags_LayoutNone;
    m_groupStack[m_groupIndex].margin = m_painter->getCanvasMargin();
    m_groupStack[m_groupIndex].space = m_painter->getCanvasSpace();
    m_groupStack[m_groupIndex].bounds = m_window;

}

void UIContext::end()
{
    m_painter->end();

    // Release focus.
    if (m_mouseButton[0].state & ButtonFlags_End) m_uiOnFocus = false;

    // Update state for next frame.
    for (int i = 0; i < 3; i++)
    {
        if (m_mouseButton[i].state & ButtonFlags_Begin) m_mouseButton[i].state ^= ButtonFlags_Begin;
        /*else*/ if (m_mouseButton[i].state & ButtonFlags_End) m_mouseButton[i].state = ButtonFlags_Off;
    }

    // Flush key buffer
    m_nbKeys = 0;
}

Rect UIContext::placeRect(const Rect & r)
{
    Group & group = m_groupStack[m_groupIndex];

    Rect rect = r;

    int layout = group.flags & GroupFlags_LayoutMask;
    int alignment = group.flags & GroupFlags_AlignMask;


    if (layout == GroupFlags_LayoutNone)
    {
        // Translate rect to absolute coordinates.
        rect.x += group.bounds.x;
        rect.y += group.bounds.y;
    }
    else if (layout == GroupFlags_LayoutVertical)
    {
        // Vertical behavior
        if (alignment & GroupFlags_AlignTop)
        {
            // Move down bounds.y with the space for the new rect
            group.bounds.y -= ((group.bounds.h > 0) * group.space + rect.h);

            // Widget's y is the group bounds.y
            rect.y = group.bounds.y;
        }
        else
        {
            rect.y = group.bounds.y + group.bounds.h + (group.bounds.h > 0) * group.space;
        }
        // Add space after first object inserted in the group
        group.bounds.h += (group.bounds.h > 0) * group.space + rect.h;

        // Horizontal behavior
        if (alignment & GroupFlags_AlignRight)
        {
            rect.x += group.bounds.x + group.bounds.w - rect.w;
            
            int minBoundX = min(group.bounds.x, rect.x);
            group.bounds.w = group.bounds.x + group.bounds.w - minBoundX;
            group.bounds.x = minBoundX;
        }
        else
        {
            group.bounds.w = max(group.bounds.w, rect.x + rect.w);
            rect.x += group.bounds.x;
       }
    }
    else if (layout == GroupFlags_LayoutHorizontal)
    {
        // Horizontal behavior
        if (alignment & GroupFlags_AlignRight)
        {
            // Move left bounds.x with the space for the new rect
            group.bounds.x -= ((group.bounds.w > 0) * group.space + rect.w);

            // Widget's x is the group bounds.x
            rect.x = group.bounds.x;
        }
        else
        {
            rect.x = group.bounds.x + group.bounds.w + (group.bounds.w > 0) * group.space;
        }
        // Add space after first object inserted in the group
        group.bounds.w += (group.bounds.w > 0) * group.space + rect.w;

        // Vertical behavior
        if (alignment & GroupFlags_AlignTop)
        {
            rect.y += group.bounds.y + group.bounds.h - rect.h;
            
            int minBoundY = min(group.bounds.y, rect.y);
            group.bounds.h = group.bounds.y + group.bounds.h - minBoundY;
            group.bounds.y = minBoundY;
        }
        else
        {
            group.bounds.h = max(group.bounds.h, rect.y + rect.h);
            rect.y += group.bounds.y;
        }
    }
    return rect;
}

void UIContext::beginGroup(int flags, const Rect& r)
{
    // Push one more group.
    Group & parentGroup = m_groupStack[m_groupIndex];
    m_groupIndex++;
    Group & newGroup = m_groupStack[m_groupIndex];

    // Assign layout behavior
    int parentLayout = parentGroup.flags & GroupFlags_LayoutMask;
    int parentAlign = parentGroup.flags & GroupFlags_AlignMask;

    // If the flags ask to force the layout then keep the newcanvas layout as is
    // otherwise, adapt it to the parent's behavior
    if ( ! (flags & GroupFlags_LayoutForce) || ! (flags & GroupFlags_LayoutNone) )
    {
        // If default then use parent style except if none layout => default fallback
        if ( flags & GroupFlags_LayoutDefault )
        {
            if ( parentLayout &  GroupFlags_LayoutNone )
                flags = GroupFlags_LayoutDefaultFallback; 
            else
                flags = parentGroup.flags;
        }
        else if (   parentLayout & ( GroupFlags_LayoutVertical | GroupFlags_LayoutHorizontal) 
                 && flags & ( GroupFlags_LayoutVertical | GroupFlags_LayoutHorizontal) )
        {
            flags = (flags & GroupFlags_AlignXMask) | parentAlign;
        }
    } 

    newGroup.margin = ((flags & GroupFlags_LayoutNoMargin) == 0) * m_painter->getCanvasMargin();
    newGroup.space = ((flags & GroupFlags_LayoutNoSpace) == 0) * m_painter->getCanvasSpace();
    newGroup.flags = flags;
    //int newLayout = flags & GroupFlags_LayoutMask;
    int newAlign = flags & GroupFlags_AlignMask;
    int newStart = flags & GroupFlags_StartMask;

    

    // Place a regular rect in current group, this will be the new group rect start pos
    Rect rect = r;

    // Don't modify parent group bounds yet, done in endGroup()
    // Right now place the new group rect
    if ( parentLayout == GroupFlags_LayoutNone)
    {
        // Horizontal behavior.
        rect.x +=   parentGroup.bounds.x + newGroup.margin
                  + ((newStart & GroupFlags_StartRight)>0)*parentGroup.bounds.w
                  - ((newAlign & GroupFlags_AlignRight)>0)*(2*newGroup.margin + rect.w);

        // Vertical behavior.
        rect.y +=   parentGroup.bounds.y + newGroup.margin
                  + ((newStart & GroupFlags_StartTop)>0)*parentGroup.bounds.h
                  - ((newAlign & GroupFlags_AlignTop)>0)*(2*newGroup.margin + rect.h);
    }
    else if ( parentLayout == GroupFlags_LayoutVertical)
    {
        // Horizontal behavior.
        rect.x +=   parentGroup.bounds.x + newGroup.margin
                  + ((parentAlign & GroupFlags_AlignRight)>0)*(parentGroup.bounds.w - 2*newGroup.margin - rect.w);

        // Vertical behavior.
        if (parentAlign & GroupFlags_AlignTop)
        {
            rect.y += parentGroup.bounds.y - ((parentGroup.bounds.h > 0) * parentGroup.space) - newGroup.margin - rect.h;
        }
        else
        {
            rect.y += parentGroup.bounds.y + parentGroup.bounds.h + (parentGroup.bounds.h > 0) * parentGroup.space + newGroup.margin;
        }
    }
    else if ( parentLayout == GroupFlags_LayoutHorizontal)
    {
        // Horizontal behavior.
        if (parentAlign & GroupFlags_AlignRight)
        {
            rect.x += parentGroup.bounds.x - ((parentGroup.bounds.w > 0) * parentGroup.space) - newGroup.margin - rect.w;
        }
        else
        {
            rect.x += parentGroup.bounds.x + parentGroup.bounds.w + (parentGroup.bounds.w > 0) * parentGroup.space + newGroup.margin;
        }

        // Vertical behavior.
        rect.y +=   parentGroup.bounds.y + newGroup.margin
                  + ((parentAlign & GroupFlags_AlignTop)>0)*(parentGroup.bounds.h - 2*newGroup.margin - rect.h);
    }

    newGroup.bounds = rect;
}

void UIContext::endGroup()
{
    // Pop the new group.
    Group & newGroup = m_groupStack[m_groupIndex];
    m_groupIndex--;
    Group & parentGroup = m_groupStack[m_groupIndex];

    // add any increment from the embedded group
    if (parentGroup.flags & ( GroupFlags_LayoutVertical | GroupFlags_LayoutHorizontal ) )
    {    
        int maxBoundX = max(parentGroup.bounds.x + parentGroup.bounds.w, newGroup.bounds.x + newGroup.bounds.w + newGroup.margin);
        int minBoundX = min(parentGroup.bounds.x, newGroup.bounds.x - newGroup.margin);
        parentGroup.bounds.x = minBoundX;
        parentGroup.bounds.w = maxBoundX - minBoundX;
        
        int maxBoundY = max(parentGroup.bounds.y + parentGroup.bounds.h, newGroup.bounds.y + newGroup.bounds.h + newGroup.margin);
        int minBoundY = min(parentGroup.bounds.y, newGroup.bounds.y - newGroup.margin);
        parentGroup.bounds.y = minBoundY;
        parentGroup.bounds.h = maxBoundY - minBoundY;
    }
}

void UIContext::beginFrame(int flags, const Rect& rect, int /*style*/)
{
    beginGroup(flags, rect);
}

void UIContext::endFrame()
{
    endGroup();
    m_painter->drawFrame( m_groupStack[m_groupIndex + 1].bounds, m_groupStack[m_groupIndex + 1].margin, 0);
}

bool UIContext::beginPanel(Rect & r, const char * text, bool * isUnfold, int flags, int style)
{
    Rect rt, ra;
    Rect rpanel = m_painter->getPanelRect(Rect(r.x, r.y), text, rt, ra);

    if (flags & GroupFlags_LayoutDefault)
        flags = GroupFlags_LayoutDefaultFallback;
    beginGroup( ( flags | GroupFlags_LayoutNoMargin |GroupFlags_LayoutNoSpace ) & GroupFlags_StartXMask , rpanel );
        
        Rect rect = m_groupStack[m_groupIndex].bounds;

        bool focus = hasFocus(rect);
        bool hover = isHover(rect);
        
        if (focus)
        {
            m_uiOnFocus = true;
           
            r.x += m_currentCursor.x - m_mouseButton[0].cursor.x;
            r.y += m_currentCursor.y - m_mouseButton[0].cursor.y;
            rect.x += m_currentCursor.x - m_mouseButton[0].cursor.x;
            rect.y += m_currentCursor.y - m_mouseButton[0].cursor.y;

            m_mouseButton[0].cursor	= m_currentCursor;
        }

        if (m_mouseButton[0].state & ButtonFlags_End && focus && overlap( Rect(rect.x + ra.x, rect.y + ra.y, ra.w, ra.h) , m_currentCursor))
        {
            if ( isUnfold )
                *isUnfold = !*isUnfold; 
        }
 
        m_painter->drawPanel(rect, text, rt, ra, *isUnfold, hover, focus, style);

        if (isUnfold && *isUnfold)
        {
            beginFrame( flags, Rect(0, 0, r.w, r.h) ); 
            return true;
        }
        else
        {
            endGroup();
            return false;
        }
}

void UIContext::endPanel()
{
    endFrame();
    endGroup();
}

void UIContext::doLabel(const Rect & r, const char * text, int style)
{
    Rect rt;
    int nbLines;
    Rect rect = placeRect(m_painter->getLabelRect(r, text, rt, nbLines));

    m_painter->drawLabel(rect, text, rt, nbLines, isHover(rect), style);
}

bool UIContext::doButton(const Rect & r, const char * text, bool * state, int style)
{
    Rect rt;
    Rect rect = placeRect(m_painter->getButtonRect(r, text, rt));

    bool focus = hasFocus(rect);
    bool hover = isHover(rect);
    bool isDown = false;
    
    if ( state )
    {
        isDown = *state;
    }
    else
    {
        isDown = (m_mouseButton[0].state & ButtonFlags_On) && hover && focus;
    }

    m_painter->drawButton(rect, text, rt, isDown, hover, focus, style);

    if (focus)
    {
        m_uiOnFocus = true;
    }

    if (m_mouseButton[0].state & ButtonFlags_End && focus && overlap(rect, m_currentCursor))
    {
        if ( state )
            *state = !*state; 
        return true;
    }

    return false;
}

bool UIContext::doCheckButton(const Rect & r, const char * text, bool * state, int style)
{
    Rect rt, rc;
    Rect rect = placeRect(m_painter->getCheckRect(r, text, rt, rc));

    bool focus = hasFocus(rect);
    bool hover = isHover(rect);
    
    m_painter->drawCheckButton(rect, text, rt, rc, ( state ) && (*state), hover, focus, style);

    if (hasFocus(rect))
    {
        m_uiOnFocus = true;
    }

    if (m_mouseButton[0].state & ButtonFlags_End && focus && overlap(rect, m_currentCursor))
    {
        if ( state )
            *state = !*state;
        return true;
    }

    return false;
}

bool UIContext::doRadioButton(int reference, const Rect & r, const char * text, int * selected, int style)
{
    Rect rr, rt;
    Rect rect = placeRect(m_painter->getRadioRect(r, text, rt, rr));
    
    bool focus = hasFocus(rect);
    bool hover = isHover(rect);
    
    m_painter->drawRadioButton(rect, text, rt, rr, (selected) && (reference == *selected), hover, focus, style);

    if (focus)
    {
        m_uiOnFocus = true;
    }

    if (m_mouseButton[0].state & ButtonFlags_End && focus && overlap(rect, m_currentCursor))
    {
        if (selected)
            *selected = reference;
        return true;
    }

    return false;
}

bool UIContext::doListItem(int index, const Rect & r, const char * text, int * selected, int style)
{
    Rect rt;
    Rect rect = placeRect(m_painter->getItemRect(r, text, rt));

    m_painter->drawListItem(rect, text, rt, (selected) && (index == *selected), isHover(rect), style);

    return isHover(rect);
}

bool UIContext::doListBox(const Rect & r, int numOptions, const char * options[], int * selected, int style)
{
    Rect ri;
    Rect rt;
    Rect rect = placeRect(m_painter->getListRect(r, numOptions, options, ri, rt));

    bool focus = hasFocus(rect);
    bool hover = isHover(rect);
    int hovered = -1;
    
    if ( hover )
    {
        hovered = numOptions - 1 - (m_currentCursor.y - (rect.y+ri.y)) / (ri.h);
    }
    
    int lSelected = -1;
    if (selected)
        lSelected = *selected;
    m_painter->drawListBox(rect, numOptions, options, ri, rt, lSelected, hovered, style);

    if (focus)
    {
        m_uiOnFocus = true;
    }

    if (m_mouseButton[0].state & ButtonFlags_End && focus && overlap(rect, m_currentCursor) && (lSelected != hovered) )
    {
        if (selected)
            *selected = hovered;
        return true;
    }

    return false;
}

bool UIContext::doComboBox(const Rect & r, int numOptions, const char * options[], int * selected, int style)
{
    // First get the rect of the COmbo box itself and do some test with it
    Rect rt, ra;
    Rect rect = placeRect(m_painter->getComboRect(r, numOptions, options, *selected, rt, ra));

    bool focus = hasFocus(rect);
    bool hover = isHover(rect);

    if (focus)
    {
        m_uiOnFocus = true;

        // then if the combo box has focus, we can look for the geometry of the optios frame
        Rect ro, ri, rit;
        ro = m_painter->getComboOptionsRect(rect, numOptions, options, ri, rit);

        int hovered = -1;
        bool hoverOptions = overlap(ro, m_currentCursor);
        if ( hoverOptions )
        {
            hovered = numOptions - 1 - (m_currentCursor.y - (ro.y+ri.y)) / (ri.h);
        }

        // draw combo anyway
        m_painter->drawComboBox( rect, numOptions, options, rt, ra, *selected, hover, focus, style);
        
        // draw options
        m_painter->drawComboOptions( ro, numOptions, options, ri, rit, *selected, hovered, hover, focus, style);

        
        // When the widget get the focus, cache the focus point
        if (!m_twoStepFocus)
        {
            if (hover && m_mouseButton[0].state & ButtonFlags_End)
            {
                m_focusPoint = m_mouseButton[0].cursor;
                m_twoStepFocus = true;                 
            }
        }
        else
        {
            // Else Release the 2level focus when left mouse down out or up anyway
            // replace the stored left mouse down pos with the focus point to keep focus
            // on this widget for the next widget rendered in the frame
            if (	!(hoverOptions || hover)	
                &&	(	m_mouseButton[0].state & ButtonFlags_Begin
                    ||	m_mouseButton[0].state & ButtonFlags_End))
            {
                m_twoStepFocus = false;
            }
            else if ( (hoverOptions || hover) && m_mouseButton[0].state & ButtonFlags_End)
            {
                m_mouseButton[0].cursor	= m_focusPoint;		
                m_twoStepFocus = false;
            }

            if (hoverOptions && m_mouseButton[0].state & ButtonFlags_Begin)
            {
                m_mouseButton[0].cursor	= m_focusPoint;		
            }
        }

        // On mouse left bouton up, then select it
        if ( (hovered > -1) && (hovered < numOptions) && (m_mouseButton[0].state & ButtonFlags_End ) )
        {
            *selected = hovered;
            return true;
        }
    }
    else
    {
        m_painter->drawComboBox( rect, numOptions, options, rt, ra, *selected, hover, focus, style);
    }

    return false;
}

bool UIContext::doHorizontalSlider(const Rect & r, float min, float max, float * value, int style)
{
    // Map current value to 0-1.
    float f = (*value - min) / (max - min);
    if (f < 0.0f) f = 0.0f;
    else if (f > 1.0f) f = 1.0f;

    Rect rs;
    Rect rc;
    Rect rect = placeRect(m_painter->getHorizontalSliderRect( r, rs, f, rc ));
  
    bool changed = false;
    if (hasFocus(rect))
    {
        m_uiOnFocus = true;

        int xs = rect.x + rs.x + rc.w / 2;
        int x = m_currentCursor.x - xs;

        if (x < 0) x = 0;
        else if (x > rs.w) x = rs.w;

        rc.x = x;

        float f = (float(x)) / ((float) rs.w);
        f = f * (max - min) + min;

        if (fabs(f - *value) > (max - min) * 0.01f) 
        {
            changed = true;
            *value = f;
        }
    }

    m_painter->drawHorizontalSlider(rect, rs, f, rc, isHover(rect), style);

    return changed;
}

bool UIContext::doLineEdit(const Rect & r, char * text, int maxTextLength, int * nbCharsReturned, int style)
{
    Rect rt;
    Rect rect = placeRect(m_painter->getLineEditRect(r, text, rt));

    bool focus = hasFocus(rect);
    bool hover = isHover(rect);
    bool result = false;
    int carretPos = -1;

    if (focus)
    {
        m_uiOnFocus = true;
        
        // When the widget get the focus, cache the focus point
        if (!m_twoStepFocus)
        {
            m_twoStepFocus = true;
            m_focusPoint = Point(rect.x, rect.y);
        }
        else
        {
            // Else Release the 2level focus when left mouse down out or up anyway
            // replace the stored left mouse down pos with the focus point to keep focus
            // on this widget for the next widget rendered in the frame
            if (	!(hover)	
                &&	(	m_mouseButton[0].state & ButtonFlags_Begin
                    ||	m_mouseButton[0].state & ButtonFlags_End))
            {
                m_twoStepFocus = false;
                m_focusCaretPos = -1;
            }

            if (hover && m_mouseButton[0].state & ButtonFlags_Begin)
            {
                m_mouseButton[0].cursor	= m_focusPoint;		
            }
        }

        // Eval caret pos on every click hover
        if ( hover && m_mouseButton[0].state & ButtonFlags_Begin )
            m_focusCaretPos = m_painter->getPickedCharNb(text, Point( m_currentCursor.x - rt.x - rect.x, m_currentCursor.y - rt.y - rect.y));


        // If keys are buffered, apply input to the edited text
        if ( m_nbKeys )
        {
            int textLength = (int)strlen(text);
    
            if (m_focusCaretPos == -1)
                m_focusCaretPos = textLength;

            int nbKeys = m_nbKeys;
            int keyNb = 0;
            while (nbKeys)
            {
                // filter for special keys
                // Enter, quit edition
                if ( m_keyBuffer[keyNb] == '\r' )
                {
                    nbKeys = 1;
                    m_twoStepFocus = false;
                    m_focusCaretPos = -1;
                }
                // Special keys
                else if ( m_keyBuffer[keyNb] >= Key_F1 )
                {
                    switch (m_keyBuffer[keyNb])
                    {
                    case Key_Left :
                        {
                            // move cursor left one char
                            if (m_focusCaretPos > 0)
                                m_focusCaretPos--;
                        } 
                        break;
                    case Key_Right :
                        {
                            // move cursor right one char
                            if (m_focusCaretPos < textLength)
                                m_focusCaretPos++;
                        } 
                        break;
                    case Key_Home :
                        {
                            m_focusCaretPos = 0;
                        } 
                        break;
                    case Key_End :
                        {
                            m_focusCaretPos = textLength;
                        } 
                        break;
                    case Key_Insert :
                        {
                        } 
                        break;
                    default :
                        {
                            // strange key pressed...
                            m_focusCaretPos--;
                        } 
                        break;
                    }
                }	
                // Delete, move the chars >= carret back 1, carret stay in place
                else if ( m_keyBuffer[keyNb] == 127 )
                {
                    if (m_focusCaretPos < textLength)
                    {
                        memmove( text + m_focusCaretPos, text + m_focusCaretPos +1, textLength - m_focusCaretPos);
                        textLength--; 
                    }
                }
                // Backspace, move the chars > carret back 1, carret move back 1
                else if ( m_keyBuffer[keyNb] == '\b' )
                {
                    if (m_focusCaretPos > 0)
                    {
                        if (m_focusCaretPos != textLength)
                        {
                            memmove( text + m_focusCaretPos - 1, text + m_focusCaretPos, textLength - m_focusCaretPos);
                        }
                        m_focusCaretPos--;
                        textLength--; 
                    }
                }
                // Regular char, append it to the edited string
                else if ( textLength < maxTextLength)
                {
                    if (m_focusCaretPos != textLength)
                    {
                        memmove( text + m_focusCaretPos + 1, text + m_focusCaretPos, textLength - m_focusCaretPos);
                    }
                    text[m_focusCaretPos] = m_keyBuffer[keyNb];
                    m_focusCaretPos++;
                    textLength++;
                }
                keyNb++;
                nbKeys--;
            }
            text[textLength] = '\0';
            *nbCharsReturned = textLength;
            result = true;
        }
        carretPos = m_focusCaretPos;
    }
    m_painter->drawLineEdit(rect, text, rt, carretPos, focus, hover, style);

    return result;
}

void UIContext::setCursor(int x, int y)
{
    m_currentCursor.x = x;
    m_currentCursor.y = m_window.h - y;
}

bool UIContext::overlap(const Rect & rect, const Point & p)
{
    return p.x >= rect.x && p.x < rect.x + rect.w && 
        p.y >= rect.y && p.y < rect.y + rect.h;
}

// Left mouse button down, and down cursor inside rect.
bool UIContext::hasFocus(const Rect & rect)
{
    if (m_twoStepFocus)
    {
        return overlap(rect, m_focusPoint);
    }
    else
    {
        return (m_mouseButton[0].state & ButtonFlags_On) && 
            overlap(rect, m_mouseButton[0].cursor);
    }
}

bool UIContext::isHover(const Rect & rect)
{
    if (m_uiOnFocus && !hasFocus(rect)) return false;
    return overlap(rect, m_currentCursor);
}


void UIContext::doTextureView(const Rect & r, const void* texID, Rect & zoomRect, int mipLevel,
							  float texelScale, float texelOffset, int red, int green, int blue, int alpha,
                              int style)
{
    Rect rt;
    Rect rect = placeRect(m_painter->getTextureViewRect(r, rt));
    
    if ((zoomRect.w == 0) || (zoomRect.h == 0))
        zoomRect = Rect( 0, 0, rt.w, rt.h);

    m_painter->drawTextureView( rect, texID, rt, zoomRect, mipLevel, texelScale, texelOffset, red, green, blue, alpha, style );
}


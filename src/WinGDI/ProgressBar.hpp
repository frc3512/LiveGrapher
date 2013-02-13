//=============================================================================
//File Name: ProgressBar.hpp
//Description: Provides an interface to a progress bar with WinGDI
//Author: Tyler Veness
//=============================================================================

#ifndef PROGRESSBAR_HPP
#define PROGRESSBAR_HPP

#include "RectangleShape.hpp"
#include "Text.hpp"
#include <string>

class ProgressBar : public RectangleShape {
public:
    ProgressBar( HWND parentWindow , const Vector2i& position , std::wstring message , COLORREF fullFillColor , COLORREF emptyFillColor , COLORREF outlineColor , float percentFull = 0.f );

    void setPercent( float percentFull );
    float getPercent();

    void setPosition( const Vector2i& position );
    void setPosition( short x , short y );

    void setSize( const Vector2i& size );
    void setSize( short width , short height );

    void setString( const std::wstring& message );
    const std::wstring& getString();

    void setBarFillColor( COLORREF fill );
    COLORREF getBarFillColor();

    void draw( PAINTSTRUCT* ps );

private:
    RectangleShape m_barFill;

    float percent;

    Text m_text;
};

#endif // PROGRESSBAR_HPP

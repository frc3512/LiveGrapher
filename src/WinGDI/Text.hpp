//=============================================================================
//File Name: Text.hpp
//Description: Provides a wrapper for WinGDI text
//Author: Tyler Veness
//=============================================================================

/* In this class's case, outlineColor is actually the background color of the
 * text
 */

#ifndef TEXT_HPP
#define TEXT_HPP

#include "Drawable.hpp"
#include <string>

class Text : public Drawable {
public:
    Text( HWND parentWindow , const Vector2i& position , HFONT font , COLORREF fillColor , COLORREF outlineColor );

    void setFont( HFONT font );

    const HFONT getFont() const;

    void setString( std::wstring text );

    const std::wstring& getString();

    void draw( PAINTSTRUCT* ps );

private:
    // The following functions don't do anything
    /* void setSize( const Vector& size );
     * void setSize( short width , short height );

     * const Vector& getSize();

     * void setOutlineThickness( int thickness );
     */

    HFONT m_font;
    std::wstring m_string;
};

#endif // TEXT_HPP

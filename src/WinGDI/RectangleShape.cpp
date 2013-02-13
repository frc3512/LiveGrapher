//=============================================================================
//File Name: RectangleShape.cpp
//Description: Provides a wrapper for WinGDI rectangles
//Author: Tyler Veness
//=============================================================================

#include "RectangleShape.hpp"

#include <wingdi.h>

RectangleShape::RectangleShape( HWND parentWindow , const Vector2i& position , const Vector2i& size , COLORREF fillColor , COLORREF outlineColor , int outlineThickness ) :
        Drawable( parentWindow , position , size , fillColor , outlineColor , outlineThickness ) {
}

void RectangleShape::draw( PAINTSTRUCT* ps ) {
    HDC hdc = ps->hdc;

    // Set up the device context for drawing this shape
    HPEN oldPen = (HPEN)SelectObject( hdc , CreatePen( PS_SOLID , Drawable::getOutlineThickness() , Drawable::getOutlineColor() ) );
    HBRUSH oldBrush = (HBRUSH)SelectObject( hdc , CreateSolidBrush( Drawable::getFillColor() ) );

    // Draw rectangle
    Rectangle( hdc ,
            getBoundingRect().left - getOutlineThickness() ,
            getBoundingRect().top - getOutlineThickness() ,
            getBoundingRect().right + getOutlineThickness() ,
            getBoundingRect().bottom + getOutlineThickness()
            );

    // Replace old pen
    oldPen = (HPEN)SelectObject( hdc , oldPen );

    // Replace old brush
    oldBrush = (HBRUSH)SelectObject( hdc , oldBrush );

    // Free newly created objects
    DeleteObject( oldPen );
    DeleteObject( oldBrush );
}

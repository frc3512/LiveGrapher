//=============================================================================
//File Name: RectangleShape.hpp
//Description: Provides a wrapper for WinGDI rectangles
//Author: Tyler Veness
//=============================================================================

#ifndef RECTANGLE_SHAPE_HPP
#define RECTANGLE_SHAPE_HPP

#include "Drawable.hpp"

class RectangleShape : public Drawable {
public:
    RectangleShape( HWND parentWindow , const Vector2i& position , const Vector2i& size , COLORREF fillColor , COLORREF outlineColor , int outlineThickness );

    void draw( PAINTSTRUCT* ps );
};

#endif // RECTANGLE_SHAPE_HPP

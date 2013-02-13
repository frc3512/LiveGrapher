//=============================================================================
//File Name: Vector.hpp
//Description: Holds definitions for ProgressBar class
//Author: Tyler Veness
//=============================================================================

#ifndef VECTOR_HPP
#define VECTOR_HPP

template<class T , class U>
struct Vector {
    Vector( T x , U y ) {
        X = x;
        Y = y;
    }

    short X;
    short Y;
};

typedef Vector<int , int> Vector2i;

#endif // VECTOR_HPP

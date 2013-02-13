//=============================================================================
//File Name: UIFont.hpp
//Description: Provides a collection of fonts for use by other classes
//Author: Tyler Veness
//=============================================================================

/* Each font's size assumes a 1:1 relationship between logical units and pixels
 * in the device context. Use 'SetMapMode( hdc , MM_TEXT )' to achieve this.
 */

#ifndef UIFONT_HPP
#define UIFONT_HPP

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class UIFont {
public:
    // @return a global instance of the fonts available
    static UIFont* getInstance();
    static void freeInstance();

    const HFONT segoeUI14();
    const HFONT segoeUI18();

protected:
    UIFont();

private:
    static UIFont* m_instance;

    HFONT m_segoeUI14;
    HFONT m_segoeUI18;

    // Disallow copy and assignment
    UIFont ( const UIFont& );
    void operator=( const UIFont& );
};

#endif // UIFONT_HPP

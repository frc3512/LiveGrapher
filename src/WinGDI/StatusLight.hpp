//=============================================================================
//File Name: StatusLight.hpp
//Description: Shows green, yellow, or red circle depending on its internal
//             state
//Author: Tyler Veness
//=============================================================================

// TODO Add color-blind mode

#ifndef STATUS_LIGHT_HPP
#define STATUS_LIGHT_HPP

#include "Drawable.hpp"
#include "Text.hpp"
#include <string>

class StatusLight : public Drawable {
public:
    enum Status {
        active,
        standby,
        inactive
    };

    StatusLight( HWND parentWindow , const Vector2i& position , std::wstring message , Status currentStatus = StatusLight::inactive );

    void setActive( Status newStatus );
    Status getActive();

    void setPosition( const Vector2i& position );
    void setPosition( short x , short y );

    void setString( const std::wstring& message );
    const std::wstring& getString();

    void draw( PAINTSTRUCT* ps );

private:
    Status m_status;

    Text m_text;
};

#endif // STATUS_LIGHT_HPP

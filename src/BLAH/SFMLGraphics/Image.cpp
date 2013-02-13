////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2012 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

/* !!! THIS IS AN EXTREMELY ALTERED AND PURPOSE-BUILT VERSION OF SFML !!!
 * This distribution is designed to possess only a limited subset of the
 * original library's functionality and to only build on Win32 platforms.
 * The original distribution of this software has many more features and
 * supports more platforms.
 */

////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "../SFML/Graphics/Image.hpp"
#include "ImageLoader.hpp"
#include "../SFML/System/Err.hpp"
#include <algorithm>
#include <cstring>


namespace sf
{
////////////////////////////////////////////////////////////
Image::Image() :
m_size(0, 0)
{

}


////////////////////////////////////////////////////////////
void Image::create(unsigned int width, unsigned int height, const uint8_t* pixels)
{
    if (pixels && width && height)
    {
        // Assign the new size
        m_size.x = width;
        m_size.y = height;

        // Copy the pixels
        std::size_t size = width * height * 4;
        m_pixels.resize(size);
        std::memcpy(&m_pixels[0], pixels, size); // faster than vector::assign
    }
    else
    {
        // Create an empty image
        m_size.x = 0;
        m_size.y = 0;
        m_pixels.clear();
    }
}


////////////////////////////////////////////////////////////
bool Image::loadFromFile(const std::string& filename)
{
    return priv::ImageLoader::getInstance().loadImageFromFile(filename, m_pixels, m_size);
}


////////////////////////////////////////////////////////////
bool Image::loadFromMemory(const void* data, std::size_t size)
{
    return priv::ImageLoader::getInstance().loadImageFromMemory(data, size, m_pixels, m_size);
}


////////////////////////////////////////////////////////////
bool Image::saveToFile(const std::string& filename) const
{
    return priv::ImageLoader::getInstance().saveImageToFile(filename, m_pixels, m_size);
}

////////////////////////////////////////////////////////////
Vector2<unsigned int> Image::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
void Image::setPixel(unsigned int x, unsigned int y, const Color& color)
{
    uint8_t* pixel = &m_pixels[(x + y * m_size.x) * 4];
    *pixel++ = color.r;
    *pixel++ = color.g;
    *pixel++ = color.b;
    *pixel++ = color.a;
}


////////////////////////////////////////////////////////////
Color Image::getPixel(unsigned int x, unsigned int y) const
{
    const uint8_t* pixel = &m_pixels[(x + y * m_size.x) * 4];
    return Color(pixel[0], pixel[1], pixel[2], pixel[3]);
}


////////////////////////////////////////////////////////////
const uint8_t* Image::getPixelsPtr() const
{
    if (!m_pixels.empty())
    {
        return &m_pixels[0];
    }
    else
    {
        err() << "Trying to access the pixels of an empty image" << std::endl;
        return NULL;
    }
}

} // namespace sf

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
#include "ImageLoader.hpp"
#include "../SFML/System/Err.hpp"
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
#include <cctype>


namespace
{
    // Convert a string to lower case
    std::string toLower(std::string str)
    {
        for (std::string::iterator i = str.begin(); i != str.end(); ++i)
            *i = static_cast<char>(std::tolower(*i));
        return str;
    }
}


namespace sf
{
namespace priv
{
////////////////////////////////////////////////////////////
ImageLoader& ImageLoader::getInstance()
{
    static ImageLoader Instance;

    return Instance;
}


////////////////////////////////////////////////////////////
ImageLoader::ImageLoader()
{
    // Nothing to do
}


////////////////////////////////////////////////////////////
ImageLoader::~ImageLoader()
{
    // Nothing to do
}


////////////////////////////////////////////////////////////
bool ImageLoader::loadImageFromFile(const std::string& filename, std::vector<uint8_t>& pixels, Vector2<unsigned int>& size)
{
    // Clear the array (just in case)
    pixels.clear();

    // Load the image and get a pointer to the pixels in memory
    int width, height, channels;
    unsigned char* ptr = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (ptr && width && height)
    {
        // Assign the image properties
        size.x = width;
        size.y = height;

        // Copy the loaded pixels to the pixel buffer
        pixels.resize(width * height * 4);
        memcpy(&pixels[0], ptr, pixels.size());

        // Free the loaded pixels (they are now in our own pixel buffer)
        stbi_image_free(ptr);

        return true;
    }
    else
    {
        // Error, failed to load the image
        err() << "Failed to load image \"" << filename << "\". Reason : " << stbi_failure_reason() << std::endl;

        return false;
    }
}


////////////////////////////////////////////////////////////
bool ImageLoader::loadImageFromMemory(const void* data, std::size_t dataSize, std::vector<uint8_t>& pixels, Vector2<unsigned int>& size)
{
    // Check input parameters
    if (data && dataSize)
    {
        // Clear the array (just in case)
        pixels.clear();

        // Load the image and get a pointer to the pixels in memory
        int width, height, channels;
        const unsigned char* buffer = static_cast<const unsigned char*>(data);
        unsigned char* ptr = stbi_load_from_memory(buffer, static_cast<int>(dataSize), &width, &height, &channels, STBI_rgb_alpha);

        if (ptr && width && height)
        {
            // Assign the image properties
            size.x = width;
            size.y = height;

            // Copy the loaded pixels to the pixel buffer
            pixels.resize(width * height * 4);
            memcpy(&pixels[0], ptr, pixels.size());

            // Free the loaded pixels (they are now in our own pixel buffer)
            stbi_image_free(ptr);

            return true;
        }
        else
        {
            // Error, failed to load the image
            err() << "Failed to load image from memory. Reason : " << stbi_failure_reason() << std::endl;

            return false;
        }
    }
    else
    {
        err() << "Failed to load image from memory, no data provided" << std::endl;
        return false;
    }
}


////////////////////////////////////////////////////////////
bool ImageLoader::saveImageToFile(const std::string& filename, const std::vector<uint8_t>& pixels, const Vector2<unsigned int>& size)
{
    // Make sure the image is not empty
    if (!pixels.empty() && (size.x > 0) && (size.y > 0))
    {
        // Deduce the image type from its extension
        if (filename.size() > 3)
        {
            // Extract the extension
            std::string extension = filename.substr(filename.size() - 3);

            if (toLower(extension) == "bmp")
            {
                // BMP format
                if (stbi_write_bmp(filename.c_str(), size.x, size.y, 4, &pixels[0]))
                    return true;
            }
            else if (toLower(extension) == "tga")
            {
                // TGA format
                if (stbi_write_tga(filename.c_str(), size.x, size.y, 4, &pixels[0]))
                    return true;
            }
            else if(toLower(extension) == "png")
            {
                // PNG format
                if (stbi_write_png(filename.c_str(), size.x, size.y, 4, &pixels[0], 0))
                    return true;
            }
        }
    }

    err() << "Failed to save image \"" << filename << "\"" << std::endl;
    return false;
}


} // namespace priv

} // namespace sf

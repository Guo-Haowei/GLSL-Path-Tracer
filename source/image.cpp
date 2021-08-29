#include "image.h"

#include <stdexcept>

#include "com_file.h"
#include "utility/string_util.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"

namespace pt {

using std::runtime_error;

void WritePng( const char* path, const void* data, int width, int height, int component )
{
    stbi_flip_vertically_on_write( true );
    stbi_write_png( path, width, height, component, data, component * width );
}

void WritePng( const std::string& path, const void* data, int width, int height, int component )
{
    WritePng( path.c_str(), data, width, height, component );
}

Image ReadImage( const char* path )
{
    Image image;
    unsigned char* data = stbi_load( path, &image.width, &image.height, &image.channel, 3 );
    if ( !data )
    {
        throw runtime_error( va( "Failed to open image '%s'", path ) );
    }

    if ( image.channel != 3 )
    {
        throw runtime_error( va( "Failed to read image '%s' with channel %d", path, image.channel ) );
    }

    image.data       = data;
    image.sizeInByte = sizeof( unsigned char ) * image.width * image.height * image.channel;
    image.type       = Image::R8G8B8;
    return image;
}

Image ReadImage( const std::string& path )
{
    return ReadImage( path.c_str() );
}

Image ReadHDRImage( const char* path )
{
    Image image;
    float* data = stbi_loadf( path, &image.width, &image.height, &image.channel, 0 );
    if ( !data )
    {
        throw runtime_error( va( "Failed to open image '%s'", path ) );
    }

    image.data       = data;
    image.sizeInByte = sizeof( float ) * image.width * image.height * image.channel;
    image.type       = Image::Float;
    return image;
}

Image ReadHDRImage( const std::string& path )
{
    return ReadHDRImage( path.c_str() );
}

}  // namespace pt

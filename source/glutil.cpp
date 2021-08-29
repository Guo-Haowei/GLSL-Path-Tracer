#include "glutil.h"

#include <stdexcept>

#include "com_dvars.h"
#include "geomath/geometry.h"
#include "image.h"
#include "universal/dvar_api.h"
#include "utility/string_util.h"

namespace pt::gl {

using std::runtime_error;

void BindSSBOToSlot( GLuint ssbo, GLuint slot )
{
    glBindBufferBase( GL_SHADER_STORAGE_BUFFER, slot, ssbo );
}

void CreateQuadVao( GLuint& outVao, GLuint& outVbo )
{
    glCreateVertexArrays( 1, &outVao );
    glCreateBuffers( 1, &outVbo );

    constexpr float s = 1.0f;
    vec2 vertices[]   = {
        { -s, +s },
        { -s, -s },
        { +s, -s },
        { -s, +s },
        { +s, -s },
        { +s, +s }
    };

    glBindVertexArray( outVao );
    glBindBuffer( GL_ARRAY_BUFFER, outVbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), vertices, GL_STATIC_DRAW );
    glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, sizeof( vec2 ), (void*)0 );
    glEnableVertexAttribArray( 0 );
}

GLuint Create3DTexture( const ImageArray& images )
{
    GLuint textureId;
    glGenTextures( 1, &textureId );
    glBindTexture( GL_TEXTURE_2D_ARRAY, textureId );

    int num( images.images.size() );
    const Image& image = images.images.front();
    struct RGB {
        unsigned char r, g, b;
    };

    const size_t perImageOffset = sizeof( RGB ) * images.maxWidth * images.maxHeight;
    RGB* data                   = (RGB*)calloc( 1, perImageOffset * num );

    for ( size_t idx = 0; idx < images.images.size(); ++idx )
    {
        const Image& image = images.images[idx];

        for ( int y = 0; y < image.height; ++y )
        {
            for ( int x = 0; x < image.width; ++x )
            {
                RGB rgb                                                                   = ( reinterpret_cast<RGB*>( image.data ) )[image.width * y + x];
                data[idx * images.maxHeight * images.maxHeight + y * images.maxWidth + x] = rgb;
            }
        }
    }

    glTexImage3D( GL_TEXTURE_2D_ARRAY,
                  0,                 // mipmap level
                  GL_RGBA8,          // gpu texel format
                  images.maxWidth,   // width
                  images.maxHeight,  // height
                  num,               // depth
                  0,                 // border
                  GL_RGB,            // cpu pixel format
                  GL_UNSIGNED_BYTE,  // cpu pixel coord type
                  data );            // pixel data

    glTexParameteri( GL_TEXTURE_2D_ARRAY,
                     GL_TEXTURE_MIN_FILTER,
                     GL_NEAREST_MIPMAP_LINEAR );
    glTexParameteri( GL_TEXTURE_2D_ARRAY,
                     GL_TEXTURE_MAG_FILTER,
                     GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT );
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap( GL_TEXTURE_2D_ARRAY );

    free( data );

    return textureId;
}

GLuint CreateOutputTextureAndBind( int width, int height )
{
    GLuint textureId;
    glGenTextures( 1, &textureId );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, textureId );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0 );
    glBindImageTexture( 0, textureId, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F );
    return textureId;
}

void SaveScreenshot()
{
    const int w         = Dvar_GetInt( wnd_width );
    const int h         = Dvar_GetInt( wnd_height );
    unsigned char* data = static_cast<unsigned char*>( calloc( 1, w * h * 4 ) );
    if ( !data )
    {
        return;
    }

    glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, data );
    WritePng( "my.png", data, w, h, 3 );
    free( data );
}

GLuint CreateEnvTexture( const char* path, Image& outImage )
{
    outImage = ReadHDRImage( path );
    GLenum imageFormat;
    switch ( outImage.channel )
    {
        case 4:
            imageFormat = GL_RGBA;
            break;
        case 3:
            imageFormat = GL_RGB;
            break;
        case 2:
            imageFormat = GL_RG;
            break;
        case 1:
            imageFormat = GL_RED;
            break;
        default:
            throw runtime_error( va(
                "Invalid texture channel '%d' detected in image '%s'",
                outImage.channel,
                path ) );
    }

    GLenum dataType;
    switch ( outImage.type )
    {
        case Image::Float:
            dataType = GL_FLOAT;
            break;
        default:
            throw runtime_error( va(
                "Invalid texture type detected in image '%s'",
                path ) );
    }

    GLuint textureId;
    glGenTextures( 1, &textureId );
    glBindTexture( GL_TEXTURE_2D, textureId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, outImage.width, outImage.height, 0, imageFormat, dataType, outImage.data );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glGenerateMipmap( GL_TEXTURE_2D );
    return textureId;
}

GLuint CreateEnvTexture( const std::string& path, Image& outImage )
{
    return CreateEnvTexture( path.c_str(), outImage );
}

}  // namespace pt::gl

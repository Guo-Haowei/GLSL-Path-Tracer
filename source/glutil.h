#pragma once
#include <string>
#include <vector>

#include "glad/glad.h"
#include "image.h"

namespace pt::gl {

template<typename T>
GLuint CreateSSBO( const std::vector<T> buffer )
{
    const size_t sizeInByte = sizeof( T ) * buffer.size();
    GLuint ssbo;
    glGenBuffers( 1, &ssbo );
    glBindBuffer( GL_SHADER_STORAGE_BUFFER, ssbo );
    glBufferData( GL_SHADER_STORAGE_BUFFER, sizeInByte, buffer.data(), GL_STATIC_DRAW );
    glBindBuffer( GL_SHADER_STORAGE_BUFFER, 0 );
    return ssbo;
}

void BindSSBOToSlot( GLuint ssbo, GLuint slot );

void CreateQuadVao( GLuint& outVao, GLuint& outVbo );

GLuint Create3DTexture( const ImageArray& images );

GLuint CreateOutputTextureAndBind( int width, int height );

GLuint CreateEnvTexture( const char* path, Image& outImage );

GLuint CreateEnvTexture( const std::string& path, Image& outImage );

void SaveScreenshot();

}  // namespace pt::gl

#pragma once
#include <string>
#include <vector>

namespace pt {

struct Image {
    enum Type {
        Float,
        R8G8B8,
    };

    int width, height;
    int channel;
    size_t sizeInByte;
    Type type;
    void* data;
    std::string debugName;
};

struct ImageArray {
    std::vector<Image> images;
    int maxWidth  = 0;
    int maxHeight = 0;
};

void WritePng( const char* path, const void* data, int width, int height, int component );

void WritePng( const std::string& path, const void* data, int width, int height, int component );

Image ReadImage( const char* path );

Image ReadImage( const std::string& path );

Image ReadHDRImage( const char* path );

Image ReadHDRImage( const std::string& path );

}  // namespace pt

#include "string_util.h"

#include <cstdarg>

namespace pt {

const char* va( const char* format, ... )
{
    constexpr size_t bufferSize = 1024;
    static char s_buffer[bufferSize];
    va_list args;

    va_start( args, format );
    vsnprintf( s_buffer, bufferSize, format, args );
    va_end( args );
    return s_buffer;
}

}  // namespace pt

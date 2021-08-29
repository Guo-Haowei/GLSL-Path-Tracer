#pragma once
#include <string>
#include <vector>

#include "com_file.h"
#include "glad/glad.h"

namespace pt::gl {

void InitGraphics();
void FinalizeGraphics();

static constexpr GLuint NullHandle            = 0;
static constexpr GLint InvalidUniformLocation = 0;

//------------------------------------------------------------------------------
// GpuResource
//------------------------------------------------------------------------------
class GpuResource {
   public:
    GpuResource();

    inline GLuint GetHandle() const { return m_handle; }

   protected:
    GLuint m_handle;
};

//------------------------------------------------------------------------------
// Program
//------------------------------------------------------------------------------
class Program : public GpuResource {
   public:
    enum class Kind {
        Unknown,
        Compute,
        Rasterize,
    };

    struct CreateInfo {
        Kind kind;
        const char* vert;
        const char* frag;
        const char* comp;
        DefineList defines;

        CreateInfo()
            : kind( Kind::Unknown ), vert( nullptr ), frag( nullptr ), comp( nullptr ) {}
    };

    ~Program();

    void Use();
    void Stop();
    void Create( CreateInfo info );
    void Link( const char* debugInfo );
    int GetUniformLoc( const char* loc );
    int GetAttribLoc( const char* loc );
};

}  // namespace pt::gl

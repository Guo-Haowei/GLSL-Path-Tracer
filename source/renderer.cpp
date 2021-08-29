#include "renderer.h"

#include <cassert>
#include <iostream>
#include <stdexcept>

#include "common.h"

namespace pt::gl {

using std::string;

static constexpr int LOG_SIZE_MAX = 1024;

//------------------------------------------------------------------------------
// GpuResource
//------------------------------------------------------------------------------
GpuResource::GpuResource()
    : m_handle( NullHandle )
{
}

//------------------------------------------------------------------------------
// Shader
//------------------------------------------------------------------------------
class Shader : public GpuResource {
   public:
    ~Shader();
    void CreateFromFile( const char *path, GLenum type, const DefineList &defines );
};

void Shader::CreateFromFile( const char *path, GLenum type, const DefineList &defines )
{
    constexpr char GLSL_VERSION_STRING[] = "#version 460 core\n";

    m_handle              = glCreateShader( type );
    string source         = PreprocessFile( path, defines );
    const char *sources[] = { GLSL_VERSION_STRING, source.c_str() };
    glShaderSource( m_handle, ARRAYSIZE( sources ), sources, NULL );
    glCompileShader( m_handle );
    int success;
    char log[LOG_SIZE_MAX];
    glGetShaderiv( m_handle, GL_COMPILE_STATUS, &success );
    if ( !success )
    {
        glGetShaderInfoLog( m_handle, LOG_SIZE_MAX, NULL, log );
        log[LOG_SIZE_MAX - 1] = '\0';  // prevent overflow
        std::string error( "Failed to compile shader [" );
        error.append( path ).append( "]\n" ).append( std::string( 80, '-' ) ).push_back( '\n' );
        error.append( log ).append( std::string( 80, '-' ) );
        throw std::runtime_error( error );
    }
}

Shader::~Shader()
{
    if ( m_handle )
    {
        glDeleteShader( m_handle );
        m_handle = NullHandle;
    }
}

//------------------------------------------------------------------------------
// Program
//------------------------------------------------------------------------------

void Program::Use()
{
    glUseProgram( m_handle );
}

void Program::Stop()
{
    glUseProgram( NullHandle );
}

Program::~Program()
{
    if ( m_handle )
    {
        glDeleteProgram( m_handle );
        m_handle = NullHandle;
    }
}

void Program::Create( Program::CreateInfo info )
{
    assert( m_handle == NullHandle );

    m_handle = glCreateProgram();
    if ( info.kind == Program::Kind::Compute )
    {
        Shader computeShader;
        computeShader.CreateFromFile( info.comp, GL_COMPUTE_SHADER, info.defines );
        glAttachShader( m_handle, computeShader.GetHandle() );
        Link( info.comp );
    }
    else if ( info.kind == Program::Kind::Rasterize )
    {
        Shader vertexShader;
        vertexShader.CreateFromFile( info.vert, GL_VERTEX_SHADER, info.defines );
        glAttachShader( m_handle, vertexShader.GetHandle() );
        Shader fragmentShader;
        fragmentShader.CreateFromFile( info.frag, GL_FRAGMENT_SHADER, info.defines );
        glAttachShader( m_handle, fragmentShader.GetHandle() );
        Link( info.vert );
    }
    else
    {
        throw std::runtime_error( "Failed to create program" );
    }
}

void Program::Link( const char *debugInfo )
{
    glLinkProgram( m_handle );
    int success;
    char log[LOG_SIZE_MAX];
    glGetProgramiv( m_handle, GL_LINK_STATUS, &success );
    if ( !success )
    {
        glGetProgramInfoLog( m_handle, LOG_SIZE_MAX, NULL, log );
        log[LOG_SIZE_MAX - 1] = '\0';  // prevent overflow
        std::string error( "Failed to link shader [" );
        error.append( debugInfo ).append( "]\n" ).append( std::string( 80, '-' ) ).push_back( '\n' );
        error.append( log ).append( std::string( 80, '-' ) );
        throw std::runtime_error( error );
    }
}

int Program::GetUniformLoc( const char *loc )
{
    return glGetUniformLocation( m_handle, loc );
}

int Program::GetAttribLoc( const char *loc )
{
    return glGetAttribLocation( m_handle, loc );
}

//------------------------------------------------------------------------------

const unsigned char *g_Vender      = nullptr;
const unsigned char *g_Renderer    = nullptr;
const unsigned char *g_GLSLVersion = nullptr;

ComputeGroup g_ComputeGroup;

static void APIENTRY debugOutput( GLenum, GLenum, unsigned int, GLenum, GLsizei, const char *, const void * );

void InitGraphics()
{
    if ( gladLoadGL() == 0 )
    {
        throw std::runtime_error( "Failed to load opengl functions" );
    }

    int flags;
    glGetIntegerv( GL_CONTEXT_FLAGS, &flags );
    if ( flags & GL_CONTEXT_FLAG_DEBUG_BIT )
    {
        glEnable( GL_DEBUG_OUTPUT );
        glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
        glDebugMessageCallback( debugOutput, nullptr );
        glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE );
    }

    g_Vender      = glGetString( GL_VENDOR );
    g_Renderer    = glGetString( GL_RENDERER );
    g_GLSLVersion = glGetString( GL_SHADING_LANGUAGE_VERSION );

    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &g_ComputeGroup.x );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &g_ComputeGroup.y );
    glGetIntegeri_v( GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &g_ComputeGroup.z );
    // glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &invocation);
    glEnable( GL_CULL_FACE );
}

void FinalizeGraphics()
{
}

static void APIENTRY debugOutput(
    GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char *message,
    const void *userParam )
{
    // ignore non-significant error/warning codes
    if ( id == 131169 || id == 131185 || id == 131218 || id == 131204 )
        return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch ( source )
    {
        case GL_DEBUG_SOURCE_API:
            std::cout << "Source: API";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            std::cout << "Source: Window System";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            std::cout << "Source: Shader Compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            std::cout << "Source: Third Party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            std::cout << "Source: Application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
            std::cout << "Source: Other";
            break;
    }
    std::cout << std::endl;

    switch ( type )
    {
        case GL_DEBUG_TYPE_ERROR:
            std::cout << "Type: Error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cout << "Type: Deprecated Behaviour";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cout << "Type: Undefined Behaviour";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cout << "Type: Portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cout << "Type: Performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            std::cout << "Type: Marker";
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cout << "Type: Push Group";
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cout << "Type: Pop Group";
            break;
        case GL_DEBUG_TYPE_OTHER:
            std::cout << "Type: Other";
            break;
    }
    std::cout << std::endl;

    switch ( severity )
    {
        case GL_DEBUG_SEVERITY_HIGH:
            std::cout << "Severity: high";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            std::cout << "Severity: medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
            std::cout << "Severity: low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            std::cout << "Severity: notification";
            break;
    }
    std::cout << std::endl;
    std::cout << std::endl;

    __debugbreak();
}

}  // namespace pt::gl

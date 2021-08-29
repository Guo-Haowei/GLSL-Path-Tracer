#include "viewer.h"

#include "../third_party/imgui/imgui.h"
#include "application.h"
#include "com_dvars.h"
#include "common.h"
#include "glutil.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "renderer.h"
#include "scene_loader.h"
#include "universal/core_assert.h"
#include "universal/dvar_api.h"
#include "universal/print.h"
#include "utility/clock.h"
#include "utility/string_util.h"

// glfw3.h must be included after glad.h
#include <GLFW/glfw3.h>

#ifndef DATA_DIR
#define DATA_DIR ""
#endif
#ifdef min
#undef min
#endif  // min
#ifdef max
#undef max
#endif  // max

namespace pt {

using std::string;
using std::vector;

static gl::Program g_PhongProgram;
static gl::Program g_TiledRenderProgram;
static gl::Program g_FullScreenProgram;
extern gl::Program g_ImguiProgram;

static GLuint g_QuadVao;
static GLuint g_QuadVbo;
static GLuint g_GeomSsbo;
static GLuint g_BBoxSsbo;
static GLuint g_MatSsbo;

/// texture
static GLuint g_Texture;
static GLuint g_ConstantBuffer;
static GLuint g_EnvTexture;
static GLuint g_AlbedoTexture;

/// texture arrays
extern ImageArray g_AlbedoMaps;

static void SetTextureSamplerUniforms( gl::Program& program )
{
    program.Use();
    GLuint handle = program.GetHandle();
    glUniform1i( glGetUniformLocation( handle, "outTexture" ), 0 );
    glUniform1i( glGetUniformLocation( handle, "envTexture" ), 1 );
    glUniform1i( glGetUniformLocation( handle, "albedoTexture" ), 2 );
    program.Stop();
}

Viewer::Viewer()
{
    m_showGui    = true;
    m_dirty      = false;
    m_tileOffset = ivec2( 0 );
}

void Viewer::Initialize()
{
    const char* scene_path = Dvar_GetString( scene );
    const int width        = Dvar_GetInt( wnd_width );
    const int height       = Dvar_GetInt( wnd_height );

    core_assert( scene_path && scene_path[0] );
    core_assert( width > 0 );
    core_assert( height > 0 );

    Scene scene;
    if ( !LuaLoadScene( scene_path, scene ) )
    {
        Com_PrintFatal( "failed to execute script %s", scene_path );
        return;
    }

    GpuScene gpuScene;
    ConstructScene( scene, gpuScene );

    InitCamera( scene.camera, gpuScene.bbox );

    g_SceneStats.height  = gpuScene.height;
    g_SceneStats.geomCnt = static_cast<int>( gpuScene.geometries.size() );
    g_SceneStats.bboxCnt = static_cast<int>( gpuScene.bvhs.size() );

    CreateMainWindow( width, height );

    gl::InitGraphics();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL( GetInternalWindow(), true );
    ImGui_ImplOpenGL3_Init( "#version 460 core" );

    g_Texture = gl::CreateOutputTextureAndBind( width, height );

    Image image;
    g_EnvTexture = gl::CreateEnvTexture( DATA_DIR "env/stairs.hdr", image );
    free( image.data );

    g_AlbedoTexture = gl::Create3DTexture( g_AlbedoMaps );
    for ( auto& albedo : g_AlbedoMaps.images )
    {
        free( albedo.data );
    }

    // shaders
    {
        gl::Program::CreateInfo createInfo = {};
        createInfo.kind                    = gl::Program::Kind::Rasterize;
        createInfo.vert                    = DATA_DIR "shaders/imgui.vert";
        createInfo.frag                    = DATA_DIR "shaders/imgui.frag";
        g_ImguiProgram.Create( createInfo );
    }
    {
        gl::Program::CreateInfo createInfo = {};
        createInfo.kind                    = gl::Program::Kind::Rasterize;
        createInfo.vert                    = DATA_DIR "shaders/fullscreen.vert";
        createInfo.frag                    = DATA_DIR "shaders/fullscreen.frag";
        g_FullScreenProgram.Create( createInfo );

        SetTextureSamplerUniforms( g_FullScreenProgram );
    }
    {
        gl::Program::CreateInfo createInfo = {};
        createInfo.defines.push_back( Define{ "BVH_COUNT", std::any( g_SceneStats.bboxCnt ) } );
        createInfo.defines.push_back( Define{ "GEOM_COUNT", std::any( g_SceneStats.geomCnt ) } );
        createInfo.defines.push_back( Define{ "MATERIAL_COUNT", std::any( gpuScene.materials.size() ) } );
        createInfo.kind = gl::Program::Kind::Compute;
        createInfo.comp = DATA_DIR "shaders/tiled.comp";
        g_TiledRenderProgram.Create( createInfo );
        SetTextureSamplerUniforms( g_TiledRenderProgram );

        createInfo.comp = DATA_DIR "shaders/phong.comp";
        g_PhongProgram.Create( createInfo );
        SetTextureSamplerUniforms( g_PhongProgram );
    }

    // quad buffer
    gl::CreateQuadVao( g_QuadVao, g_QuadVbo );

    // constant buffer
    glGenBuffers( 1, &g_ConstantBuffer );
    glBindBufferBase( GL_UNIFORM_BUFFER, 0, g_ConstantBuffer );

    // ssbo buffer
    g_GeomSsbo = gl::CreateSSBO( gpuScene.geometries );
    gl::BindSSBOToSlot( g_GeomSsbo, 1 );
    g_BBoxSsbo = gl::CreateSSBO( gpuScene.bvhs );
    gl::BindSSBOToSlot( g_BBoxSsbo, 2 );
    g_MatSsbo = gl::CreateSSBO( gpuScene.materials );
    gl::BindSSBOToSlot( g_MatSsbo, 3 );

    m_lastTimestamp = GetMsSinceEpoch();
}

void Viewer::CopyCameraToCache()
{
    m_cache.camPos   = m_cam.pos;
    m_cache.camRight = m_cam.right;
    m_cache.camUp    = m_cam.up;
    m_cache.camFwd   = m_cam.fwd;
    m_cache.camFov   = m_cam.fov;
    m_cache.dirty    = m_dirty;
}

void Viewer::Finalize()
{
    glDeleteVertexArrays( 1, &g_QuadVao );
    glDeleteBuffers( 1, &g_QuadVbo );
    glDeleteBuffers( 1, &g_GeomSsbo );
    glDeleteBuffers( 1, &g_BBoxSsbo );
    glDeleteBuffers( 1, &g_MatSsbo );
    glDeleteTextures( 1, &g_Texture );

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    gl::FinalizeGraphics();
    DestroyMainWindow();
}

void Viewer::InitCamera( const SceneCamera& cam, const Box3& bbox )
{
    m_cam = Camera( cam );
    m_cam.CalcSpeed( bbox );
}

void Viewer::EnterRenderMode()
{
    m_state   = Render;
    m_showGui = false;
}

void Viewer::HandleInput( float deltaTime )
{
    m_state = Interactive;
    if ( Dvar_GetInt( ssp ) != 0 )
    {
        EnterRenderMode();
    }

    // TODO: set state back to interactive when finished rendering

    if ( ImGui::IsKeyReleased( GLFW_KEY_ESCAPE ) )
    {
        CloseWindow();
        return;
    }

    if ( ImGui::IsKeyDown( GLFW_KEY_GRAVE_ACCENT ) )
    {
        m_showGui = !m_showGui;
    }

    if ( ImGui::IsKeyDown( GLFW_KEY_LEFT_CONTROL ) )
    {
        if ( ImGui::IsKeyDown( GLFW_KEY_S ) )
        {
            gl::SaveScreenshot();
        }
        else if ( ImGui::IsKeyDown( GLFW_KEY_R ) )
        {
            // TODO: refactor
            Dvar_SetInt( ssp, 128 );
            m_state = Render;
        }
        return;
    }

    m_dirty = false;
    if ( m_state == Interactive )
    {
        UpdateCamera( deltaTime );
    }
}

void Viewer::UpdateCamera( float deltaTime )
{
    Camera& cam = m_cam;
    int dx      = ImGui::IsKeyDown( GLFW_KEY_D ) - ImGui::IsKeyDown( GLFW_KEY_A );
    int dy      = ImGui::IsKeyDown( GLFW_KEY_E ) - ImGui::IsKeyDown( GLFW_KEY_Q );
    int dz      = ImGui::IsKeyDown( GLFW_KEY_W ) - ImGui::IsKeyDown( GLFW_KEY_S );
    if ( dx || dy || dz )
    {
        vec3 delta =
            static_cast<float>( dx ) * cam.right +
            static_cast<float>( dy ) * vec3( 0, 1, 0 ) +
            static_cast<float>( dz ) * cam.fwd;

        float speed = cam.speed * deltaTime;
        speed *= ImGui::IsKeyDown( GLFW_KEY_LEFT_SHIFT ) ? 5.0f : 1.0f;
        cam.pos += speed * glm::normalize( delta );
        m_dirty = true;
    }

    int ry = ImGui::IsKeyDown( GLFW_KEY_RIGHT ) - ImGui::IsKeyDown( GLFW_KEY_LEFT );
    int rz = ImGui::IsKeyDown( GLFW_KEY_UP ) - ImGui::IsKeyDown( GLFW_KEY_DOWN );

    const float sensitivity = 0.001f * deltaTime;
    mat3 rotate;

    if ( ry )
    {
        rotate = glm::rotate( glm::mat4( 1 ), ry * sensitivity, vec3( 0, -1, 0 ) );
    }
    else if ( rz )
    {
        rotate = glm::rotate( glm::mat4( 1 ), rz * sensitivity, cam.right );
    }

    if ( ry || rz )
    {
        cam.fwd   = rotate * cam.fwd;
        cam.right = rotate * cam.right;
        cam.up    = rotate * cam.up;
        m_dirty   = true;
    }
}

void Viewer::DrawGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    if ( !m_showGui )
    {
        return;
    }

    ImGui::NewFrame();

    if ( ImGui::BeginMainMenuBar() )
    {
        if ( ImGui::BeginMenu( "Scene" ) )
        {
            if ( ImGui::MenuItem( "Load Scene", "Ctrl+L", false ) )
            {
            }
            if ( ImGui::MenuItem( "Render Scene", "Ctrl+R", false ) )
            {
            }
            if ( ImGui::MenuItem( "Save Screen Shot", "Ctrl+S", false ) )
            {
                gl::SaveScreenshot();
            }
            ImGui::EndMenu();
        }

        {
            ImGui::Begin( "Debug Window" );
            ImGui::Text( "Vendor: %s", gl::g_Vender );
            ImGui::Text( "Renderer: %s", gl::g_Renderer );
            ImGui::Text( "GLSL Version: %s", gl::g_GLSLVersion );
            ImGui::Text( "Compute Group x: %d, y: %d, z: %d",
                         gl::g_ComputeGroup.x,
                         gl::g_ComputeGroup.y,
                         gl::g_ComputeGroup.z );
            ImGui::Text( "Triangle Count: %d", g_SceneStats.geomCnt );
            ImGui::Text( "BBox Count: %d", g_SceneStats.bboxCnt );
            ImGui::Text( "Camera:" );
            const Camera& cam = m_cam;
            ImGui::Text( "  origin: %f, %f, %f", cam.pos.x, cam.pos.y, cam.pos.z );
            const vec3 lookAt = cam.pos + cam.fwd;
            ImGui::Text( "  lookAt: %f, %f, %f", lookAt.x, lookAt.y, lookAt.z );
            ImGui::End();
        }

        ImGui::Separator();
        ImGui::Text( "FPS: %.1f", ImGui::GetIO().Framerate );
        ImGui::EndMainMenuBar();
    }

    // Rendering
    ImGui::Render();
}

void Viewer::RenderGui()
{
    if ( !m_showGui )
    {
        return;
    }

    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );
}

void Viewer::Update()
{
    const int width  = Dvar_GetInt( wnd_width );
    const int height = Dvar_GetInt( wnd_height );

    const double currentTimestamp = GetMsSinceEpoch();
    float deltaTime               = static_cast<int>( currentTimestamp - m_lastTimestamp );
    deltaTime                     = glm::max( deltaTime, 1.0f );
    m_lastTimestamp               = currentTimestamp;
    HandleInput( deltaTime );

    DrawGui();

    // viewport
    int display_w, display_h;
    GetDisplaySize( &display_w, &display_h );
    glViewport( 0, 0, display_w, display_h );
    glClear( GL_COLOR_BUFFER_BIT );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, g_Texture );
    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, g_EnvTexture );
    glActiveTexture( GL_TEXTURE2 );
    glBindTexture( GL_TEXTURE_2D_ARRAY, g_AlbedoTexture );

    ++m_cache.frame;
    CopyCameraToCache();

    if ( GetState() == Viewer::Render )
    {
        g_TiledRenderProgram.Use();
        static int counter = 0;
        counter            = ( counter + 1 ) % Dvar_GetInt( ssp );

        const int tileSize = Dvar_GetInt( tile );

        m_cache.tileOffset = m_tileOffset;

        if ( counter == 0 )
        {
            m_tileOffset.x += tileSize;
            if ( m_tileOffset.x >= width )
            {
                m_tileOffset.y += tileSize;
                m_tileOffset.x = 0;
            }

            if ( m_tileOffset.y >= height )
            {
                // TODO: do something
            }
        }

        glNamedBufferData( g_ConstantBuffer, sizeof( ConstantBufferCache ), &m_cache, GL_DYNAMIC_DRAW );
        glDispatchCompute( tileSize, tileSize, 1 );
    }
    else
    {
        g_PhongProgram.Use();
        glNamedBufferData( g_ConstantBuffer, sizeof( ConstantBufferCache ), &m_cache, GL_DYNAMIC_DRAW );
        glDispatchCompute( width, height, 1 );
    }

    // NOTE: this slows things down!!!!
    glMemoryBarrier( GL_SHADER_IMAGE_ACCESS_BARRIER_BIT );

    g_FullScreenProgram.Use();
    glDrawArrays( GL_TRIANGLES, 0, 6 );

    m_cache.dirty = 0;

    RenderGui();
}

}  // namespace pt

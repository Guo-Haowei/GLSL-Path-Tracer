#include "scene_loader.h"

#include <filesystem>
#include <stdexcept>

#include "com_file.h"
#include "universal/core_assert.h"
#include "universal/print.h"

// #define DEBUG_LUA_VERBOSE IN_USE
#define DEBUG_LUA_VERBOSE NOT_IN_USE
#if USING( DEBUG_LUA_VERBOSE )
#define IF_DEBUG_LUA_VERBOSE( ... ) __VA_ARGS__
#else
#define IF_DEBUG_LUA_VERBOSE( ... ) ( (void)0 )
#endif

static inline bool streq( const char* a, const char* b )
{
    return strcmp( a, b ) == 0;
}

static Scene* g_scene;
static bool g_verbose = true;

extern "C" {
#include <lua/lauxlib.h>
#include <lua/lua.h>
#include <lua/lualib.h>

static void LuaHelper_FillVec3( lua_State* L, vec3& outVec )
{
    if ( !lua_istable( L, -1 ) )
    {
        Com_PrintError( "Expected Vector3, got %s", lua_typename( L, lua_type( L, -1 ) ) );
        return;
    }

    core_assert( lua_rawlen( L, -1 ) == 3 );

    for ( int index = 0; index < 3; ++index )
    {
        // Push our target index to the stack.
        lua_pushinteger( L, index + 1 );
        lua_gettable( L, -2 );
        outVec[index] = (float)luaL_checknumber( L, -1 );
        lua_pop( L, 1 );
    }
}

static int LuaFunc_SceneAddMaterial( lua_State* L )
{
    core_assert( g_scene );

    SceneMat material;
    material.name = luaL_checkstring( L, 1 );

    core_assert( lua_istable( L, 2 ) );

    lua_pushvalue( L, -1 );
    lua_pushnil( L );
    while ( lua_next( L, -2 ) )
    {
        lua_pushvalue( L, -2 );
        const char* field = lua_tostring( L, -1 );
        lua_pop( L, 1 );
        IF_DEBUG_LUA_VERBOSE( Com_Printf( "processing field %s of material '%s'", field, material.name.c_str() ) );
        if ( streq( field, "albedo" ) )
        {
            LuaHelper_FillVec3( L, material.albedo );
        }
        else if ( streq( field, "emissive" ) )
        {
            LuaHelper_FillVec3( L, material.emissive );
        }
        else if ( streq( field, "reflect" ) )
        {
            material.reflect = (float)lua_tonumber( L, -1 );
        }
        else if ( streq( field, "roughness" ) )
        {
            material.roughness = (float)lua_tonumber( L, -1 );
        }
        else
        {
            Com_PrintError( "[lua] unknown filed 'Material.%s'", field );
        }
        lua_pop( L, 1 );
    }
    lua_pop( L, 1 );

    if ( g_verbose )
    {
        Com_Printf( "Adding material '%s'", material.name.c_str() );
        {
            const vec3& v = material.albedo;
            Com_Printf( "\talbedo:    %.3f %.3f %.3f", v.r, v.g, v.b );
        }
        {
            const vec3& v = material.emissive;
            Com_Printf( "\temissive:  %.3f %.3f %.3f", v.r, v.g, v.b );
        }
        Com_Printf( "\treflect:   %.3f", material.reflect );
        Com_Printf( "\troughness: %.3f", material.roughness );
    }

    g_scene->materials.emplace_back( material );
    return 0;
}

static int LuaFunc_SceneAddGeometry( lua_State* L )
{
    core_assert( g_scene );

    SceneGeometry geom;
    geom.name = luaL_checkstring( L, 1 );

    core_assert( lua_istable( L, 2 ) );

    lua_pushvalue( L, -1 );
    lua_pushnil( L );
    while ( lua_next( L, -2 ) )
    {
        lua_pushvalue( L, -2 );
        const char* field = lua_tostring( L, -1 );
        lua_pop( L, 1 );
        IF_DEBUG_LUA_VERBOSE( Com_Printf( "processing field %s of geometry '%s'", field, geom.name.c_str() ) );
        if ( streq( field, "type" ) )
        {
            geom.kind = static_cast<SceneGeometry::Kind>( lua_tointeger( L, -1 ) );
        }
        else if ( streq( field, "material" ) )
        {
            const std::string material_name = lua_tostring( L, -1 );
            for ( int i = 0; i < g_scene->materials.size(); ++i )
            {
                if ( g_scene->materials.at( i ).name == material_name )
                {
                    geom.materidId = static_cast<int>( i );
                    break;
                }
            }

            if ( geom.materidId == -1 )
            {
                Com_PrintError( "material '%s' not found for geometry '%s'", material_name.c_str(), geom.name.c_str() );
            }
        }
        else if ( streq( field, "translate" ) )
        {
            LuaHelper_FillVec3( L, geom.translate );
        }
        else if ( streq( field, "euler" ) )
        {
            LuaHelper_FillVec3( L, geom.euler );
        }
        else if ( streq( field, "scale" ) )
        {
            LuaHelper_FillVec3( L, geom.scale );
        }
        else if ( streq( field, "path" ) )
        {
            geom.path = luaL_checkstring( L, -1 );
        }
        else
        {
            Com_PrintError( "[lua] unknown filed 'Geometry.%s'", field );
        }
        lua_pop( L, 1 );
    }
    lua_pop( L, 1 );

    if ( g_verbose )
    {
        Com_Printf( "Adding geometry '%s' (%s)", geom.name.c_str(), GeomKindToString( geom.kind ) );
        if ( !geom.path.empty() )
        {
            Com_Printf( "\tpath:     '%s'", geom.path.c_str() );
        }
        Com_Printf( "\tmaterial:  %d", geom.materidId );
        {
            const vec3& v = geom.translate;
            Com_Printf( "\ttranslate: %.3f %.3f %.3f", v.x, v.y, v.z );
        }
        {
            const vec3& v = geom.euler;
            Com_Printf( "\teuler:     %.3f %.3f %.3f", v.x, v.y, v.z );
        }
        {
            const vec3& v = geom.scale;
            Com_Printf( "\tscale:     %.3f %.3f %.3f", v.x, v.y, v.z );
        }
    }

    g_scene->geometries.emplace_back( geom );
    return 0;
}

static int LuaFunc_SceneAddCamera( lua_State* L )
{
    core_assert( g_scene );
    SceneCamera& camera = g_scene->camera;

    camera.name = luaL_checkstring( L, 1 );

    core_assert( lua_istable( L, 2 ) );

    lua_pushvalue( L, -1 );
    lua_pushnil( L );
    while ( lua_next( L, -2 ) )
    {
        lua_pushvalue( L, -2 );
        const char* field = lua_tostring( L, -1 );
        lua_pop( L, 1 );
        if ( streq( field, "eye" ) )
        {
            LuaHelper_FillVec3( L, camera.eye );
        }
        else if ( streq( field, "lookAt" ) )
        {
            LuaHelper_FillVec3( L, camera.lookAt );
        }
        else if ( streq( field, "up" ) )
        {
            LuaHelper_FillVec3( L, camera.up );
        }
        else if ( streq( field, "fov" ) )
        {
            camera.fov = (float)luaL_checknumber( L, -1 );
        }
        else
        {
            Com_PrintError( "[lua] unknown filed 'Camera.%s'", field );
        }
        lua_pop( L, 1 );
    }
    lua_pop( L, 1 );

    if ( g_verbose )
    {
        Com_Printf( "Adding camera '%s'", camera.name.c_str() );
        Com_Printf( "\tfov:      %.3f", camera.fov );
        {
            const vec3& v = camera.eye;
            Com_Printf( "\teye:      %.3f %.3f %.3f", v.x, v.y, v.z );
        }
        {
            const vec3& v = camera.lookAt;
            Com_Printf( "\tlookAt:   %.3f %.3f %.3f", v.x, v.y, v.z );
        }
        {
            const vec3& v = camera.up;
            Com_Printf( "\tup:       %.3f %.3f %.3f", v.x, v.y, v.z );
        }
    }
    return 0;
}

#define LUA_DVAR_LIB( func )       \
    {                              \
#func, LuaFunc_Scene##func \
    }

static const luaL_Reg s_funcs[] = {
    LUA_DVAR_LIB( AddMaterial ),
    LUA_DVAR_LIB( AddGeometry ),
    LUA_DVAR_LIB( AddCamera ),
    { nullptr, nullptr }
};

static int luaopen_MyLib( lua_State* L )
{
    luaL_newlib( L, s_funcs );
    return 1;
}
}

bool LuaLoadScene( const char* path, Scene& outScene )
{
    g_scene = &outScene;

    if ( !std::filesystem::exists( path ) )
    {
        Com_PrintFatal( "[filesystem] file '%s' does not exist", path );
        return false;
    }

    Com_PrintInfo( "[lua] executing %s", path );

    DefineList defines;
    std::string source = PreprocessFile( std::string( path ), defines );

    lua_State* L = luaL_newstate();
    core_assert( L );
    if ( L == nullptr )
    {
        return false;
    }

    luaL_openlibs( L );
    luaL_requiref( L, "Scene", luaopen_MyLib, 1 );

    int code;
    // code = luaL_loadfile( L, ROOT_FOLDER "scripts/common.lua" );
    // core_assert( code == LUA_OK );
    code = luaL_dostring( L, source.c_str() );
    switch ( code )
    {
        case LUA_OK:
            break;
        default:
            lua_error( L );
            const char* err = lua_tostring( L, -1 );
            Com_PrintError( "[lua] error %d\n%s", code, err );
            break;
    }

    lua_close( L );
    g_scene = nullptr;
    return code == LUA_OK;
}

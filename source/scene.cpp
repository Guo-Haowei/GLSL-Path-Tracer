#include "scene.h"

#include <list>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "image.h"
#include "utility/string_util.h"

#ifdef max
#undef max
#endif  // #ifdef max
#ifdef min
#undef min
#endif  // #ifdef min
#define TINYOBJLOADER_IMPLEMENTATION
#include "../third_party/tinyobjloader/tiny_obj_loader.h"

#ifndef DATA_DIR
#define DATA_DIR ""
#endif

namespace pt {

using std::list;
using std::runtime_error;
using std::string;
using std::unordered_map;
using std::vector;

SceneStats g_SceneStats;

const char* GeomKindToString( SceneGeometry::Kind kind )
{
    const char* s_map[] = {
        "invalid",
        "sphere",
        "quad",
        "cube",
        "mesh",
    };

    return s_map[static_cast<std::underlying_type_t<SceneGeometry::Kind>>( kind )];
}

SceneGeometry::Kind StringToGeomKind( const string& kind )
{
    static const unordered_map<string, SceneGeometry::Kind> s_map = {
        { "quad", SceneGeometry::Kind::Quad },
        { "cube", SceneGeometry::Kind::Cube },
        { "mesh", SceneGeometry::Kind::Mesh },
        { "sphere", SceneGeometry::Kind::Sphere },
    };

    auto it = s_map.find( kind );
    if ( it == s_map.end() )
    {
        throw runtime_error( va( "Invalid object kind '%s'", kind.c_str() ) );
    }

    return it->second;
}

//------------------------------------------------------------------------------
// GPU Scene Construction
//------------------------------------------------------------------------------

static vec3 Mat4MulVec3( const mat4& mat, const vec3& vec )
{
    vec4 vec4( vec, 1.0f );
    return vec3( mat * vec4 );
}

static mat4 CalcTransform( const SceneGeometry& geom )
{
    mat4 identity  = mat4( 1 );
    mat4 rotateX   = glm::rotate( identity, glm::radians( geom.euler.x ), vec3( 1, 0, 0 ) );
    mat4 rotateY   = glm::rotate( identity, glm::radians( geom.euler.y ), vec3( 0, 1, 0 ) );
    mat4 rotateZ   = glm::rotate( identity, glm::radians( geom.euler.z ), vec3( 0, 0, 1 ) );
    mat4 translate = glm::translate( identity, geom.translate );
    mat4 scale     = glm::scale( identity, geom.scale );
    return translate * rotateX * rotateY * rotateZ * scale;
}

static void AddQuad( const SceneGeometry& quad, GeometryList& geoms )
{
    static const vec3 s_points[] = {
        vec3( -1, 0, +1 ),
        vec3( -1, 0, -1 ),
        vec3( +1, 0, -1 ),
        vec3( +1, 0, +1 ),
    };
    const mat4 trans = CalcTransform( quad );

    geoms.push_back( Geometry(
        Mat4MulVec3( trans, s_points[0] ),
        Mat4MulVec3( trans, s_points[1] ),
        Mat4MulVec3( trans, s_points[2] ),
        quad.materidId ) );

    geoms.push_back( Geometry(
        Mat4MulVec3( trans, s_points[3] ),
        Mat4MulVec3( trans, s_points[0] ),
        Mat4MulVec3( trans, s_points[2] ),
        quad.materidId ) );
}

static void AddSphere( const SceneGeometry& sphere, GeometryList& geoms )
{
    geoms.push_back( Geometry( sphere.translate, sphere.scale.x, sphere.materidId ) );
}

/**
 *        E__________________ H
 *       /|                 /|
 *      / |                / |
 *     /  |               /  |
 *   A/___|______________/D  |
 *    |   |              |   |
 *    |   |              |   |
 *    |   |              |   |
 *    |  F|______________|___|G
 *    |  /               |  /
 *    | /                | /
 *   B|/_________________|C
 *
 */

static void AddCube( const SceneGeometry& cube, GeometryList& geoms )
{
    enum { A,
           B,
           C,
           D,
           E,
           F,
           G,
           H };

    const vec3& size    = cube.scale;
    vector<vec3> points = {
        vec3( -1, +1, +1 ),
        vec3( -1, -1, +1 ),
        vec3( +1, -1, +1 ),
        vec3( +1, +1, +1 ),
        vec3( -1, +1, -1 ),
        vec3( -1, -1, -1 ),
        vec3( +1, -1, -1 ),
        vec3( +1, +1, -1 )
    };

    const mat4 trans = CalcTransform( cube );
    for ( vec3& point : points )
    {
        point = Mat4MulVec3( trans, point );
    }

    vector<uvec3> faces = {
        { A, B, D },  // ABD
        { D, B, C },  // DBC
        { E, H, F },  // EHF
        { H, G, F },  // HGF

        { D, C, G },  // DCG
        { D, G, H },  // DGH
        { A, F, B },  // AFB
        { A, E, F },  // AEF

        { A, D, H },  // ADH
        { A, H, E },  // AHE
        { B, F, G },  // BFG
        { B, G, C },  // BGC
    };

    for ( const uvec3& face : faces )
    {
        Geometry geom( points[face.x], points[face.y], points[face.z], cube.materidId );
        geoms.push_back( geom );
    }
}

ImageArray g_AlbedoMaps;

static void AddMesh( const SceneGeometry& mesh, GeometryList& outGeoms, vector<GpuMaterial>& inoutMats )
{
    static int sCnt = 0;
    ++sCnt;
    if ( sCnt > 1 )
    {
        throw runtime_error( "At most one .obj per scene" );
    }

    string path = DATA_DIR;
    path.append( mesh.path );
    tinyobj::ObjReaderConfig reader_config;
    reader_config.triangulate = true;
    string searchPath( path.c_str(), strrchr( path.c_str(), '/' ) + 1 );
    reader_config.mtl_search_path = searchPath;  // Path to material files

    tinyobj::ObjReader reader;
    if ( !reader.ParseFromFile( path, reader_config ) )
    {
        if ( !reader.Error().empty() )
        {
            std::string err = "Failed to parse obj '" + path + "'";
            throw std::runtime_error( err );
        }
    }

    // if (!reader.Warning().empty()) {
    //     printf("Warning parsing '%s', %s\n", path.c_str(), reader.Warning().c_str());
    // }

    auto& attrib    = reader.GetAttrib();
    auto& shapes    = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    // TODO: create textures
    vector<string> albedoPaths;

    const size_t materialOffset = inoutMats.size();
    for ( const tinyobj::material_t& mat : materials )
    {
        GpuMaterial gpuMat;
        gpuMat.albedo.x = mat.diffuse[0];
        gpuMat.albedo.y = mat.diffuse[1];
        gpuMat.albedo.z = mat.diffuse[2];
        // HACK: approximate
        gpuMat.reflect = 0.01f * mat.shininess;
        // gpuMat.reflect = (glm::log2(mat.shininess) / glm::log2(256.f)) - 0.3f;
        gpuMat.reflect        = glm::clamp( gpuMat.reflect, 0.0f, 1.0f );
        gpuMat.emissive       = vec3( 0.f );
        gpuMat.roughness      = 1 - gpuMat.reflect;
        gpuMat.albedoMapLevel = 0.0f;
        gpuMat.albedo         = glm::max( gpuMat.albedo, vec3( 0.05 ) );
        // printf("%s : %f\n", mat.name.c_str(), gpuMat.reflect);

        const string& albedoMapName = mat.diffuse_texname;

        if ( !albedoMapName.empty() )
        {
            bool found = false;
            size_t idx = 0;
            for ( ; idx < albedoPaths.size(); ++idx )
            {
                const string& path = albedoPaths[idx];
                if ( path == albedoMapName )
                {
                    found = true;
                    break;
                }
            }

            if ( !found )
            {
                albedoPaths.push_back( albedoMapName );
            }

            gpuMat.albedoMapLevel = float( idx );
        }

        inoutMats.push_back( gpuMat );
    }

    for ( const string& path : albedoPaths )
    {
        Image image     = ReadImage( searchPath + path );
        image.debugName = path;
        g_AlbedoMaps.images.push_back( image );
        g_AlbedoMaps.maxWidth  = glm::max( g_AlbedoMaps.maxWidth, image.width );
        g_AlbedoMaps.maxHeight = glm::max( g_AlbedoMaps.maxHeight, image.height );
    }

    const mat4 trans = CalcTransform( mesh );

    // Loop over shapes
    for ( size_t s = 0; s < shapes.size(); s++ )
    {
        size_t index_offset = 0;
        for ( size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++ )
        {
            size_t fv = size_t( shapes[s].mesh.num_face_vertices[f] );
            assert( fv == 3 );

            vec3 points[3];
            vec3 normals[3];
            vec2 uvs[3] = { vec2( 0.0f ), vec2( 0.0f ), vec2( 0.0f ) };

            Image albedo;
            bool hasAlbedo = false;
            bool hasNormal = false;

            int matId = mesh.materidId;
            if ( materials.size() )
            {
                const auto tinyobjMatId               = shapes[s].mesh.material_ids[f];
                matId                                 = tinyobjMatId + static_cast<int>( materialOffset );
                const tinyobj::material_t& tinyobjMat = materials.at( tinyobjMatId );

                // find which image
                for ( const auto& image : g_AlbedoMaps.images )
                {
                    if ( image.debugName == tinyobjMat.diffuse_texname )
                    {
                        albedo    = image;
                        hasAlbedo = true;
                        break;
                    }
                }
            }

            for ( size_t v = 0; v < fv; v++ )
            {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx   = attrib.vertices[3 * size_t( idx.vertex_index ) + 0];
                tinyobj::real_t vy   = attrib.vertices[3 * size_t( idx.vertex_index ) + 1];
                tinyobj::real_t vz   = attrib.vertices[3 * size_t( idx.vertex_index ) + 2];

                {
                    vec4 tmp( vx, vy, vz, 1.0f );
                    points[v] = trans * tmp;
                }

                if ( idx.normal_index >= 0 )
                {
                    hasNormal          = true;
                    tinyobj::real_t nx = attrib.normals[3 * size_t( idx.normal_index ) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t( idx.normal_index ) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t( idx.normal_index ) + 2];
                    normals[v]         = mat3( trans ) * vec3( nx, ny, nz );
                    normals[v]         = glm::normalize( normals[v] );
                }

                if ( idx.texcoord_index >= 0 )
                {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t( idx.texcoord_index ) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t( idx.texcoord_index ) + 1];

                    // TODO: fix uv
                    // if (hasAlbedo) {
                    //     tx = tx - glm::floor(tx);
                    //     ty = ty - glm::floor(ty);
                    // }

                    uvs[v].x = tx;
                    uvs[v].y = ty;
                }
            }
            index_offset += fv;

            // per-face material
            Geometry geom( points[0], points[1], points[2], mesh.materidId );
            geom.uv1          = uvs[0];
            geom.uv2          = uvs[1];
            geom.uv3x         = uvs[2].x;
            geom.uv3y         = uvs[2].y;
            geom.hasAlbedoMap = hasAlbedo ? 1.0f : 0.0f;
            geom.materialId   = matId;
            if ( hasNormal )
            {
                geom.normal1 = normals[0];
                geom.normal2 = normals[1];
                geom.normal3 = normals[2];
            }
            outGeoms.push_back( geom );
        }
    }
}

void ConstructScene( const Scene& inScene, GpuScene& outScene )
{
    /// materials
    outScene.materials.clear();
    for ( const SceneMat& mat : inScene.materials )
    {
        GpuMaterial gpuMat;
        gpuMat.albedo         = mat.albedo;
        gpuMat.emissive       = mat.emissive;
        gpuMat.reflect        = mat.reflect;
        gpuMat.roughness      = mat.roughness;
        gpuMat.albedoMapLevel = 0.0f;
        outScene.materials.push_back( gpuMat );
    }

    /// objects
    outScene.geometries.clear();
    GeometryList tmpGpuObjects;
    for ( const SceneGeometry& geom : inScene.geometries )
    {
        switch ( geom.kind )
        {
            case SceneGeometry::Kind::Sphere:
                AddSphere( geom, tmpGpuObjects );
                break;
            case SceneGeometry::Kind::Quad:
                AddQuad( geom, tmpGpuObjects );
                break;
            case SceneGeometry::Kind::Cube:
                AddCube( geom, tmpGpuObjects );
                break;
            case SceneGeometry::Kind::Mesh:
                AddMesh( geom, tmpGpuObjects, outScene.materials );
                break;
            default:
                printf( "Invalid scene object type '%s'\n", GeomKindToString( geom.kind ) );
                break;
        }
    }

    /// construct bvh
    Bvh root( tmpGpuObjects );
    root.CreateGpuBvh( outScene.bvhs, outScene.geometries );

    outScene.bbox = root.GetBox();

    /// adjust bbox
    for ( GpuBvh& bvh : outScene.bvhs )
    {
        for ( int i = 0; i < 3; ++i )
        {
            if ( glm::abs( bvh.max[i] - bvh.min[i] ) < 0.01f )
            {
                bvh.min[i] -= Box3::minSpan;
                bvh.max[i] += Box3::minSpan;
            }
        }
    }
}

}  // namespace pt

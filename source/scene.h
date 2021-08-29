#pragma once
#include <string>

#include "geomath/bvh.h"
#include "geomath/geometry.h"

namespace pt {

struct SceneStats {
    int height;
    int geomCnt;
    int bboxCnt;
};

extern SceneStats g_SceneStats;

struct SceneCamera {
    static constexpr float DEFAULT_FOV = 60.0f;

    std::string name;
    float fov;
    vec3 eye;
    vec3 lookAt;
    vec3 up;

    SceneCamera()
        : fov( DEFAULT_FOV ), eye( vec4( 0, 0, 0, 1 ) ), lookAt( vec3( 0 ) ), up( vec3( 0, 1, 0 ) ) {}
};

struct SceneMat {
    std::string name;
    vec3 albedo;
    vec3 emissive;
    float reflect;
    float roughness;

    SceneMat()
        : albedo( vec3( 1 ) )
        , emissive( vec3( 0 ) )
        , reflect( 0.f )
        , roughness( 1.f ) {}
};

struct SceneGeometry {
    enum class Kind {
        Invalid,
        Sphere,
        Quad,
        Cube,
        Mesh,
    };

    SceneGeometry()
        : kind( Kind::Invalid ), materidId( -1 ), translate( vec3( 0 ) ), euler( vec3( 0 ) ), scale( vec3( 1 ) ) {}

    std::string name;
    std::string path;
    Kind kind;
    int materidId;
    vec3 translate;
    vec3 euler;
    vec3 scale;
};

const char* GeomKindToString( SceneGeometry::Kind kind );

struct Scene {
    SceneCamera camera;
    std::vector<SceneMat> materials;
    std::vector<SceneGeometry> geometries;
};

struct GpuMaterial {
    vec3 albedo;
    float reflect;
    vec3 emissive;
    float roughness;
    float albedoMapLevel;
    int padding[3];
};

struct GpuScene {
    std::vector<GpuMaterial> materials;
    std::vector<Geometry> geometries;
    std::vector<GpuBvh> bvhs;

    int height;
    Box3 bbox;
};

void ConstructScene( const Scene& inScene, GpuScene& outScene );

}  // namespace pt

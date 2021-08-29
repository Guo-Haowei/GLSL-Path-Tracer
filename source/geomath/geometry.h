#pragma once
#include <vector>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#ifdef max
#undef max
#endif  // #ifdef max
#ifdef min
#undef min
#endif  // #ifdef min

namespace pt {

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;
using glm::mat3;
using glm::mat4;
using glm::uvec2;
using glm::uvec3;
using glm::uvec4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

struct Geometry {
    enum class Kind {
        Invalid,
        Triangle,
        Sphere,
        Count
    };

    vec3 A;
    Kind kind;
    vec3 B;
    float radius;
    vec3 C;
    int materialId;
    vec2 uv1;
    vec2 uv2;
    vec3 normal1;
    float uv3x;
    vec3 normal2;
    float uv3y;
    vec3 normal3;
    float hasAlbedoMap = 0.0f;

    Geometry();
    Geometry( const vec3& A, const vec3& B, const vec3& C, int material );
    Geometry( const vec3& center, float radius, int material );
    vec3 Centroid() const;
    void CalcNormal();
};

using GeometryList = std::vector<Geometry>;
static_assert( sizeof( Geometry ) % sizeof( vec4 ) == 0 );

struct Box3 {
    static constexpr float minSpan = 0.001f;

    vec3 min;
    vec3 max;

    Box3();
    Box3( const vec3& min, const vec3& max );
    Box3( const Box3& box1, const Box3& box2 );

    vec3 Center() const;
    float SurfaceArea() const;
    bool IsValid() const;
    void Expand( const vec3& point );
    void Expand( const Box3& box );

    void MakeValid();

    static Box3 FromGeometry( const Geometry& geom );
    static Box3 FromGeometries( const GeometryList& geoms );
    static Box3 FromGeometriesCentroid( const GeometryList& geoms );
};

}  // namespace pt

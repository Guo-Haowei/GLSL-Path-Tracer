#include "geometry.h"

#include <cassert>
#include <limits>

namespace pt {

Geometry::Geometry()
    : kind( Kind::Invalid ), materialId( -1 ) {}

Geometry::Geometry( const vec3& A, const vec3& B, const vec3& C, int material )
    : kind( Kind::Triangle ), A( A ), B( B ), C( C ), materialId( material )
{
    CalcNormal();
}

Geometry::Geometry( const vec3& center, float radius, int material )
    : kind( Kind::Sphere ), A( center ), materialId( material )
{
    // TODO: refactor
    this->radius = glm::max( 0.01f, glm::abs( radius ) );
}

void Geometry::CalcNormal()
{
    using glm::cross;
    using glm::normalize;
    vec3 BA   = normalize( B - A );
    vec3 CA   = normalize( C - A );
    vec3 norm = normalize( cross( BA, CA ) );
    normal1   = norm;
    normal2   = norm;
    normal3   = norm;
}

vec3 Geometry::Centroid() const
{
    switch ( kind )
    {
        case Kind::Triangle:
            return ( A + B + C ) / 3.0f;
        case Kind::Sphere:
            return A;
        default:
            assert( 0 );
            return vec3( 0.0f );
    }
}

using GeometryList = std::vector<Geometry>;
static_assert( sizeof( Geometry ) % sizeof( vec4 ) == 0 );

Box3::Box3()
    : min( std::numeric_limits<float>::infinity() ), max( -std::numeric_limits<float>::infinity() ) {}

Box3::Box3( const vec3& min, const vec3& max )
    : min( min ), max( max ) {}

Box3::Box3( const Box3& box1, const Box3& box2 )
    : min( glm::min( box1.min, box2.min ) ), max( glm::max( box1.max, box2.max ) ) {}

vec3 Box3::Center() const
{
    return 0.5f * ( min + max );
}

void Box3::Expand( const vec3& p )
{
    min = glm::min( min, p );
    max = glm::max( max, p );
}

void Box3::Expand( const Box3& box )
{
    min = glm::min( min, box.min );
    max = glm::max( max, box.max );
}

bool Box3::IsValid() const
{
    return min.x < max.y && min.y < max.y && min.z < max.z;
}

void Box3::MakeValid()
{
    for ( int i = 0; i < 3; ++i )
    {
        if ( min[i] == max[i] )
        {
            min[i] -= Box3::minSpan;
            max[i] += Box3::minSpan;
        }
    }
}

float Box3::SurfaceArea() const
{
    if ( !IsValid() )
    {
        return 0.0f;
    }
    vec3 span    = glm::abs( max - min );
    float result = 2.0f * ( span.x * span.y +
                            span.x * span.z +
                            span.y * span.z );
    return result;
}

static Box3 Box3FromSphere( const Geometry& sphere )
{
    assert( sphere.kind == Geometry::Kind::Sphere );

    return Box3( sphere.A - vec3( sphere.radius ), sphere.A + vec3( sphere.radius ) );
}

static Box3 Box3FromTriangle( const Geometry& triangle )
{
    assert( triangle.kind == Geometry::Kind::Triangle );

    Box3 ret = Box3(
        glm::min( triangle.C, glm::min( triangle.A, triangle.B ) ),
        glm::max( triangle.C, glm::max( triangle.A, triangle.B ) ) );

    ret.MakeValid();
    return ret;
}

Box3 Box3::FromGeometry( const Geometry& geom )
{
    switch ( geom.kind )
    {
        case Geometry::Kind::Triangle:
            return Box3FromTriangle( geom );
        case Geometry::Kind::Sphere:
            return Box3FromSphere( geom );
        default:
            assert( 0 );
    }
    return Box3();
}

Box3 Box3::FromGeometries( const GeometryList& geoms )
{
    Box3 box;
    for ( const Geometry& geom : geoms )
    {
        box = Box3( box, Box3::FromGeometry( geom ) );
    }

    box.MakeValid();
    return box;
}

Box3 Box3::FromGeometriesCentroid( const GeometryList& geoms )
{
    Box3 box;
    for ( const Geometry& geom : geoms )
    {
        box.Expand( geom.Centroid() );
    }

    box.MakeValid();
    return box;
}

}  // namespace pt

#include "bvh.h"

#include <algorithm>
#include <cassert>
#include <cstdio>

namespace pt {

using std::vector;

GpuBvh::GpuBvh()
    : missIdx( -1 ), hitIdx( -1 ), leaf( 0 ), geomIdx( -1 )
{
    padding[0] = 0;
    padding[1] = 0;
}

static int genIdx()
{
    static int idx = 0;
    return idx++;
}

static int DominantAxis( const Box3& box )
{
    const vec3 span = box.max - box.min;
    int axis        = 0;
    if ( span[axis] < span.y )
    {
        axis = 1;
    }
    if ( span[axis] < span.z )
    {
        axis = 2;
    }

    return axis;
}

void Bvh::SplitByAxis( GeometryList& geoms )
{
    class Sorter {
       public:
        bool operator()( const Geometry& geom1, const Geometry& geom2 )
        {
            Box3 aabb1   = Box3::FromGeometry( geom1 );
            Box3 aabb2   = Box3::FromGeometry( geom2 );
            vec3 center1 = aabb1.Center();
            vec3 center2 = aabb2.Center();
            return center1[axis] < center2[axis];
        }

        Sorter( int axis )
            : axis( axis )
        {
            assert( axis < 3 );
        }

       private:
        const unsigned int axis;
    };

    Sorter sorter( DominantAxis( m_box ) );

    std::sort( geoms.begin(), geoms.end(), sorter );
    const size_t mid = geoms.size() / 2;
    GeometryList leftPartition( geoms.begin(), geoms.begin() + mid );
    GeometryList rightPartition( geoms.begin() + mid, geoms.end() );

    m_left  = new Bvh( leftPartition, this );
    m_right = new Bvh( rightPartition, this );
    m_leaf  = false;
}

Bvh::Bvh( GeometryList& geometries, Bvh* parent )
    : m_idx( genIdx() ), m_parent( parent )
{
    m_left    = nullptr;
    m_right   = nullptr;
    m_leaf    = false;
    m_hitIdx  = -1;
    m_missIdx = -1;

    const size_t nGeoms = geometries.size();

    assert( nGeoms );

    if ( nGeoms == 1 )
    {
        m_geom = geometries.front();
        m_leaf = true;
        m_box  = Box3::FromGeometry( m_geom );
        return;
    }

    m_box                      = Box3::FromGeometries( geometries );
    const float boxSurfaceArea = m_box.SurfaceArea();

    if ( nGeoms <= 4 || boxSurfaceArea == 0.0f )
    {
        SplitByAxis( geometries );
        return;
    }

    constexpr int nBuckets = 12;
    struct BucketInfo {
        int count = 0;
        Box3 box;
    };
    BucketInfo buckets[nBuckets];

    vector<vec3> centroids( nGeoms );

    Box3 centroidBox;
    for ( size_t i = 0; i < nGeoms; ++i )
    {
        centroids[i] = geometries.at( i ).Centroid();
        centroidBox.Expand( centroids[i] );
    }

    const int axis   = DominantAxis( centroidBox );
    const float tmin = centroidBox.min[axis];
    const float tmax = centroidBox.max[axis];

    for ( size_t i = 0; i < nGeoms; ++i )
    {
        float tmp = ( ( centroids.at( i )[axis] - tmin ) * nBuckets ) / ( tmax - tmin );
        int slot( tmp );
        slot               = glm::clamp( slot, 0, nBuckets - 1 );
        BucketInfo& bucket = buckets[slot];
        ++bucket.count;
        bucket.box.Expand( Box3::FromGeometry( geometries.at( i ) ) );
    }

    float costs[nBuckets - 1];
    for ( int i = 0; i < nBuckets - 1; ++i )
    {
        Box3 b0, b1;
        int count0 = 0, count1 = 0;
        for ( int j = 0; j <= i; ++j )
        {
            b0.Expand( buckets[j].box );
            count0 += buckets[j].count;
        }
        for ( int j = i + 1; j < nBuckets; ++j )
        {
            b1.Expand( buckets[j].box );
            count1 += buckets[j].count;
        }

        constexpr float travCost      = 0.125f;
        constexpr float intersectCost = 1.f;
        costs[i]                      = travCost + ( count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea() ) / boxSurfaceArea;
    }

    int splitIndex = 0;
    float minCost  = costs[splitIndex];
    for ( int i = 0; i < nBuckets - 1; ++i )
    {
        // printf("cost of split after bucket %d is %f\n", i, costs[i]);
        if ( costs[i] < minCost )
        {
            splitIndex = i;
            minCost    = costs[splitIndex];
        }
    }

    // printf("split index is %d\n", splitIndex);

    GeometryList leftPartition;
    GeometryList rightPartition;

    for ( const Geometry& geom : geometries )
    {
        const vec3 t = geom.Centroid();
        float tmp    = ( t[axis] - tmin ) / ( tmax - tmin );
        tmp *= nBuckets;
        int slot( tmp );
        slot = glm::clamp( slot, 0, nBuckets - 1 );
        if ( slot <= splitIndex )
        {
            leftPartition.push_back( geom );
        }
        else
        {
            rightPartition.push_back( geom );
        }
    }

    // printf("left has %llu\n", leftPartition.size());
    // printf("right has %llu\n", rightPartition.size());

    m_left  = new Bvh( leftPartition, this );
    m_right = new Bvh( rightPartition, this );

    m_leaf = false;
}

Bvh::~Bvh()
{
    // TODO: free memory
}

void Bvh::DiscoverIdx()
{
    // hit link (find right link)
    m_missIdx = -1;
    for ( const Bvh* cursor = m_parent; cursor; cursor = cursor->m_parent )
    {
        if ( cursor->m_right && cursor->m_right->m_idx > m_idx )
        {
            m_missIdx = cursor->m_right->m_idx;
            break;
        }
    }

    m_hitIdx = m_left ? m_left->m_idx : m_missIdx;
}

void Bvh::CreateGpuBvh( GpuBvhList& outBvh, GeometryList& outGeometries )
{
    DiscoverIdx();

    GpuBvh gpuBvh;
    gpuBvh.min     = m_box.min;
    gpuBvh.max     = m_box.max;
    gpuBvh.hitIdx  = m_hitIdx;
    gpuBvh.missIdx = m_missIdx;
    gpuBvh.leaf    = m_leaf;
    gpuBvh.geomIdx = -1;
    if ( m_leaf )
    {
        gpuBvh.geomIdx = static_cast<int>( outGeometries.size() );
        outGeometries.push_back( m_geom );
    }

    outBvh.push_back( gpuBvh );
    if ( m_left )
    {
        m_left->CreateGpuBvh( outBvh, outGeometries );
    }
    if ( m_right )
    {
        m_right->CreateGpuBvh( outBvh, outGeometries );
    }
}

}  // namespace pt

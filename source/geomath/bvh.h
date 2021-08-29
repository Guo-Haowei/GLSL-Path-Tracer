#pragma once
#include <memory>

#include "geometry.h"

namespace pt {

struct GpuBvh {
    vec3 min;
    int missIdx;
    vec3 max;
    int hitIdx;

    int leaf;
    int geomIdx;
    int padding[2];

    GpuBvh();
};

using GpuBvhList = std::vector<GpuBvh>;

static_assert( sizeof( GpuBvh ) % sizeof( vec4 ) == 0 );

class Bvh {
   public:
    Bvh() = delete;
    explicit Bvh( GeometryList& geoms, Bvh* parent = nullptr );
    ~Bvh();

    void CreateGpuBvh( GpuBvhList& outBvh, GeometryList& outTriangles );
    inline const Box3& GetBox() const { return m_box; }

   private:
    void SplitByAxis( GeometryList& geoms );
    void DiscoverIdx();

    Box3 m_box;
    Geometry m_geom;
    Bvh* m_left;
    Bvh* m_right;
    bool m_leaf;

    const int m_idx;
    Bvh* const m_parent;

    int m_hitIdx;
    int m_missIdx;
};

}  // namespace pt

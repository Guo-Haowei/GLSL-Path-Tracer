#pragma once
#include "geomath/geometry.h"

namespace pt {

struct ConstantBufferCache {
    vec3 camPos;
    float camFov;
    vec3 camFwd;
    int dirty;

    vec3 camRight;
    int frame;
    vec3 camUp;
    int envTexture;

    ivec2 tileOffset;
    int padding[2];

    ConstantBufferCache()
        : camPos(vec3(0, 0, 1)),
          camFwd(vec3(0)),
          camRight(1, 0, 0),
          camUp(0, 1, 0),
          frame(0),
          dirty(0),
          tileOffset(ivec2(0)),
          camFov(60.f),
          envTexture(1) {}
};

static_assert(sizeof(ConstantBufferCache) % sizeof(vec4) == 0);

}  // namespace pt

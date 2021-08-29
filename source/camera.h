#pragma once
#include "Scene.h"

namespace pt {

struct Camera {
    vec3 pos;
    vec3 fwd;
    vec3 right;
    vec3 up;
    float fov;
    float speed;

    Camera() = default;
    Camera( const SceneCamera& camera );
    void CalcSpeed( const Box3& bbox );
};

}  // namespace pt

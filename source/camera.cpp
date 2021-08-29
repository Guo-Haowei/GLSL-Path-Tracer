#include "camera.h"

namespace pt {

using glm::cross;
using glm::normalize;

Camera::Camera( const SceneCamera& cam )
{
    fov   = cam.fov;
    pos   = cam.eye;
    fwd   = normalize( cam.lookAt - pos );
    right = cross( fwd, normalize( cam.up ) );
    up    = cross( right, fwd );
}

void Camera::CalcSpeed( const Box3& bbox )
{
    constexpr float speedFactor = 0.0003f;
    constexpr float minSpeed    = 0.0f;
    vec3 span                   = bbox.max - bbox.min;
    speed                       = glm::min( span.x, span.y );
    speed                       = glm::min( speed, span.z );
    speed *= speedFactor;
    speed = glm::max( speed, minSpeed );
}

}  // namespace pt

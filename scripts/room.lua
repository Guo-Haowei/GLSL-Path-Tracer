#include "common.lua"

Scene.AddMaterial('light', {
    albedo = Vector3.Make(0.78),
    emissive = Vector3.Make(10.0)
});

Scene.AddGeometry('light', {
    type = GeometryKind.Quad,
    material = 'light',
    translate = Vector3.Make(0.0, 6.0, 0.0),
    scale = Vector3.Make(4.0)
});

Scene.AddGeometry('room', {
    type = GeometryKind.Mesh,
    path = 'models/conference/conference.obj',
    scale = Vector3.Make(0.01)
});

Scene.AddCamera('camera', {
    eye = Vector3.Make(-5.430935, 6.203391, -0.948637),
    lookAt = Vector3.Make(-4.511868, 5.809289, -0.948637)
});
#include "./common.lua"

Scene.AddMaterial('light', {
    albedo = Vector3.Make(0.78),
    emissive = Vector3.Make(17.0),
});

Scene.AddGeometry('light', {
    type = GeometryKind.Sphere,
    material = 'light',
    scale = Vector3.Make(10.0),
    translate = Vector3.Make(20.0, 30.0, 2.0)
});

Scene.AddGeometry('sponza', {
    type = GeometryKind.Mesh,
    path = 'models/sponza/sponza.obj'
});

Scene.AddCamera('camera', {
    eye = Vector3.Make(-5.0, 5.0, 0.0),
    lookAt = Vector3.Make(-4.0, 5.0, 0.0)
});

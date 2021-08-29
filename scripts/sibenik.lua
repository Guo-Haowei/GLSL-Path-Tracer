#include "./common.lua"

Scene.AddMaterial('light', {
    albedo = Vector3.Make(0.78),
    emissive = Vector3.Make(16.0),
});

Scene.AddMaterial('white', {
    albedo = Vector3.Make(0.7),
    reflect = 0.01,
    roughness = 0.99
});

-- add 3 lights
for i, xpos in ipairs({ 6.0, 0.0, -6.0}) do
    Scene.AddGeometry('light' .. tostring(i), {
        type = GeometryKind.Sphere,
        material = 'light',
        scale = Vector3.Make(1.5),
        translate = Vector3.Make(xpos, 2.5, 0.0)
    });
end

-- add model
Scene.AddGeometry('sibenik', {
    type = GeometryKind.Mesh,
    material = 'white',
    path = 'models/sibenik/sibenik.obj'
});

Scene.AddCamera('camera', {
    eye = Vector3.Make(-9.0, -6.9, 0.0),
    lookAt = Vector3.Make(-8.0, -7.0, 0.0)
});
#include "./common.lua"

Scene.AddMaterial('light', {
    albedo = Vector3.Make(1.0),
    emissive = Vector3.Make(4.0)
});

Scene.AddMaterial('yellow', {
    albedo = Vector3.Make(0.94, 0.79, 0.29),
    reflect = 0.3,
    roughness = 0.7
});

Scene.AddMaterial('blue', {
    albedo = Vector3.Make(0.2, 0.302, 0.261),
    reflect = 0.01,
    roughness = 0.99
});

-- add 2 lights
for i, xpos in ipairs({ 0.5, 1.5 }) do
    Scene.AddGeometry('light' .. tostring(i), {
        type = GeometryKind.Sphere,
        material = 'light',
        translate = Vector3.Make(xpos, 1.0, 3.0)
    });
end

Scene.AddGeometry('background', {
    type = GeometryKind.Quad,
    material = 'blue',
    euler = Vector3.Make(270.0, 0.0, 0.0),
    translate = Vector3.Make(0.0, 0.0, -2.0),
    scale = Vector3.Make(5.0)
});

Scene.AddGeometry('monkey', {
    type = GeometryKind.Mesh,
    path = 'models/monkey.obj',
    material = 'yellow',
});

Scene.AddCamera('camera', {
    eye = Vector3.Make(0.0, 0.0, 2.8),
});

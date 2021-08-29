#include "./common.lua"

Scene.AddMaterial('light', {
    albedo = Vector3.Make(0.78),
    emissive = Vector3.Make(17.0, 12.0, 4.0)
});

Scene.AddMaterial('white', {
    albedo = Vector3.Make(0.7),
    reflect = 0.01,
    roughness = 0.99
});

Scene.AddMaterial('red', {
    albedo = Vector3.Make(0.5, 0.1, 0.1),
    reflect = 0.01,
    roughness = 0.99
});

Scene.AddMaterial('green', {
    albedo = Vector3.Make(0.1, 0.5, 0.1),
    reflect = 0.01,
    roughness = 0.99
});

Scene.AddMaterial('glass', {
    albedo = Vector3.Make(0.7),
    reflect = 1.0,
    roughness = 0.0
});

Scene.AddGeometry('light', {
    type = GeometryKind.Quad,
    material = 'light',
    scale = Vector3.Make(0.25),
    translate = Vector3.Make(0.0, 0.99, 0.0)
});

local walls = {
    {
        euler = Vector3.Make(),
        translate = Vector3.Make(0.0, 1.0, 0.0)
    },
    {
        euler = Vector3.Make(180.0, 0.0, 0.0),
        translate = Vector3.Make(0.0, -1.0, 0.0)
    },
    {
        -- material = 'glass',
        euler = Vector3.Make(270.0, 0.0, 0.0),
        translate = Vector3.Make(0.0, 0.0, -1.0)
    },
    -- leftWall
    {
        material = 'red',
        euler = Vector3.Make(0.0, 0.0, 90.0),
        translate = Vector3.Make(-1.0, 0.0, 0.0)
    },
    -- rightWall
    {
        material = 'green',
        euler = Vector3.Make(0.0, 0.0, 270.0),
        translate = Vector3.Make(1.0, 0.0, 0.0)
    }
};

for i, wall in ipairs(walls) do
    Scene.AddGeometry('wall' .. tostring(i), {
        type = GeometryKind.Quad,
        material = wall.material and wall.material or 'white',
        euler = wall.euler,
        translate = wall.translate
    });
end

Scene.AddMaterial('glossy', {
    albedo = Vector3.Make(0.7),
    reflect = 0.6,
    roughness = 0.4
});

Scene.AddGeometry('cube1', {
    type = GeometryKind.Cube,
    material = 'glossy',
    scale = Vector3.Make(0.27),
    euler = Vector3.Make(0.0, -20.0, 0.0),
    translate = Vector3.Make(0.3, -0.73, 0.3)
});

Scene.AddGeometry('cube2', {
    type = GeometryKind.Cube,
    material = 'glossy',
    scale = Vector3.Make(0.3, 0.6, 0.3),
    euler = Vector3.Make(0.0, 25.0, 0.0),
    translate = Vector3.Make(-0.3, -0.4, -0.3)
});

Scene.AddCamera('camera', {
    eye = Vector3.Make(0.0, 0.0, 2.7)
});

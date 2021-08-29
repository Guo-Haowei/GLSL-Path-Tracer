local GeometryKind = {
    Invalid = 0,
    Sphere = 1,
    Quad = 2,
    Cube = 3,
    Mesh = 4,
};

local Vector3 = {};
Vector3.Make = function (x, y, z)
    x = x or 0.0;
    y = y or x;
    z = z or x;
    return { x, y, z };
end

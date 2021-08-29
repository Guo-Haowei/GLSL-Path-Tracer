//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------
#define EPSILON     1e-6
#define PI          3.14159265359
#define TWO_PI      6.28318530718
#define MAX_BOUNCE  10
#define RAY_T_MIN 1e-6
#define RAY_T_MAX 9999999.0
#define TRIANGLE_KIND 1
#define SPHERE_KIND 2

// #extension GL_EXT_texture_array : enable

layout (local_size_x = 1, local_size_y = 1) in;
layout (rgba32f, binding = 0) uniform image2D outImage;
uniform sampler2D envTexture;
// uniform sampler2D albedoTexture;
uniform sampler2DArray albedoTexture;

struct Ray {
    vec3 origin;
    float t;
    vec3 direction;
    int materialId;
    vec3 hitNormal;
    vec2 hitUv;
    float hasAlbedoMap;
};

struct Geometry {
    vec3 A;
    int kind;
    vec3 B;
    float radius;
    vec3 C;
    int materialId;
    vec2 uv1;
    vec2 uv2;
    vec3 normal1;
    float uv3x;
    vec3 normal2;
    float uv3y;
    vec3 normal3;
    float hasAlbedoMap;
};

struct Bvh {
    vec3 min;
    int missIdx;
    vec3 max;
    int hitIdx;

    int leaf;
    int geomIdx;
    int _padding0;
    int _padding1;
};

struct Material {
    vec3 albedo;
    float reflectChance;
    vec3 emissive;
    float roughness;
    float albedoMapLevel;
    int _padding0;
    int _padding1;
    int _padding2;
};

layout (std140, binding = 0) uniform Constant
{
    vec3 camPos;
    float camFov;

    vec3 camFwd;
    int dirty;

    vec3 camRight;
    int frame;

    vec3 camUp;
    int _padding0;

    ivec2 tileOffset;

    int _padding1;
    int _padding2;
};

layout (std140, binding = 1) buffer Geoms
{
    Geometry g_geoms[GEOM_COUNT];
};

layout (std140, binding = 2) buffer Bvhs
{
    Bvh g_bvhs[BVH_COUNT];
};

layout (std140, binding = 3) buffer Materials
{
    Material g_materials[MATERIAL_COUNT];
};

//------------------------------------------------------------------------------
// Random function
//------------------------------------------------------------------------------
// https://blog.demofox.org/2020/05/25/casual-shadertoy-path-tracing-1-basic-camera-diffuse-emissive/
uint WangHash(inout uint seed) {
    seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
    seed *= uint(9);
    seed = seed ^ (seed >> 4);
    seed *= uint(0x27d4eb2d);
    seed = seed ^ (seed >> 15);
    return seed;
}

// random number between 0 and 1
float Random(inout uint state)
{
    return float(WangHash(state)) / 4294967296.0;
}

// random unit vector
vec3 RandomUnitVector(inout uint state)
{
    float z = Random(state) * 2.0 - 1.0;
    float a = Random(state) * TWO_PI;
    float r = sqrt(1.0 - z * z);
    float x = r * cos(a);
    float y = r * sin(a);
    return vec3(x, y, z);
}

//------------------------------------------------------------------------------
// Common Ray Trace Functions
//------------------------------------------------------------------------------
// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection
bool HitTriangle(inout Ray ray, in Geometry triangle) {
    // P = A + u(B - A) + v(C - A) => O - A = -tD + u(B - A) + v(C - A)
    // -tD + uAB + vAC = AO
    vec3 AB = triangle.B - triangle.A;
    vec3 AC = triangle.C - triangle.A;

    vec3 P = cross(ray.direction, AC);
    float det = dot(AB, P);

    if (det < EPSILON)
        return false;

    float invDet = 1.0 / det;
    vec3 AO = ray.origin - triangle.A;

    vec3 Q = cross(AO, AB);
    float u = dot(AO, P) * invDet;
    float v = dot(ray.direction, Q) * invDet;

    if (u < 0.0 || v < 0.0 || u + v > 1.0)
        return false;

    float t = dot(AC, Q) * invDet;
    if (t >= ray.t || t < EPSILON)
        return false;

    ray.t = t;
    ray.materialId = triangle.materialId;
    ray.hasAlbedoMap = triangle.hasAlbedoMap;
    vec3 norm = triangle.normal1 + u * (triangle.normal2 - triangle.normal1) + v * (triangle.normal3 - triangle.normal1);
    ray.hitNormal = norm;
    vec2 uv3 = vec2(triangle.uv3x, triangle.uv3y);
    ray.hitUv = triangle.uv1 + u * (triangle.uv2 - triangle.uv1) + v * (uv3 - triangle.uv1);

    return true;
}

bool HitSphere(inout Ray ray, in Geometry sphere) {
    vec3 oc = ray.origin - sphere.A;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = half_b * half_b - a * c;

    float t = -half_b - sqrt(discriminant) / a;
    if (discriminant < EPSILON || t >= ray.t || t < EPSILON)
        return false;

    ray.t = t;
    vec3 p = ray.origin + t * ray.direction;
    ray.hitNormal = normalize(p - sphere.A);
    ray.materialId = sphere.materialId;

    return true;
}

// https://medium.com/@bromanz/another-view-on-the-classic-ray-aabb-intersection-algorithm-for-bvh-traversal-41125138b525
bool HitBvh(in Ray ray, in Bvh bvh) {
    vec3 invD = vec3(1.) / (ray.direction);
    vec3 t0s = (bvh.min - ray.origin) * invD;
    vec3 t1s = (bvh.max - ray.origin) * invD;

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger  = max(t0s, t1s);

    float tmin = max(RAY_T_MIN, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    float tmax = min(RAY_T_MAX, min(tbigger.x, min(tbigger.y, tbigger.z)));

    return (tmin < tmax) && (ray.t > tmin);
}

bool HitScene(inout Ray ray) {
    bool anyHit = false;

    int bvhIdx = 0;
    while (bvhIdx != -1) {
        Bvh bvh = g_bvhs[bvhIdx];
        if (HitBvh(ray, bvh)) {
            if (bvh.geomIdx != -1) {
                Geometry geom = g_geoms[bvh.geomIdx];
                if (geom.kind == TRIANGLE_KIND) {
                    if (HitTriangle(ray, geom)) {
                        anyHit = true;
                    }
                } else if (geom.kind == SPHERE_KIND) {
                    if (HitSphere(ray, geom)) {
                        anyHit = true;
                    }
                }
            }
            bvhIdx = bvh.hitIdx;
        } else {
            bvhIdx = bvh.missIdx;
        }
    }

    return anyHit;
}

vec2 SampleSphericalMap(in vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

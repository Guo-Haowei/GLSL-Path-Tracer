#pragma once

namespace pt::gl {

extern const unsigned char *g_Vender;
extern const unsigned char *g_Renderer;
extern const unsigned char *g_GLSLVersion;

struct ComputeGroup {
    int x, y, z;
};

extern ComputeGroup g_ComputeGroup;
extern int g_Width;
extern int g_Height;

}  // namespace pt::gl

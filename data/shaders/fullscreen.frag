layout (location = 0) out vec4 out_color;
layout (location = 0) in vec2 pass_uv;

uniform sampler2D outTexture;
uniform sampler2D envTexture;

#include "color.glsl"

#define EXPOSURE 0.5

void main() {
    vec4 color4 = texture(outTexture, pass_uv);
    vec3 color = color4.rgb / color4.a;
    color *= EXPOSURE;
    color = ACESFilm(color);
    color = LinearToSRGB(color);
    out_color = vec4(color, 1.0);
}



#ifndef MAIN_SHADERS_H
#define MAIN_SHADERS_H

const char* kScreenVS = R"(#version 140

uniform vec2 uScreenSize;
uniform mat2x3 uTransform;
uniform float uScaleFactor;

in vec2 vPosition;
in vec2 vTexcoord;

smooth out vec2 fTexcoord;

void main()
{
    vec4 fpos;

    fpos.xy = vec3(vPosition, 1.0) * uTransform * uScaleFactor;

    fpos.xy = ((fpos.xy * 2.0) / (uScreenSize * uScaleFactor)) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;

    gl_Position = fpos;
    fTexcoord = vTexcoord;
}
)";

const char* kScreenFS = R"(#version 140

uniform sampler2D ScreenTex;

smooth in vec2 fTexcoord;

out vec4 oColor;

void main()
{
    vec4 pixel = texture(ScreenTex, fTexcoord);

    oColor = vec4(pixel.bgr, 1.0);
}
)";

#endif // MAIN_SHADERS_H

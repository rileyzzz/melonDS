

#ifndef OSD_SHADERS_H
#define OSD_SHADERS_H

const char* kScreenVS_OSD = R"(#version 300 es

uniform mediump vec2 uScreenSize;

uniform ivec2 uOSDPos;
uniform ivec2 uOSDSize;
uniform mediump float uScaleFactor;

in mediump vec2 vPosition;

smooth out mediump vec2 fTexcoord;

void main()
{
    mediump vec4 fpos;

    mediump vec2 osdpos = (vPosition * vec2(uOSDSize * uScaleFactor));
    fTexcoord = osdpos;
    osdpos += uOSDPos;

    fpos.xy = ((osdpos * 2.0) / uScreenSize * uScaleFactor) - 1.0;
    fpos.y *= -1;
    fpos.z = 0.0;
    fpos.w = 1.0;

    gl_Position = fpos;
}
)";

const char* kScreenFS_OSD = R"(#version 300 es

uniform sampler2D OSDTex;

smooth in mediump vec2 fTexcoord;

out mediump vec4 oColor;

void main()
{
    mediump vec4 pixel = texelFetch(OSDTex, ivec2(fTexcoord), 0);
    oColor = pixel.bgra;
}
)";

#endif // OSD_SHADERS_H

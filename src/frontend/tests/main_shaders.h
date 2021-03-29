

#ifndef MAIN_SHADERS_H
#define MAIN_SHADERS_H

//const char* kScreenVS = R"(#version 140
//uniform vec2 uScreenSize;
//uniform mat2x3 uTransform;
//uniform float uScaleFactor;
//in vec2 vPosition;
//in vec2 vTexcoord;
//smooth out vec2 fTexcoord;
//void main()
//{
//    vec4 fpos;
//    fpos.xy = vec3(vPosition, 1.0) * uTransform * uScaleFactor;
//    fpos.xy = ((fpos.xy * 2.0) / (uScreenSize * uScaleFactor)) - 1.0;
//    fpos.y *= -1;
//    fpos.z = 0.0;
//    fpos.w = 1.0;
//    gl_Position = fpos;
//    fTexcoord = vTexcoord;
//}
//)";
//
//const char* kScreenFS = R"(#version 140
//uniform sampler2D ScreenTex;
//smooth in vec2 fTexcoord;
//out vec4 oColor;
//void main()
//{
//    vec4 pixel = texture(ScreenTex, fTexcoord);
//    oColor = vec4(pixel.bgr, 1.0);
//}
//)";

const char* kScreenVS = R"(#version 300 es

uniform mediump vec2 uScreenSize;
uniform mat2x3 uTransform;
uniform mediump float uScaleFactor;

in mediump vec2 vPosition;
in mediump vec2 vTexcoord;

smooth out mediump vec2 fTexcoord;

void main()
{
    mediump vec4 fpos;

    fpos.xy = vec3(vPosition, 1.0) * uTransform * uScaleFactor;

    fpos.xy = ((fpos.xy * 2.0) / (uScreenSize * uScaleFactor)) - 1.0;
    fpos.y *= -1.0;
    fpos.z = 0.0;
    fpos.w = 1.0;

    gl_Position = fpos;
    fTexcoord = vTexcoord;
}
)";

const char* kScreenFS = R"(#version 300 es

uniform mediump sampler2D ScreenTex;

smooth in mediump vec2 fTexcoord;

out mediump vec4 oColor;

void main()
{
    mediump vec4 pixel = texture(ScreenTex, fTexcoord);

    oColor = vec4(pixel.bgr, 1.0);
    //vec4 oColor = vec4(1.0, 0.0, 0.0, 1.0);
    //oColor = vec4(fTexcoord.x, fTexcoord.y, 0.0, 1.0);

    //gl_FragColor = oColor;
}
)";

#endif // MAIN_SHADERS_H

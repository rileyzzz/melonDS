/*
    Copyright 2016-2021 Arisotura

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#ifndef OPENGLSUPPORT_H
#define OPENGLSUPPORT_H

#include <stdio.h>
#include <string.h>

#define GLEW_STATIC
#include <GL/glew.h>
#ifndef Q_OBJECT

#include <GL/gl.h>
#include <GL/glu.h>
#endif

//#include <epoxy/gl.h>
#ifdef __EMSCRIPTEN__

#define GL_GLEXT_PROTOTYPES
#include "SDL_opengl_glext.h"

#include <SDL2/SDL.h>
#else
#include <SDL.h>

#endif
// #define _EM_GLES3_

// #include <GLES3/gl32.h>
// #define GL_GLEXT_PROTOTYPES 1
// #include <GLES3/gl2ext.h>

// #define glBindFragDataLocation(a, b, c) glBindFragDataLocationEXT(a, b, c)
// #define glDepthRange(a, b) glDepthRangef(a, b)
// #define glClearDepth(a) glClearDepthf(a)
// #define glMapBuffer(a, b) glMapBufferRange(a, b)
// #define glDrawBuffer(a) const GLenum buffers[]{ a }; glDrawBuffers(1, buffers )

// #define GL_UNSIGNED_SHORT_1_5_5_5_REV GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
// #define GL_BGRA GL_BGRA_EXT


#ifdef __EMSCRIPTEN__
#include <GL/Regal.h>
#include <emscripten.h>

#define glFramebufferTexture(target, att, tex, level) glFramebufferTexture2D(target, att, GL_TEXTURE_2D, tex, level)
//#define glColorMaski(i, r, g, b, a)
#define glBindFragDataLocation(a, b, c)
#define glDrawBuffer(a) const GLenum buffers[]{ a }; glDrawBuffers(1, buffers )

#define glColorMaski(i, r, g, b, a) glColorMask(r, g, b, a)
//#define glMapBuffer(a, b) glMapBufferRange(a, b)
//#define glMapBuffer(target, access) glMapBufferRange(target, 0, 0, access)

#else

#endif

//#define glMapBuffer(a, b) glMapBufferRange(a, b)

//GLES defines
// #define glDepthRange(a, b) glDepthRangef(a, b)
// #define glClearDepth(a) glClearDepthf(a)
// #define glMapBuffer(a, b) glMapBufferOES(a, b)


// #define glBindFragDataLocation(a, b, c) glBindFragDataLocationEXT(a, b, c)

// #define GL_UNSIGNED_SHORT_1_5_5_5_REV GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT
// #define GL_BGRA GL_BGRA_EXT

#include "Platform.h"


namespace OpenGL
{

bool BuildShaderProgram(const char* vs, const char* fs, GLuint* ids, const char* name);
bool LinkShaderProgram(GLuint* ids);
void DeleteShaderProgram(GLuint* ids);
void UseShaderProgram(GLuint* ids);

}

#endif // OPENGLSUPPORT_H

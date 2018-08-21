/*
 * Copyright (C) 2017, 2018
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <cstdlib>
#include <cstring>

#include <QOpenGLContext>

#include "gl.hpp"

namespace CamSim {

QOpenGLFunctions_4_5_Core* getGlFunctionsFromCurrentContext(const char* callingFunction)
{
    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        qCritical("No OpenGL context is current in the following function:");
        qCritical("%s", callingFunction);
        std::exit(1);
    }
    QOpenGLFunctions_4_5_Core* funcs = ctx->versionFunctions<QOpenGLFunctions_4_5_Core>();
    if (!funcs) {
        qCritical("Cannot obtain required OpenGL context version in the following function:");
        qCritical("%s", callingFunction);
        std::exit(1);
    }
    return funcs;
}

void glCheck(const char* callingFunction, const char* file, int line)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    GLenum err = gl->glGetError();
    if (err != GL_NO_ERROR) {
        qCritical("%s:%d: OpenGL error 0x%04X in the following function:", file, line, err);
        qCritical("%s", callingFunction);
        std::exit(1);
    }
}

size_t glTypeSize(GLenum type)
{
    size_t s = 0;
    switch (type) {
    case GL_UNSIGNED_BYTE:
    case GL_BYTE:
        s = 1;
        break;
    case GL_UNSIGNED_SHORT:
    case GL_SHORT:
        s = 2;
        break;
    case GL_UNSIGNED_INT:
    case GL_INT:
    case GL_FLOAT:
        s = 4;
        break;
    }
    Q_ASSERT(s != 0);
    return s;
}

size_t glFormatSize(GLenum format)
{
    size_t s = 0;
    switch (format) {
    case GL_RED:
    case GL_RED_INTEGER:
        s = 1;
        break;
    case GL_RG:
    case GL_RG_INTEGER:
        s = 2;
        break;
    case GL_RGB:
    case GL_BGR:
    case GL_RGB_INTEGER:
    case GL_BGR_INTEGER:
        s = 3;
        break;
    case GL_RGBA:
    case GL_BGRA:
    case GL_RGBA_INTEGER:
    case GL_BGRA_INTEGER:
        s = 4;
        break;
    }
    Q_ASSERT(s != 0);
    return s;
}

void glUploadTex(unsigned int pbo, unsigned int tex,
        int width, int height, int internalFormat, int format, int type,
        const void* data, size_t size)
{
    Q_ASSERT(width * height * glFormatSize(format) * glTypeSize(type) == size);
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    gl->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
    gl->glBufferData(GL_PIXEL_UNPACK_BUFFER, size, nullptr, GL_STREAM_DRAW);
    void* ptr = gl->glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, size,
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
    Q_ASSERT(ptr);
    std::memcpy(ptr, data, size);
    gl->glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    gl->glBindTexture(GL_TEXTURE_2D, tex);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0,
            format, type, nullptr);
    gl->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

}

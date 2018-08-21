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

#include <cstring>

#include <QByteArray>

#include "gl.hpp"

#include "texdata.hpp"

namespace CamSim {

TexData::TexData() :
    _w(0),
    _h(0),
    _type(0),
    _channels(0)
{
}

TexData::TexData(int w, int h, int channels, int type, const QByteArray& packedData, const QList<QString>& names) :
    _w(w), _h(h), _type(type), _channels(channels), _packedData(packedData)
{
    for (int i = 0; i < std::min(channels, names.size()); i++)
        _names[i] = names[i];
}

TexData::TexData(unsigned int tex, int cubeSide, int arrayLayer, int retrievalFormat, const QList<QString>& names, unsigned int pbo) :
    TexData()
{
    setTexture(tex, cubeSide, arrayLayer, retrievalFormat, names, pbo);
}

size_t TexData::typeSize() const
{
    return glTypeSize(_type);
}

void TexData::setTexture(unsigned int tex, int cubeSide, int arrayLayer, int retrievalFormat, const QList<QString>& names, unsigned int pbo)
{
    if (tex == 0)
        return;

    Q_ASSERT(retrievalFormat == GL_R8
            || retrievalFormat == GL_RG8
            || retrievalFormat == GL_RGB8
            || retrievalFormat == GL_RGBA8
            || retrievalFormat == GL_R32F
            || retrievalFormat == GL_RG32F
            || retrievalFormat == GL_RGB32F
            || retrievalFormat == GL_RGBA32F
            || retrievalFormat == GL_R32UI
            || retrievalFormat == GL_RG32UI
            || retrievalFormat == GL_RGB32UI
            || retrievalFormat == GL_RGBA32UI);

    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);

    GLint texW, texH;
    gl->glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_WIDTH, &texW);
    gl->glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_HEIGHT, &texH);
    ASSERT_GLCHECK();

    int texType = 0;
    int texChannels = 0;
    switch (retrievalFormat) {
    case GL_R8:
        texType = GL_UNSIGNED_BYTE;
        texChannels = 1;
        break;
    case GL_RG8:
        texType = GL_UNSIGNED_BYTE;
        texChannels = 2;
        break;
    case GL_RGB8:
        texType = GL_UNSIGNED_BYTE;
        texChannels = 3;
        break;
    case GL_RGBA8:
        texType = GL_UNSIGNED_BYTE;
        texChannels = 4;
        break;
    case GL_R32F:
        texType = GL_FLOAT;
        texChannels = 1;
        break;
    case GL_RG32F:
        texType = GL_FLOAT;
        texChannels = 2;
        break;
    case GL_RGB32F:
        texType = GL_FLOAT;
        texChannels = 3;
        break;
    case GL_RGBA32F:
        texType = GL_FLOAT;
        texChannels = 4;
        break;
    case GL_R32UI:
        texType = GL_UNSIGNED_INT;
        texChannels = 1;
        break;
    case GL_RG32UI:
        texType = GL_UNSIGNED_INT;
        texChannels = 2;
        break;
    case GL_RGB32UI:
        texType = GL_UNSIGNED_INT;
        texChannels = 3;
        break;
    case GL_RGBA32UI:
        texType = GL_UNSIGNED_INT;
        texChannels = 4;
        break;
    }

    _w = texW;
    _h = texH;
    _type = texType;
    _channels = texChannels;
    for (int i = 0; i < std::min(_channels, names.size()); i++)
        _names[i] = names[i];

    _packedData.resize(packedDataSize());
    GLenum format;
    if (_type == GL_UNSIGNED_INT) {
        format = (_channels == 4 ? GL_RGBA_INTEGER : _channels == 3 ? GL_RGB_INTEGER : _channels == 2 ? GL_RG_INTEGER : GL_RED_INTEGER);
    } else {
        format = (_channels == 4 ? GL_RGBA : _channels == 3 ? GL_RGB : _channels == 2 ? GL_RG : GL_RED);
    }
    if (_channels == 1 && _type == GL_FLOAT) {
        GLint internalFormat;
        gl->glGetTextureLevelParameteriv(tex, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
        if (internalFormat == GL_DEPTH_COMPONENT
                || internalFormat == GL_DEPTH_COMPONENT16
                || internalFormat == GL_DEPTH_COMPONENT24
                || internalFormat == GL_DEPTH_COMPONENT32F) {
            format = GL_DEPTH_COMPONENT;
        }
    }
    gl->glPixelStorei(GL_PACK_ALIGNMENT, (packedLineSize() % 4 == 0 ? 4 : packedLineSize() % 2 == 0 ? 2 : 1));
    int zOffset = 0;
    if (cubeSide >= 0) {
        if (arrayLayer >= 0)
            zOffset = 6 * arrayLayer + cubeSide;
        else
            zOffset = cubeSide;
    } else if (arrayLayer >= 0) {
        zOffset = arrayLayer;
    }
    if (pbo != 0) {
        gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        gl->glBufferData(GL_PIXEL_PACK_BUFFER, packedDataSize(), nullptr, GL_STREAM_READ);
        gl->glGetTextureSubImage(tex, 0, 0, 0, zOffset, _w, _h, 1, format, _type, packedDataSize(), nullptr);
        const void* ptr = gl->glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, packedDataSize(), GL_MAP_READ_BIT);
        Q_ASSERT(ptr);
        std::memcpy(static_cast<void*>(_packedData.data()), ptr, packedDataSize());
        gl->glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        gl->glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    } else {
        gl->glGetTextureSubImage(tex, 0, 0, 0, zOffset, _w, _h, 1, format, _type, packedDataSize(),
                static_cast<void*>(_packedData.data()));
    }
    ASSERT_GLCHECK();

    // reverse y direction
    QByteArray packedLine;
    packedLine.resize(packedLineSize());
    for (int y = 0; y < texH / 2; y++) {
        int yy = texH - 1 - y;
        std::memcpy(
                static_cast<void*>(packedLine.data()),
                static_cast<const void*>(_packedData.constData() + y * packedLineSize()),
                packedLineSize());
        std::memcpy(
                static_cast<void*>(_packedData.data() + y * packedLineSize()),
                static_cast<const void*>(_packedData.constData() + yy * packedLineSize()),
                packedLineSize());
        std::memcpy(
                static_cast<void*>(_packedData.data() + yy * packedLineSize()),
                static_cast<const void*>(packedLine.constData()),
                packedLineSize());
    }
}

QByteArray TexData::planarDataArray(int channel) const
{
    Q_ASSERT(channel >= 0 && channel <= channels());
    QByteArray planarData;
    planarData.resize(planarDataSize());
    for (int y = 0; y < height(); y++) {
        for (int x = 0; x < width(); x++) {
            size_t dstIndex = y * planarLineSize() + x * planarElementSize();
            std::memcpy(static_cast<void*>(planarData.data() + dstIndex),
                    element(x, y, channel), typeSize());
        }
    }
    return planarData;
}

QByteArray TexData::transposedPlanarDataArray(int channel) const
{
    Q_ASSERT(channel >= 0 && channel <= channels());
    QByteArray transposedPlanarData;
    transposedPlanarData.resize(planarDataSize());
    for (int y = 0; y < height(); y++) {
        for (int x = 0; x < width(); x++) {
            size_t dstIndex = x * planarColumnSize() + y * planarElementSize();
            std::memcpy(static_cast<void*>(transposedPlanarData.data() + dstIndex),
                    element(x, y, channel), typeSize());
        }
    }
    return transposedPlanarData;
}

}

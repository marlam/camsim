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

#ifndef CAMSIM_TEXTUREDATA_HPP
#define CAMSIM_TEXTUREDATA_HPP

#include <QByteArray>

namespace CamSim {

/**
 * \brief Provides convenient access to data stored in a texture
 */
class TexData
{
private:
    int _w, _h;
    int _type;
    int _channels;
    QString _names[4];
    QByteArray _packedData;

public:
    /*! \brief Constructor */
    TexData();

    /*! \brief Constructor for data given in main memory instead of texture memory. */
    TexData(int w, int h, int channels, int type, const QByteArray& packedData, const QList<QString>& names = QList<QString>());

    /*! \brief Constructor for texture \a tex. Its data is retrieved immediately so that
     * the OpenGL texture handle can be reused or deleted. See \a setTexture() for details. */
    TexData(unsigned int tex, int cubeSide, int arrayLayer, int retrievalFormat, const QList<QString>& names = QList<QString>(), unsigned int pbo = 0);

    /*! \brief Set the texture \a tex to use. Its data is retrieved immediately so that
     * the OpenGL texture handle can be reused or deleted.
     *
     * If \a cubeSide is greater than or equal to zero, then \a tex is assumed to be
     * a cube map and the specified cube side is used. Otherwise (\a cubeSide is less
     * than zero), \a tex is assumed to be a standard 2D texture.
     *
     * If \a arrayLayer is greater than or equal to zero, then \a tex is assumed to be
     * an array texture and the specified layer is used. Otherwise (\a arrayLayer is less
     * than zero), \a tex is assumed to be a standard 2D texture.
     *
     * Retrieval of data is done in the format suggested by \a retrievalFormat,
     * which can be one of GL_R8, GL_RG8 GL_RGB8, GL_RGBA8, GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F,
     * GL_R32UI, GL_RG32UI, GL_RGB32UI, GL_RGBA32UI.
     *
     * Optionally each channel in the texture can be assigned a name. This will be used
     * by the \a Exporter class, depending on the file format. For example, for Matlab
     * *.mat files, the name will be used as the corresponding variable name.
     *
     * Optionally a pixel buffer object (\a pbo) is used to transfer the data,
     * for potentially better performance. */
    void setTexture(unsigned int tex, int cubeSide, int arrayLayer, int retrievalFormat, const QList<QString>& names = QList<QString>(), unsigned int pbo = 0);

    /*! \brief Returns the texture width */
    int width() const
    {
        return _w;
    }

    /*! \brief Returns the texture height */
    int height() const
    {
        return _h;
    }

    /*! \brief Returns the number of channels in the texture */
    int channels() const
    {
        return _channels;
    }

    /*! \brief Returns the name of the given channel (may be empty) */
    const QString& channelName(int channel) const
    {
        Q_ASSERT(channel >= 0 && channel < 4);
        return _names[channel];
    }

    /*! \brief Returns the data type (depends on the retrieval format with which this object was created).
     * Currently GL_UNSIGNED_BYTE, GL_UNSIGNED_INT, or GL_FLOAT. */
    int type() const
    {
        return _type;
    }

    /*! \brief Returns the size of the data type */
    size_t typeSize() const;

    /*! \brief Returns the size of one packed texture element (consisting of one or more components,
     * depending on the channels (see \a packedData) */
    size_t packedElementSize() const
    {
        return typeSize() * channels();
    }

    /*! \brief Returns the size of one line of packed data (see \a packedData) */
    size_t packedLineSize() const
    {
        return packedElementSize() * width();
    }

    /*! \brief Returns the size of the packed data (see \a packedData) */
    size_t packedDataSize() const
    {
        return packedLineSize() * height();
    }

    /*! \brief Return a pointer to the packed data.
     *
     * Packed means that for every texture element all channels are stored together
     * (in planar format, they are separated).
     */
    const void* packedData() const
    {
        return _packedData.constData();
    }

    /*! \brief Return a pointer to the data element with coordinates \a x and \a y
     * and the channel number \a c. */
    const void* element(int x, int y, int c = 0) const
    {
        Q_ASSERT(x >= 0 && x < _w);
        Q_ASSERT(y >= 0 && y < _h);
        Q_ASSERT(c >= 0 && c < _channels);
        size_t offset = y * packedLineSize() + x * packedElementSize() + c * typeSize();
        return static_cast<const void*>(static_cast<const unsigned char*>(packedData()) + offset);
    }

    /*! \brief Returns the size of one planar texture element (consisting of only one components).
     * This is the same as \a typeSize(). See \a planarData.*/
    size_t planarElementSize() const
    {
        return typeSize();
    }

    /*! \brief Returns the size of one line of planar data (see \a planarData) */
    size_t planarLineSize() const
    {
        return planarElementSize() * width();
    }

    /*! \brief Returns the size of one column of planar data (see \a planarData) */
    size_t planarColumnSize() const
    {
        return planarElementSize() * height();
    }

    /*! \brief Returns the size of the planar data (see \a planarData) */
    size_t planarDataSize() const
    {
        return planarLineSize() * height();
    }

    /*! \brief Return the planar data for the given \a channel as a byte array.
     *
     * Planar means that the data contains only the given channel.
     */
    QByteArray planarDataArray(int channel) const;

    /*! \brief Return the transposed planar data for the given \a channel as a byte array.
     *
     * Planar means that the data contains only the given channel.
     */
    QByteArray transposedPlanarDataArray(int channel) const;
};

}

#endif

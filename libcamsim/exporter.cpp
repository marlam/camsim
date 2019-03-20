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

#include <QFile>
#include <QTextStream>
#include <QImage>
#include <QFuture>
#include <QtConcurrent>

#ifdef HAVE_GTA
# include <gta/gta.hpp>
#endif
#ifdef HAVE_MATIO
# include <matio.h>
#endif
#ifdef HAVE_HDF5
# include <H5Cpp.h>
#endif

#include "gl.hpp"
#include "exporter.hpp"


namespace CamSim {

Exporter::Exporter()
{
}

bool Exporter::isFileFormatSupported(FileFormat format)
{
    switch (format) {
    case FileFormat_RAW:
        return true;
        break;
    case FileFormat_CSV:
        return true;
        break;
    case FileFormat_PNM:
        return true;
        break;
    case FileFormat_PNG:
        return true;
        break;
    case FileFormat_PFS:
        return true;
        break;
    case FileFormat_GTA:
#ifdef HAVE_GTA
        return true;
#else
        return false;
#endif
        break;
    case FileFormat_MAT:
#ifdef HAVE_MATIO
        return true;
#else
        return false;
#endif
        break;
    case FileFormat_HDF5:
#ifdef HAVE_HDF5
        return true;
#else
        return false;
#endif
        break;
    case FileFormat_Auto:
        // cannot happen
        break;
    }
    return false;
}

bool Exporter::isFileFormatCompatible(FileFormat format, const TexData& data, const QList<int>& channels)
{
    int channelCount = (channels.size() == 0 ? data.channels() : channels.size());
    switch (format) {
    case FileFormat_RAW:
        return true;
        break;
    case FileFormat_CSV:
        return true;
        break;
    case FileFormat_PNM:
        return (channelCount == 1 || channelCount == 3) && data.type() == GL_UNSIGNED_BYTE;
        break;
    case FileFormat_PNG:
        return (channelCount == 1 || channelCount == 3) && data.type() == GL_UNSIGNED_BYTE;
        break;
    case FileFormat_PFS:
        return true;
        break;
    case FileFormat_GTA:
        return true;
        break;
    case FileFormat_MAT:
        return true;
        break;
    case FileFormat_HDF5:
        return true;
        break;
    case FileFormat_Auto:
        // cannot happen
        break;
    }
    return false;
}

FileFormat Exporter::fileFormatFromName(const QString& fileName)
{
    if (fileName.endsWith(".raw"))
        return FileFormat_RAW;
    else if (fileName.endsWith(".csv"))
        return FileFormat_CSV;
    else if (fileName.endsWith(".pgm") || fileName.endsWith(".ppm"))
        return FileFormat_PNM;
    else if (fileName.endsWith(".png"))
        return FileFormat_PNG;
    else if (fileName.endsWith(".pfs"))
        return FileFormat_PFS;
    else if (fileName.endsWith(".gta"))
        return FileFormat_GTA;
    else if (fileName.endsWith(".mat"))
        return FileFormat_MAT;
    else if (fileName.endsWith(".h5"))
        return FileFormat_HDF5;
    else
        return FileFormat_Auto;
}

bool Exporter::checkExportRequest(Exporter* asyncExporter,
        const QString& fileName, FileFormat format,
        const QList<TexData>& dataList, const QList<QList<int>>& channelsList,
        int compressionLevel,
        FileFormat& cleanedFormat,
        QList<QList<int>>& cleanedChannelsList,
        int& cleanedCompressionLevel)
{
    if (dataList.size() == 0) {
        qCritical("%s: no data to export", qPrintable(fileName));
        return false;
    }
    cleanedFormat = format;
    if (format == FileFormat_Auto) {
        cleanedFormat = fileFormatFromName(fileName);
    }
    if (cleanedFormat == FileFormat_Auto) {
        qCritical("%s: cannot detect file format from name", qPrintable(fileName));
        return false;
    }
    if (!isFileFormatSupported(cleanedFormat)) {
        qCritical("%s: file format not supported (library missing)", qPrintable(fileName));
        return false;
    }
    if (asyncExporter) {
        if (asyncExporter->_asyncExportFileNames.contains(fileName)) {
            qCritical("%s: cannot have more than one async export per file", qPrintable(fileName));
            return false;
        }
    }
    if (channelsList.size() != 0 && channelsList.size() != dataList.size()) {
        qCritical("%s: number of channel lists does not match number of data", qPrintable(fileName));
        return false;
    }
    for (int i = 0; i < channelsList.size(); i++) {
        for (int j = 0; j < channelsList[i].size(); j++) {
            if (channelsList[i][j] < 0 || channelsList[i][j] >= dataList[i].channels()) {
                qCritical("%s: invalid channel given", qPrintable(fileName));
                return false;
            }
        }
    }
    cleanedChannelsList.clear();
    if (channelsList.size() == 0) {
        for (int i = 0; i < dataList.size(); i++) {
            QList<int> cleanChannels;
            for (int j = 0; j < dataList[i].channels(); j++)
                cleanChannels.append(j);
            cleanedChannelsList.append(cleanChannels);
        }
    } else {
        for (int i = 0; i < dataList.size(); i++) {
            if (channelsList[i].size() == 0) {
                QList<int> cleanChannels;
                for (int j = 0; j < dataList[i].channels(); j++)
                    cleanChannels.append(j);
                cleanedChannelsList.append(cleanChannels);
            } else {
                cleanedChannelsList.append(channelsList[i]);
            }
        }
    }
    for (int i = 0; i < dataList.size(); i++) {
        if (!isFileFormatCompatible(cleanedFormat, dataList[i], cleanedChannelsList[i])
                || (i > 0 && cleanedFormat == FileFormat_PNG)) {
            qCritical("%s: file format is not compatible with data", qPrintable(fileName));
            return false;
        }
    }
    cleanedCompressionLevel = compressionLevel;
    if (cleanedCompressionLevel < 0)
        cleanedCompressionLevel = 0;
    else if (cleanedCompressionLevel > 9)
        cleanedCompressionLevel = 9;
    return true;
}

bool Exporter::exportData(Exporter* asyncExporter,
        const QString& fileName, FileFormat format,
        const QList<TexData>& dataList,
        const QList<QList<int>>& channelsList,
        int compressionLevel)
{
    FileFormat cleanedFormat;
    QList<QList<int>> cleanedChannelsList;
    int cleanedCompressionLevel;
    if (!checkExportRequest(asyncExporter, fileName, format,
                dataList, channelsList, compressionLevel,
                cleanedFormat, cleanedChannelsList, cleanedCompressionLevel)) {
        return false;
    }
    if (asyncExporter) {
        asyncExporter->_asyncExportFileNames.append(fileName);
        QFuture<bool> exp;
        switch (cleanedFormat) {
        case FileFormat_RAW:
            exp = QtConcurrent::run(writeRAW, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_CSV:
            exp = QtConcurrent::run(writeCSV, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PNM:
            exp = QtConcurrent::run(writePNM, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PNG:
            exp = QtConcurrent::run(writePNG, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PFS:
            exp = QtConcurrent::run(writePFS, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_GTA:
            exp = QtConcurrent::run(writeGTA, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_MAT:
            exp = QtConcurrent::run(writeMAT, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_HDF5:
            exp = QtConcurrent::run(writeHDF, fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_Auto:
            // cannot happen
            break;
        }
        asyncExporter->_asyncExports.append(exp);
        return true;
    } else {
        switch (cleanedFormat) {
        case FileFormat_RAW:
            return writeRAW(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_CSV:
            return writeCSV(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PNM:
            return writePNM(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PNG:
            return writePNG(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_PFS:
            return writePFS(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_GTA:
            return writeGTA(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_MAT:
            return writeMAT(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_HDF5:
            return writeHDF(fileName, dataList, cleanedChannelsList, cleanedCompressionLevel);
            break;
        case FileFormat_Auto:
            // cannot happen
            break;
        }
        return false;
    }
}

bool Exporter::waitForAsyncExports()
{
    bool ok = true;
    for (int i = 0; i < _asyncExports.size(); i++) {
        bool asyncOk = _asyncExports[i].result();
        if (!asyncOk)
            ok = false;
    }
    _asyncExports.clear();
    _asyncExportFileNames.clear();
    return ok;
}

static bool haveDefaultChannels(const TexData& data, const QList<int>& channels)
{
    bool defaultChannels = true;
    if (channels.size() != data.channels())
        defaultChannels = false;
    for (int i = 0; i < channels.size(); i++)
        if (channels[i] != i)
            defaultChannels = false;
    return defaultChannels;
}

bool Exporter::writeRAW(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int /* compressionLevel */)
{
    QFile file(fileName);
    QFile header(fileName + "_header");
    if (!file.open(QIODevice::Append) || !header.open(QIODevice::Append)) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    for (int i = 0; i < dataList.size(); i++) {
        const TexData& data = dataList[i];
        const QList<int>& channels = channelsList[i];
        if (haveDefaultChannels(data, channels)) {
            unsigned int n = file.write(static_cast<const char*>(data.packedData()), data.packedDataSize());
            if (n != data.packedDataSize()) {
                qCritical("%s: write error", qPrintable(fileName));
                return false;
            }
        } else {
            for (int y = 0; y < data.height(); y++) {
                for (int x = 0; x < data.width(); x++) {
                    for (int c = 0; c < channels.size(); c++) {
                        unsigned int n = file.write(static_cast<const char*>(data.element(x, y, channels[c])), data.typeSize());
                        if (n != data.typeSize()) {
                            qCritical("%s: write error", qPrintable(fileName));
                            return false;
                        }
                    }
                }
            }
        }
        QTextStream headerStream(&header);
        headerStream << "dimensions: " << data.width() << " " << data.height() << '\n';
        QString typeString = (data.type() == GL_UNSIGNED_BYTE ? "uint8" : data.type() == GL_UNSIGNED_INT ? "uint32" : "float32");
        headerStream << "components:";
        for (int i = 0; i < channels.size(); i++)
            headerStream << ' ' << typeString;
        headerStream << '\n';
        headerStream << "component names:";
        for (int i = 0; i < channels.size(); i++) {
            QString channelName = (data.channelName(i).isEmpty() ? "unnamed" : data.channelName(i));
            headerStream << ' ' << channelName;
        }
        headerStream << '\n';
    }
    if (!file.flush() || !header.flush()) {
        qCritical("%s: write error", qPrintable(fileName));
        return false;
    }
    file.close();
    header.close();
    return true;
}

bool Exporter::writeCSV(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int /* compressionLevel */)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    for (int i = 0; i < dataList.size(); i++) {
        const TexData& data = dataList[i];
        const QList<int>& channels = channelsList[i];
        for (int c = 0; c < channels.size(); c++) {
            for (int y = 0; y < data.height(); y++) {
                for (int x = 0; x < data.width(); x++) {
                    QString element;
                    const void* p = data.element(x, y, channels[c]);
                    switch (data.type()) {
                    case GL_UNSIGNED_BYTE:
                        element.append(QString::number(*(static_cast<const unsigned char*>(p))));
                        break;
                    case GL_UNSIGNED_INT:
                        element.append(QString::number(*(static_cast<const unsigned int*>(p))));
                        break;
                    case GL_FLOAT:
                        element.append(QString::number(*(static_cast<const float*>(p)), 'g', 9));
                        break;
                    }
                    if (x != data.width() - 1)
                        element.append(',');
                    file.write(qPrintable(element));
                }
                file.write("\n");
            }
            file.write("\n");
        }
    }
    if (!file.flush()) {
        qCritical("%s: write error", qPrintable(fileName));
        return false;
    }
    file.close();
    return true;
}

bool Exporter::writePNM(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int /* compressionLevel */)
{
    FILE* f = std::fopen(qPrintable(fileName), "ab");
    if (!f) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    for (int i = 0; i < dataList.size(); i++) {
        const TexData& data = dataList[i];
        const QList<int>& channels = channelsList[i];
        const unsigned char* imgDataPtr;
        QVector<unsigned char> imgData;
        imgDataPtr = static_cast<const unsigned char*>(data.packedData());
        std::fprintf(f, "P%d\n%d %d\n255\n", channels.size() == 1 ? 5 : 6,
                data.width(), data.height());
        std::fwrite(imgDataPtr, data.width() * data.height() * data.channels(), 1, f);
    }
    if (std::fflush(f) != 0 || std::fclose(f) != 0) {
        qCritical("%s: write error", qPrintable(fileName));
        return false;
    }
    return true;
}

bool Exporter::writePNG(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel)
{
    const TexData& data = dataList[0];
    const QList<int>& channels = channelsList[0];
    if (data.type() == GL_UNSIGNED_BYTE && data.packedLineSize() % 4 == 0) {
        /* fast path for common case */
        QImage img(static_cast<const uchar*>(data.packedData()), data.width(), data.height(),
                channels.size() == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);
        return img.save(fileName, "PNG", 11 * (9 - compressionLevel) + 1);
    } else {
        QImage img(data.width(), data.height(),
                channels.size() == 1 ? QImage::Format_Grayscale8 : QImage::Format_RGB888);
        for (int y = 0; y < data.height(); y++) {
            unsigned char* scanline = img.scanLine(y);
            for (int x = 0; x < data.width(); x++) {
                for (int c = 0; c < channels.size(); c++) {
                    const void* p = data.element(x, y, channels[c]);
                    unsigned char byte = *(static_cast<const unsigned char*>(p));
                    std::memcpy(scanline + x * channels.size() + c, &byte, 1);
                }
            }
        }
        return img.save(fileName, "PNG", 11 * (9 - compressionLevel) + 1);
    }
}

bool Exporter::writePFS(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int /* compressionLevel */)
{
    FILE* f = std::fopen(qPrintable(fileName), "ab");
    if (!f) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    for (int i = 0; i < dataList.size(); i++) {
        const TexData& data = dataList[i];
        const QList<int>& channels = channelsList[i];
        std::fprintf(f, "PFS1\n%d %d\n%d\n0\n", data.width(), data.height(), channels.size());
        for (int c = 0; c < channels.size(); c++) {
            QString channelName = data.channelName(channels[c]);
            if (channelName.isEmpty()) {
                char defaultChannelName[] = "CAMSIM-0";
                defaultChannelName[7] += c;
                channelName = defaultChannelName;
            }
            std::fprintf(f, "%s\n0\n", qPrintable(channelName));
        }
        std::fprintf(f, "ENDH");
        QByteArray channelData;
        for (int c = 0; c < channels.size(); c++) {
            if (data.type() == GL_FLOAT) {
                channelData = data.planarDataArray(channels[c]);
                std::fwrite(channelData.constData(), data.planarDataSize(), 1, f);
            } else {
                channelData.resize(data.width() * data.height() * sizeof(float));
                for (int y = 0; y < data.height(); y++) {
                    for (int x = 0; x < data.width(); x++) {
                        float* dst = reinterpret_cast<float*>(channelData.data()) + y * data.width() + x;
                        const void* src = data.element(x, y, channels[c]);
                        if (data.type() == GL_UNSIGNED_BYTE)
                            *dst = *(static_cast<const unsigned char*>(src));
                        else if (data.type() == GL_UNSIGNED_INT)
                            *dst = *(static_cast<const unsigned int*>(src));
                    }
                }
                std::fwrite(channelData.constData(), channelData.size(), 1, f);
            }
        }
    }
    if (std::fflush(f) != 0 || std::fclose(f) != 0) {
        qCritical("%s: write error", qPrintable(fileName));
        return false;
    }
    return true;
}

bool Exporter::writeGTA(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel)
{
#ifdef HAVE_GTA
    FILE* f = std::fopen(qPrintable(fileName), "ab");
    if (!f) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    try {
        for (int i = 0; i < dataList.size(); i++) {
            const TexData& data = dataList[i];
            const QList<int>& channels = channelsList[i];
            gta::header hdr;
            switch (compressionLevel) {
            case 0:
            default:
                break;
            case 1:
                hdr.set_compression(gta::zlib1);
                break;
            case 2:
                hdr.set_compression(gta::zlib2);
                break;
            case 3:
                hdr.set_compression(gta::zlib3);
                break;
            case 4:
                hdr.set_compression(gta::zlib4);
                break;
            case 5:
                hdr.set_compression(gta::zlib5);
                break;
            case 6:
                hdr.set_compression(gta::zlib6);
                break;
            case 7:
                hdr.set_compression(gta::zlib7);
                break;
            case 8:
                hdr.set_compression(gta::zlib8);
                break;
            case 9:
                hdr.set_compression(gta::zlib9);
                break;
            }
            hdr.set_dimensions(data.width(), data.height());
            gta::type componentType;
            switch (data.type()) {
            case GL_UNSIGNED_BYTE:
                componentType = gta::uint8;
                break;
            case GL_UNSIGNED_INT:
                componentType = gta::uint32;
                break;
            case GL_FLOAT:
            default:
                componentType = gta::float32;
                break;
            }
            if (haveDefaultChannels(data, channels)) {
                std::vector<gta::type> types(data.channels(), componentType);
                hdr.set_components(data.channels(), &(types[0]));
                for (int c = 0; c < data.channels(); c++) {
                    if (!data.channelName(c).isEmpty())
                        hdr.component_taglist(c).set("INTERPRETATION", qPrintable(data.channelName(c)));
                }
                hdr.write_to(f);
                hdr.write_data(f, data.packedData());
            } else {
                std::vector<gta::type> types(channels.size(), componentType);
                hdr.set_components(channels.size(), &(types[0]));
                for (int c = 0; c < channels.size(); c++) {
                    if (!data.channelName(channels[c]).isEmpty())
                        hdr.component_taglist(c).set("INTERPRETATION", qPrintable(data.channelName(channels[c])));
                }
                hdr.write_to(f);
                gta::io_state iostate;
                for (int y = 0; y < data.height(); y++) {
                    for (int x = 0; x < data.width(); x++) {
                        unsigned char element[hdr.element_size()];
                        for (int c = 0; c < channels.size(); c++) {
                            int channel = channels[c];
                            std::memcpy(static_cast<void*>(element + c * data.typeSize()),
                                    data.element(x, y, channel),
                                    data.typeSize());
                        }
                        hdr.write_elements(iostate, f, 1, static_cast<void*>(element));
                    }
                }
            }
        }
    }
    catch (std::exception& e) {
        qCritical("%s: %s", qPrintable(fileName), e.what());
        return false;
    }
    if (std::fflush(f) != 0 || std::fclose(f) != 0) {
        qCritical("%s: write error", qPrintable(fileName));
        return false;
    }
    return true;
#else
    Q_UNUSED(fileName);
    Q_UNUSED(dataList);
    Q_UNUSED(channelsList);
    Q_UNUSED(compressionLevel);
    return false;
#endif
}

bool Exporter::writeMAT(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel)
{
#ifdef HAVE_MATIO
    // delete an existing file because MATIO refuses to overwrite it
    QFile file(fileName); file.remove();
    mat_t *mat = Mat_Create(qPrintable(fileName), NULL);
    if (!mat) {
        qCritical("%s: cannot open for writing", qPrintable(fileName));
        return false;
    }
    int defaultNameCounter = 0;
    for (int i = 0; i < dataList.size(); i++) {
        const TexData& data = dataList[i];
        const QList<int>& channels = channelsList[i];
        enum matio_classes classType;
        enum matio_types dataType;
        switch (data.type()) {
        case GL_UNSIGNED_BYTE:
            classType = MAT_C_UINT8;
            dataType = MAT_T_UINT8;
            break;
        case GL_UNSIGNED_INT:
            classType = MAT_C_UINT32;
            dataType = MAT_T_UINT32;
            break;
        case GL_FLOAT:
        default:
            classType = MAT_C_SINGLE;
            dataType = MAT_T_SINGLE;
            break;
        }
        for (int c = 0; c < channels.size(); c++) {
            QString varName = data.channelName(channels[c]);
            if (varName.isEmpty())
                varName = QString("CAMSIM") + QString::number(defaultNameCounter++);
            size_t dims[2] = { static_cast<size_t>(data.height()), static_cast<size_t>(data.width()) };
            QByteArray transposedPlanarDataArray = data.transposedPlanarDataArray(channels[c]);
            matvar_t *matvar = Mat_VarCreate(qPrintable(varName), classType, dataType, 2, &(dims[0]),
                    transposedPlanarDataArray.data(), MAT_F_DONT_COPY_DATA);
            if (!matvar) {
                qCritical("%s: cannot create variable", qPrintable(fileName));
                Mat_Close(mat);
                return false;
            }
            if (Mat_VarWrite(mat, matvar, compressionLevel > 0 ? MAT_COMPRESSION_ZLIB : MAT_COMPRESSION_NONE) != 0) {
                qCritical("%s: cannot write variable", qPrintable(fileName));
                Mat_Close(mat);
                return false;
            }
            Mat_VarFree(matvar);
        }
    }
    Mat_Close(mat);
    return true;
#else
    Q_UNUSED(fileName);
    Q_UNUSED(dataList);
    Q_UNUSED(channelsList);
    Q_UNUSED(compressionLevel);
    return false;
#endif
}

bool Exporter::writeHDF(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel)
{
#ifdef HAVE_HDF5
    int defaultNameCounter = 0;
    try {
        H5::Exception::dontPrint();
        H5::H5File file(qPrintable(fileName), H5F_ACC_TRUNC);
        H5::FloatType floattype(H5::PredType::NATIVE_FLOAT);
        H5::IntType uchartype(H5::PredType::NATIVE_UCHAR);
        H5::IntType uinttype(H5::PredType::NATIVE_UINT);
        for (int i = 0; i < dataList.size(); i++) {
            const TexData& data = dataList[i];
            const QList<int>& channels = channelsList[i];
            for (int c = 0; c < channels.size(); c++) {
                QString varName = data.channelName(channels[c]);
                if (varName.isEmpty())
                    varName = QString("CAMSIM") + QString::number(defaultNameCounter++);
                QByteArray transposedPlanarDataArray = data.transposedPlanarDataArray(channels[c]);
                hsize_t dims[2] = { static_cast<hsize_t>(data.width()), static_cast<hsize_t>(data.height()) };
                H5::DataSpace dataspace(2, dims);
                H5::DSetCreatPropList proplist;
                if (compressionLevel > 0)
                    proplist.setDeflate(compressionLevel);
                H5::DataSet dataset;
                switch (data.type()) {
                case GL_UNSIGNED_BYTE:
                    dataset = file.createDataSet(qPrintable(varName), uchartype, dataspace, proplist);
                    dataset.write(transposedPlanarDataArray.constData(), H5::PredType::NATIVE_UCHAR);
                    break;
                case GL_UNSIGNED_INT:
                    dataset = file.createDataSet(qPrintable(varName), uinttype, dataspace, proplist);
                    dataset.write(transposedPlanarDataArray.constData(), H5::PredType::NATIVE_UINT);
                    break;
                case GL_FLOAT:
                default:
                    dataset = file.createDataSet(qPrintable(varName), floattype, dataspace, proplist);
                    dataset.write(transposedPlanarDataArray.constData(), H5::PredType::NATIVE_FLOAT);
                    break;
                }
            }
        }
    }
    catch (H5::Exception& error) {
        qCritical("%s: %s", qPrintable(fileName), error.getCDetailMsg());
        return false;
    }
    return true;
#else
    Q_UNUSED(fileName);
    Q_UNUSED(dataList);
    Q_UNUSED(channelsList);
    Q_UNUSED(compressionLevel);
    return false;
#endif
}

}

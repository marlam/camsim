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

#ifndef CAMSIM_EXPORTER_HPP
#define CAMSIM_EXPORTER_HPP

#include <QList>
#include <QFuture>

#include "texdata.hpp"

namespace CamSim {

/*!
 * \brief File format
 */
typedef enum {
    /*! \brief Raw binary data (*.raw), plus a text-based header describing the content (*.raw_header) */
    FileFormat_RAW,
    /*! \brief Comma-separated values (*.csv) */
    FileFormat_CSV,
    /*! \brief Portable anymap (*.pgm or *.ppm) */
    FileFormat_PNM,
    /*! \brief Portable Network Graphics (*.png) */
    FileFormat_PNG,
    /*! \brief Portable floatmap streams (*.pfs) */
    FileFormat_PFS,
    /*! \brief Generic Tagged Arrays (*.gta); requires libgta */
    FileFormat_GTA,
    /*! \brief Matlab files (*.mat); requires libmatio */
    FileFormat_MAT,
    /*! \brief HDF5 files (*.h5); requires libhdf5 */
    FileFormat_HDF5,
    /*! \brief Autodetect file format from file name */
    FileFormat_Auto
} FileFormat;

/**
 * \brief Export simulation results to various file formats
 */
class Exporter
{
private:
    QList<QString> _asyncExportFileNames;
    QList<QFuture<bool>> _asyncExports;

    static bool checkExportRequest(Exporter* asyncExporter,
            const QString& fileName, FileFormat format,
            const QList<TexData>& dataList, const QList<QList<int>>& channelsList,
            int compressionLevel,
            FileFormat& cleanedFormat,
            QList<QList<int>>& cleanedChannelsList,
            int& cleanedCompressionLevel);

    static bool exportData(Exporter* asyncExporter,
            const QString& fileName, FileFormat format,
            const QList<TexData>& dataList,
            const QList<QList<int>>& channelsList,
            int compressionLevel);

    static bool writeRAW(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writeCSV(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writePNM(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writePNG(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writePFS(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writeGTA(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writeMAT(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);
    static bool writeHDF(const QString fileName, const QList<TexData> dataList, const QList<QList<int>> channelsList, int compressionLevel);

public:
    /*! \brief Constructor */
    Exporter();

    /*! \brief Return whether the given file format is supported (it may not be if a library was missing at build time) */
    static bool isFileFormatSupported(FileFormat format);

    /*! \brief Return whether the given file format can store the subset of the \a data that
     * is selected by \a channels. */
    static bool isFileFormatCompatible(FileFormat format, const TexData& data, const QList<int>& channels = QList<int>());

    /*! \brief Return a file format based on \a fileName. */
    static FileFormat fileFormatFromName(const QString& fileName);

    /*! \brief Export the \a data to the file named \a fileName.
     * Optionally select a list of \a channels to export.
     *
     * The file format is determined automatically from the filename, see \a fileFormatFromName().
     *
     * If the file already exists, the data is appended to it (RAW, PFS, GTA). Otherwise,
     * the existing file is overwritten (CSV, PNG, MAT).
     *
     * If \a channels is empty, all the channels contained in \a data are exported.
     *
     * The effect of the \a compressionLevel parameter depends on the file format. Values between 0
     * and 9 are allowed, where 0 corresponds to no compression and 9 to best compression.
     *
     * Return true on success, false on error.
     *
     * Example: exportData("file.csv", data, { 0, 1, 2 });
     */
    static bool exportData(const QString& fileName, const TexData& data,
            const QList<int>& channels = QList<int>(), int compressionLevel = 0)
    {
        return exportData(nullptr, fileName, FileFormat_Auto,
                QList<TexData>({ data }), QList<QList<int>>({ channels }),
                compressionLevel);
    }

    /*! \brief Same as \a exportData, but you can choose the file \a format explicitly. */
    static bool exportData(const QString& fileName, FileFormat format, const TexData& data,
            const QList<int>& channels = QList<int>(), int compressionLevel = 0)
    {
        return exportData(nullptr, fileName, format,
                QList<TexData>({ data }), QList<QList<int>>({ channels }),
                compressionLevel);
    }

    /*! \brief Same as \a exportData, but for multiple data sets destined for the same file. */
    static bool exportData(const QString& fileName, const QList<TexData>& dataList,
            const QList<QList<int>>& channelsList = QList<QList<int>>(),
            int compressionLevel = 0)
    {
        return exportData(nullptr, fileName, FileFormat_Auto, dataList, channelsList, compressionLevel);
    }

    /*! \brief Same as \a exportData for multiple data sets, but you can choose the file \a format explicitly. */
    bool exportData(const QString& fileName, FileFormat format, const QList<TexData>& dataList,
            const QList<QList<int>>& channelsList = QList<QList<int>>(),
            int compressionLevel = 0)
    {
        return exportData(nullptr, fileName, format, dataList, channelsList, compressionLevel);
    }


    /*! \brief Same as \a exportData, but the export is started asynchronously in a separate
     * thread and this function returns immediately.
     *
     * You can use this to make best use of both the GPU and your CPU cores: export
     * all of the data from the last simulated frame while simulating the new frame.
     * This is useful especially for compressing file formats where the export eats
     * a lot of CPU cycles.
     *
     * This function returns success if the export was successfully launched.
     *
     * Call \a waitForAsyncExports to wait for all asynchronously started exports to finish.
     */
    bool asyncExportData(const QString& fileName, const TexData& data,
            const QList<int>& channels = QList<int>(), int compressionLevel = 0)
    {
        return exportData(this, fileName, FileFormat_Auto,
                QList<TexData>({ data }), QList<QList<int>>({ channels }),
                compressionLevel);
    }

    /*! \brief Same as \a asyncExportData, but you can choose the file \a format explicitly. */
    bool asyncExportData(const QString& fileName, FileFormat format, const TexData& data,
            const QList<int>& channels = QList<int>(), int compressionLevel = 0)
    {
        return exportData(this, fileName, format,
                QList<TexData>({ data }), QList<QList<int>>({ channels }),
                compressionLevel);
    }

    /*! \brief Same as \a asyncExportData, but for multiple data sets destined for the
     * same file.
     */
    bool asyncExportData(const QString& fileName, const QList<TexData>& dataList,
            const QList<QList<int>>& channelsList = QList<QList<int>>(),
            int compressionLevel = 0)
    {
        return exportData(this, fileName, FileFormat_Auto, dataList, channelsList, compressionLevel);
    }

    /*! \brief Same as \a asyncExportData, but you can choose the file \a format explicitly. */
    bool asyncExportData(const QString& fileName, FileFormat format, const QList<TexData>& dataList,
            const QList<QList<int>>& channelsList = QList<QList<int>>(),
            int compressionLevel = 0)
    {
        return exportData(this, fileName, format, dataList, channelsList, compressionLevel);
    }

    /*! \brief Wait for all asynchronous exports to finish. Returns true if all of them were
     * successful, and false if at least one of them failed. */
    bool waitForAsyncExports();
};

}

#endif

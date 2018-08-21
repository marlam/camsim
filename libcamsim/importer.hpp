/*
 * Copyright (C) 2016, 2017, 2018
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

#ifndef CAMSIM_IMPORTER_HPP
#define CAMSIM_IMPORTER_HPP

#include <QMatrix4x4>
#include <QString>

#include "scene.hpp"

namespace CamSim {

class ImporterInternals;

class Importer
{
private:
    ImporterInternals* _internals;
    QString _fileName;
    QMatrix4x4 _transformationMatrix;

public:
    /**
     * \name Constructor / Destructor
     */
    /*@{*/

    /*! \brief Constructor */
    Importer();
    /*! \brief Destructor */
    ~Importer();

    /*@}*/

    /**
     * \name Importing objects, materials, light sources
     */
    /*@{*/

    /*! \brief Import from the file with the given \a fileName */
    bool import(const QString& fileName);

    /*! \brief Set a global transformation matrix for all imported data */
    void setTransformationMatrix(const QMatrix4x4 transformationMatrix)
    {
        _transformationMatrix = transformationMatrix;
    }

    /*! \brief Add all imported light sources to a scene, with corresponding animations */
    void addLightsToScene(Scene& scene) const;
    /*! \brief Get the number of light sources in the imported data */
    int lightCount() const;
    /*! \brief Add an imported light source to a scene, with a corresponding animation */
    void addLightToScene(int lightSourceIndex, Scene& scene) const;

    /*! \brief Add all imported shapes combined into one object to a scene, with corresponding animations.
     * This function creates OpenGL objects in the current OpenGL context. */
    void addObjectToScene(Scene& scene) const;

    /*@}*/

    /**
     * \name Importing objects, materials, light sources
     */
    /*@{*/

    /*! \brief Import a texture from the file with the given \a fileName.
     * Returns zero on failure.
     *
     * This function currently supports 8-bit-per-channel image formats
     * that Qt can load (e.g. PNG, JPEG). This will probably extended in the
     * future for file formats that can hold floating point data (e.g. GTA). */
    static unsigned int importTexture(const QString& fileName);

    /*@}*/
};

}

#endif

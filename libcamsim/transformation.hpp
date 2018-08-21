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

#ifndef CAMSIM_TRANSFORMATION_HPP
#define CAMSIM_TRANSFORMATION_HPP

#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>

namespace CamSim {

/**
 * \brief Defines a pose, consisting of translation, rotation, and scaling.
 */
class Transformation
{
public:
    /*! \brief Constructor for zero translation, zero rotation, and scale factor 1 */
    Transformation();

    /*! \brief Translation */
    QVector3D translation;
    /*! \brief Rotation */
    QQuaternion rotation;
    /*! \brief Scaling */
    QVector3D scaling;

    /*! \brief Return this pose as a 4x4 matrix */
    QMatrix4x4 toMatrix4x4() const;

    /*! \brief Extract a pose from a 4x4 matrix. This should be avoided if possible.
     * This function assumes that the matrix contains tanslation, rotation, and
     * positive scaling components, and nothing else. */
    static Transformation fromMatrix4x4(const QMatrix4x4& M);

    /*! \brief Interpolate two poses. The value of \a alpha should be in [0,1],
     * where 0 results in \a p0, and 1 results in \a p1. Positions are interpolated
     * linearly and rotations are interpolated via spherical linear interpolation (slerp). */
    static Transformation interpolate(const Transformation& p0, const Transformation& p1, float alpha);
};

}

#endif

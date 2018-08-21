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

#include "transformation.hpp"

namespace CamSim {

Transformation::Transformation() :
    translation(), rotation(), scaling(1.0f, 1.0f, 1.0f)
{
}

QMatrix4x4 Transformation::toMatrix4x4() const
{
    QMatrix4x4 M;
    M.translate(translation);
    M.rotate(rotation);
    M.scale(scaling);
    return M;
}

Transformation Transformation::fromMatrix4x4(const QMatrix4x4& M)
{
    Transformation p;
    p.translation = M.column(3).toVector3D();
    p.scaling = QVector3D(
            M.column(0).toVector3D().length(),
            M.column(1).toVector3D().length(),
            M.column(2).toVector3D().length());
    QMatrix4x4 R;
    R.setColumn(0, M.column(0) / p.scaling.x());
    R.setColumn(1, M.column(1) / p.scaling.y());
    R.setColumn(2, M.column(2) / p.scaling.z());
    p.rotation = QQuaternion::fromRotationMatrix(R.toGenericMatrix<3, 3>());
    return p;
}

Transformation Transformation::interpolate(const Transformation& p0, const Transformation& p1, float alpha)
{
    Transformation p;
    p.translation = (1.0f - alpha) * p0.translation + alpha * p1.translation;
    p.rotation = QQuaternion::slerp(p0.rotation, p1.rotation, alpha);
    p.scaling = (1.0f - alpha) * p0.scaling + alpha * p1.scaling;
    return p;
}

}

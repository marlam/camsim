/*
 * Copyright (C) 2012, 2013, 2014, 2017, 2018
 * Computer Graphics Group, University of Siegen, Germany.
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>.
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

#include <cmath>

#include <QtMath>
#include <QFile>
#include <QTextStream>

#include "animation.hpp"


namespace CamSim {

static const float epsilon = 0.0001f;

Animation::Animation()
{
}

Animation::Animation(const QList<Keyframe>& kfs) : _keyframes(kfs)
{
}

static void findKeyframeIndices(const QList<Animation::Keyframe>& keyframes,
        long long t, int& lowerIndex, int& higherIndex)
{
    // Binary search for the two nearest keyframes.
    // At this point we know that if we have only one keyframe, then its
    // time stamp is the requested time stamp so we have an exact match below.
    int a = 0;
    int b = keyframes.size() - 1;
    while (b >= a) {
        int c = (a + b) / 2;
        if (keyframes[c].t < t) {
            a = c + 1;
        } else if (keyframes[c].t > t) {
            b = c - 1;
        } else {
            // exact match
            lowerIndex = c;
            higherIndex = c;
            return;
        }
    }
    // Now we have two keyframes: b is the lower one and a is the higher one.
    lowerIndex = b;
    higherIndex = a;
}

void Animation::addKeyframe(const Keyframe& keyframe)
{
    if (_keyframes.size() == 0) {
        _keyframes.append(keyframe);
    } else if (keyframe.t > endTime()) {
        // fast path for common case
        _keyframes.append(keyframe);
    } else if (keyframe.t < startTime()) {
        // fast path for common case
        _keyframes.prepend(keyframe);
    } else {
        int lowerIndex, higherIndex;
        findKeyframeIndices(_keyframes, keyframe.t, lowerIndex, higherIndex);
        if (lowerIndex == higherIndex) {
            _keyframes[lowerIndex] = keyframe;
        } else {
            _keyframes.insert(higherIndex, keyframe);
        }
    }
}

Transformation Animation::interpolate(long long t)
{
    // Catch corner cases
    if (_keyframes.size() == 0) {
        return Transformation();
    } else if (t <= startTime()) {
        return _keyframes.front().transformation;
    } else if (t >= endTime()) {
        return _keyframes.back().transformation;
    }

    int lowerIndex, higherIndex;
    findKeyframeIndices(_keyframes, t, lowerIndex, higherIndex);
    if (lowerIndex == higherIndex) {
        return _keyframes[lowerIndex].transformation;
    }

    // Compute alpha value for interpolation.
    float alpha = 1.0f - static_cast<float>(_keyframes[higherIndex].t - t) / (_keyframes[higherIndex].t - _keyframes[lowerIndex].t);

    // Interpolate transformation
    return Transformation::interpolate(
            _keyframes[lowerIndex].transformation,
            _keyframes[higherIndex].transformation, alpha);
}

bool Animation::load(const QString& filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        qCritical("animation file %s: cannot open", qPrintable(filename));
        return false;
    }
    QTextStream in(&f);

    QList<Keyframe> keyframesBackup = _keyframes;
    _keyframes.clear();

    int lineCounter = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        lineCounter++;
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        Keyframe keyframe;
        QStringList tokens = line.simplified().split(' ', QString::SkipEmptyParts);
        int i = 0;
        while (i < tokens.size()) {
            if (tokens[i] == "time" && i + 1 < tokens.size()) {
                keyframe.t += tokens[i + 1].toDouble() * 1000000;
                i += 2;
            } else if (tokens[i] == "pos:cart" && i + 3 < tokens.size()) {
                keyframe.transformation.translation += QVector3D(tokens[i + 1].toDouble(),
                        tokens[i + 2].toDouble(), tokens[i + 3].toDouble());
                i += 4;
            } else if (tokens[i] == "pos:cyl" && i + 3 < tokens.size()) {
                double tmp[] = { tokens[i + 1].toDouble(),
                    qDegreesToRadians(-tokens[i + 2].toDouble()),
                    tokens[i + 3].toDouble() };
                keyframe.transformation.translation += QVector3D(tmp[0] * std::sin(tmp[1]), tmp[2], -tmp[0] * std::cos(tmp[1]));
                i += 4;
            } else if (tokens[i] == "pos:sph" && i + 3 < tokens.size()) {
                double tmp[] = { tokens[i + 1].toDouble(),
                    qDegreesToRadians(-tokens[i + 2].toDouble()),
                    qDegreesToRadians(tokens[i + 3].toDouble()) };
                keyframe.transformation.translation += QVector3D(tmp[0] * std::cos(tmp[2]) * std::sin(tmp[1]),
                        tmp[0] * std::sin(tmp[2]), tmp[0] * std::cos(tmp[2]) * std::cos(tmp[1]));
                i += 4;
            } else if (tokens[i] == "rot:axisangle" && i + 4 < tokens.size()) {
                double tmp[] = { tokens[i + 1].toDouble(), tokens[i + 2].toDouble(),
                    tokens[i + 3].toDouble(), tokens[i + 4].toDouble() };
                keyframe.transformation.rotation *= QQuaternion::fromAxisAndAngle(tmp[0], tmp[1], tmp[2], tmp[3]);
                i += 5;
            } else if (tokens[i] == "rot:dir" && i + 6 < tokens.size()) {
                double tmp[] = { tokens[i + 1].toDouble(), tokens[i + 2].toDouble(),
                    tokens[i + 3].toDouble(), tokens[i + 4].toDouble(),
                    tokens[i + 4].toDouble(), tokens[i + 5].toDouble() };
                keyframe.transformation.rotation *= QQuaternion::fromDirection(
                        QVector3D(tmp[0], tmp[1], tmp[2]), QVector3D(tmp[3], tmp[4], tmp[5]));
                i += 7;
            } else if (tokens[i] == "rot:euler" && i + 3 < tokens.size()) {
                double tmp[] = { tokens[i + 1].toDouble(), tokens[i + 2].toDouble(),
                    tokens[i + 3].toDouble() };
                keyframe.transformation.rotation *= QQuaternion::fromEulerAngles(tmp[0], tmp[1], tmp[2]);
                i += 4;
            } else if (tokens[i] == "scale" && i + 3 < tokens.size()) {
                keyframe.transformation.scaling *= QVector3D(tokens[i + 1].toDouble(),
                        tokens[i + 2].toDouble(), tokens[i + 3].toDouble());
                i += 4;
            } else {
                qCritical("animation file %s line %d: invalid token %d ('%s')",
                        qPrintable(filename), lineCounter, i, qPrintable(tokens[i]));
                _keyframes = keyframesBackup;
                return false;
            }
        }
        addKeyframe(keyframe);
    }
    return true;
}

}

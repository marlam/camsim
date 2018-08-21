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

#ifndef CAMSIM_ANIMATION_HPP
#define CAMSIM_ANIMATION_HPP

#include <QList>

#include "transformation.hpp"

namespace CamSim {

/**
 * \brief The Animation class.
 *
 * This class describes an animation through a set key frames, which are points
 * in time at which the transformation of an object is known.
 *
 * Transformations at arbitrary points in time are interpolated from these key frames.
 *
 * Positions are interpolated linearly, and rotations are interpolated via
 * spherical linear interpolation (slerp).
 *
 * The translation of a target is measured relative to the origin, and its
 * orientation is given by a rotation angle around a rotation axis.
 *
 * Example: if you animate an object and your camera is in the origin, looking
 * along -z, with up vector +y, and everything is measured in meters:
 * - Target translation (0, 0, -0.2) means that the origin of the target is
 *   directly in front of the camera at 20cm distance.
 * - Zero rotation means that the target is upright, while a rotation of 180
 *   degrees around the z axis means that it is upside down.
 */
class Animation
{
public:
    /**
     * \brief The Keyframe class.
     *
     * This class describes one key frame, consisting of a point in time and
     * the transformation of a target at this point in time.
     */
    class Keyframe
    {
    public:
        long long t;            /**< \brief Key frame time in microseconds. */
        Transformation transformation; /**< \brief Transformation of the target at time t. */

        Keyframe() : t(0) {}
        Keyframe(long long usecs, const Transformation& transf) : t(usecs), transformation(transf) {}
    };

private:
    QList<Keyframe> _keyframes; // list of keyframes, sorted by ascending time

public:
    /** \brief Constructor
     *
     * Constructs a default animation (one keyframe at t=0 with default transformation).
     */
    Animation();

    /** \brief Constructor
     *
     * Constructs an animation from the given list of keyframes
     */
    Animation(const QList<Keyframe>& keyframes);

    /** \brief Get the current list of keyframes, sorted by time in ascending order */
    const QList<Keyframe>& keyframes()
    {
        return _keyframes;
    }

    /** \brief Add a key frame. If a key frame with the same time stamp already exists,
     * it is overwritten. */
    void addKeyframe(const Keyframe& keyframe);

    /** \brief Convenience wrapper for \a addKeyFrame(). */
    void addKeyframe(long long usecs, const Transformation& transf)
    {
        addKeyframe(Keyframe(usecs, transf));
    }

    /** \brief Get start time of animation.
     *
     * Returns the time of the first keyframe.
     */
    long long startTime() const
    {
        return _keyframes.size() == 0 ? 0 : _keyframes.front().t;
    }

    /** \brief Get end time of animation.
     *
     * Returns the time of the first keyframe.
     */
    long long endTime() const
    {
        return _keyframes.size() == 0 ? 0 : _keyframes.back().t;
    }

    /** \brief Interpolate transformation
     *
     * \param t         Point in time, in microseconds
     *
     * Returns the transformation at the given point in time.
     */
    Transformation interpolate(long long t);

    /** \brief Load animation description from a file
     *
     * Returns true on success, false on error. */
    bool load(const QString& filename);
};

}

#endif

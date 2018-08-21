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

#ifndef CAMSIM_GENERATOR_HPP
#define CAMSIM_GENERATOR_HPP

#include <QMatrix4x4>
#include <QString>

#include "scene.hpp"

namespace CamSim {

/**
 * \brief Generates basic objects to populate a scene
 */
class Generator
{
public:
    /*! \brief Adds geometry as an object to a scene. */
    static void addObjectToScene(Scene& scene, int materialIndex,
            int vertexCount,
            const float* positions,
            const float* normals,
            const float* texCoords,
            int indexCount,
            const unsigned int* indices,
            const Transformation& transformation = Transformation(),
            const Animation& animation = Animation());

    /*! \brief Adds a geometry as an object to a scene. Convenient
     * alternative to the above function; it does the same */
    static void addObjectToScene(Scene& scene, int materialIndex,
            const QVector<float>& positions,
            const QVector<float>& normals,
            const QVector<float>& texCoords,
            const QVector<unsigned int>& indices,
            const Transformation& transformation = Transformation(),
            const Animation& animation = Animation());

    /*! \brief Adds a quad object to a scene.
     * The initial quad has the corners (-1, -1, 0), (+1, -1, 0), (+1, +1, 0), (-1, +1, 0).
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addQuadToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(), int slices = 40);

    /*! \brief Adds a cube object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addCubeToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(), int slices = 40);

    /*! \brief Adds a disk object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addDiskToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(),
            float innerRadius = 0.2f, int slices = 40);

    /*! \brief Adds a sphere object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addSphereToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(),
            int slices = 40, int stacks = 20);

    /*! \brief Adds a cylinder object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addCylinderToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(),
            int slices = 40);

    /*! \brief Adds a cone object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addConeToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(),
            int slices = 40, int stacks = 20);

    /*! \brief Adds a torus object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addTorusToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation(),
            float innerRadius = 0.4f, int sides = 40, int rings = 40);

    /*! \brief Adds an Stanford Armadillo object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     * Note that unlike most other objects provided in this class, this one
     * does not have texture coordinates.
     */
    static void addArmadilloToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation());

    /*! \brief Adds a Stanford Happy Buddha object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     * Note that unlike most other objects provided in this class, this one
     * does not have texture coordinates.
     */
    static void addBuddhaToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation());

    /*! \brief Adds a Stanford Bunny object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addBunnyToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation());

    /*! \brief Adds a Stanford Dragon object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     * Note that unlike most other objects provided in this class, this one
     * does not have texture coordinates.
     */
    static void addDragonToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation());

    /*! \brief Adds a teapot object to a scene.
     * The initial geometry is centered on the origin and fills [-1,+1]^3.
     * The \a transformation is applied to this geometry.
     * The shape material is given by \a materialIndex, which indexes a material in
     * the global list of materials in \a scene.
     */
    static void addTeapotToScene(Scene& scene, int materialIndex,
            const Transformation& transformation,
            const Animation& animation = Animation());
};

}

#endif

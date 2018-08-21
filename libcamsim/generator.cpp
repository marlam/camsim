/*
 * Copyright (C) 2013, 2014, 2015, 2016, 2017, 2018
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

#define _USE_MATH_DEFINES
#include <cmath>

#include "generator.hpp"
#include "gl.hpp"

extern "C" {
#include "models/armadillo.h"
#include "models/buddha.h"
#include "models/bunny.h"
#include "models/dragon.h"
#include "models/teapot.h"
}

namespace CamSim {

static const float pi = M_PI;
static const float half_pi = M_PI_2;
static const float two_pi = 2 * M_PI;

void Generator::addObjectToScene(Scene& scene, int materialIndex,
        int vertexCount,
        const float* positions,
        const float* normals,
        const float* texCoords,
        int indexCount,
        const unsigned int* indices,
        const Transformation& transformation,
        const Animation& animation)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    std::vector<float> data;

    QMatrix4x4 transformationMatrix = transformation.toMatrix4x4();
    QMatrix4x4 normalMatrix = QMatrix4x4(transformationMatrix.normalMatrix());

    Shape shape;
    shape.materialIndex = materialIndex;
    gl->glGenVertexArrays(1, &(shape.vao));
    gl->glBindVertexArray(shape.vao);
    GLuint positionBuffer;
    gl->glGenBuffers(1, &positionBuffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
    data.resize(vertexCount * 3);
    for (int j = 0; j < vertexCount; j++) {
        QVector3D v = QVector3D(positions[3 * j + 0], positions[3 * j + 1], positions[3 * j + 2]);
        QVector3D w = transformationMatrix * v;
        data[3 * j + 0] = w.x();
        data[3 * j + 1] = w.y();
        data[3 * j + 2] = w.z();
    }
    gl->glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), data.data(), GL_STATIC_DRAW);
    gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    gl->glEnableVertexAttribArray(0);
    GLuint normalBuffer;
    gl->glGenBuffers(1, &normalBuffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    data.resize(vertexCount * 3);
    for (int j = 0; j < vertexCount; j++) {
        QVector3D v = QVector3D(normals[3 * j + 0], normals[3 * j + 1], normals[3 * j + 2]);
        QVector3D w = normalMatrix * v;
        data[3 * j + 0] = w.x();
        data[3 * j + 1] = w.y();
        data[3 * j + 2] = w.z();
    }
    gl->glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), data.data(), GL_STATIC_DRAW);
    gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    gl->glEnableVertexAttribArray(1);
    GLuint texcoordBuffer;
    gl->glGenBuffers(1, &texcoordBuffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
    gl->glBufferData(GL_ARRAY_BUFFER, vertexCount * 2 * sizeof(float), texCoords, GL_STATIC_DRAW);
    gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    gl->glEnableVertexAttribArray(2);
    GLuint indexBuffer;
    gl->glGenBuffers(1, &indexBuffer);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
    shape.indices = indexCount;

    Object object;
    object.shapes.append(shape);

    scene.objects.append(object);
    scene.objectAnimations.append(animation);
}

void Generator::addObjectToScene(Scene& scene, int materialIndex,
        const QVector<float>& positions,
        const QVector<float>& normals,
        const QVector<float>& texCoords,
        const QVector<unsigned int>& indices,
        const Transformation& transformation,
        const Animation& animation)
{
    Q_ASSERT(positions.size() > 0);
    Q_ASSERT(positions.size() % 3 == 0);
    Q_ASSERT(positions.size() == normals.size());
    Q_ASSERT(texCoords.size() % 2 == 0);
    Q_ASSERT(positions.size() / 3 == texCoords.size() / 2);
    Q_ASSERT(indices.size() > 0);
    Q_ASSERT(indices.size() % 3 == 0);

    addObjectToScene(scene, materialIndex,
            positions.size() / 3,
            positions.constData(),
            normals.constData(),
            texCoords.constData(),
            indices.size(),
            indices.constData(),
            transformation, animation);
}

void Generator::addQuadToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation, int slices)
{
    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    for (int i = 0; i <= slices; i++) {
        float ty = i / (slices / 2.0f);
        for (int j = 0; j <= slices; j++) {
            float tx = j / (slices / 2.0f);
            float x = -1.0f + tx;
            float y = -1.0f + ty;
            float z = 0.0f;
            positions.append(x);
            positions.append(y);
            positions.append(z);
            normals.append(0.0f);
            normals.append(0.0f);
            normals.append(1.0f);
            texcoords.append(tx / 2.0f);
            texcoords.append(ty / 2.0f);
            if (i < slices && j < slices) {
                indices.append((i + 0) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
            }
        }
    }

    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addCubeToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation, int slices)
{
    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    int verticesPerSide = (slices + 1) * (slices + 1);
    for (int side = 0; side < 6; side++) {
        float nx, ny, nz;
        switch (side) {
        case 0: // front
            nx = 0.0f;
            ny = 0.0f;
            nz = +1.0f;
            break;
        case 1: // back
            nx = 0.0f;
            ny = 0.0f;
            nz = -1.0f;
            break;
        case 2: // left
            nx = -1.0f;
            ny = 0.0f;
            nz = 0.0f;
            break;
        case 3: // right
            nx = +1.0f;
            ny = 0.0f;
            nz = 0.0f;
            break;
        case 4: // top
            nx = 0.0f;
            ny = +1.0f;
            nz = 0.0f;
            break;
        case 5: // bottom
            nx = 0.0f;
            ny = -1.0f;
            nz = 0.0f;
            break;
        }
        for (int i = 0; i <= slices; i++) {
            float ty = i / (slices / 2.0f);
            for (int j = 0; j <= slices; j++) {
                float tx = j / (slices / 2.0f);
                float x, y, z;
                switch (side) {
                case 0: // front
                    x = -1.0f + tx;
                    y = -1.0f + ty;
                    z = 1.0f;
                    break;
                case 1: // back
                    x = 1.0f - tx;
                    y = -1.0f + ty;
                    z = -1.0f;
                    break;
                case 2: // left
                    x = -1.0f;
                    y = -1.0f + ty;
                    z = -1.0f + tx;
                    break;
                case 3: // right
                    x = 1.0f;
                    y = -1.0f + ty;
                    z = 1.0f - tx;
                    break;
                case 4: // top
                    x = -1.0f + ty;
                    y = 1.0f;
                    z = -1.0f + tx;
                    break;
                case 5: // bottom
                    x = 1.0f - ty;
                    y = -1.0f;
                    z = -1.0f + tx;
                    break;
                }
                positions.append(x);
                positions.append(y);
                positions.append(z);
                normals.append(nx);
                normals.append(ny);
                normals.append(nz);
                texcoords.append(tx / 2.0f);
                texcoords.append(ty / 2.0f);
                if (i < slices && j < slices) {
                    indices.append(side * verticesPerSide + (i + 0) * (slices + 1) + (j + 0));
                    indices.append(side * verticesPerSide + (i + 0) * (slices + 1) + (j + 1));
                    indices.append(side * verticesPerSide + (i + 1) * (slices + 1) + (j + 0));
                    indices.append(side * verticesPerSide + (i + 0) * (slices + 1) + (j + 1));
                    indices.append(side * verticesPerSide + (i + 1) * (slices + 1) + (j + 1));
                    indices.append(side * verticesPerSide + (i + 1) * (slices + 1) + (j + 0));
                }
            }
        }
    }

    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addDiskToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation,
        float innerRadius, int slices)
{
    const int loops = 1;

    Q_ASSERT(innerRadius >= 0.0f);
    Q_ASSERT(innerRadius <= 1.0f);
    Q_ASSERT(slices >= 4);
    Q_ASSERT(loops >= 1);

    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    for (int i = 0; i <= loops; i++) {
        float ty = static_cast<float>(i) / loops;
        float r = innerRadius + ty * (1.0f - innerRadius);
        for (int j = 0; j <= slices; j++) {
            float tx = static_cast<float>(j) / slices;
            float alpha = tx * two_pi + half_pi;
            positions.append(r * std::cos(alpha));
            positions.append(r * std::sin(alpha));
            positions.append(0.0f);
            normals.append(0.0f);
            normals.append(0.0f);
            normals.append(1.0f);
            texcoords.append(1.0f - tx);
            texcoords.append(ty);
            if (i < loops && j < slices) {
                indices.append((i + 0) * (slices + 1) + (j + 0));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 1) * (slices + 1) + (j + 1));
            }
        }
    }
    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addSphereToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation,
        int slices, int stacks)
{
    Q_ASSERT(slices >= 4);
    Q_ASSERT(stacks >= 2);

    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    for (int i = 0; i <= stacks; i++) {
        float ty = static_cast<float>(i) / stacks;
        float lat = ty * pi;
        for (int j = 0; j <= slices; j++) {
            float tx = static_cast<float>(j) / slices;
            float lon = tx * two_pi - half_pi;
            float sinlat = std::sin(lat);
            float coslat = std::cos(lat);
            float sinlon = std::sin(lon);
            float coslon = std::cos(lon);
            float x = sinlat * coslon;
            float y = coslat;
            float z = sinlat * sinlon;
            positions.append(x);
            positions.append(y);
            positions.append(z);
            normals.append(x);
            normals.append(y);
            normals.append(z);
            texcoords.append(1.0f - tx);
            texcoords.append(1.0f - ty);
            if (i < stacks && j < slices) {
                indices.append((i + 0) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
            }
        }
    }
    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addCylinderToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation,
        int slices)
{
    const int stacks = 1;

    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    for (int i = 0; i <= stacks; i++) {
        float ty = static_cast<float>(i) / stacks;
        for (int j = 0; j <= slices; j++) {
            float tx = static_cast<float>(j) / slices;
            float alpha = tx * two_pi - half_pi;
            float x = std::cos(alpha);
            float y = -(ty * 2.0f - 1.0f);
            float z = std::sin(alpha);
            positions.append(x);
            positions.append(y);
            positions.append(z);
            normals.append(x);
            normals.append(0);
            normals.append(z);
            texcoords.append(1.0f - tx);
            texcoords.append(1.0f - ty);
            if (i < stacks && j < slices) {
                indices.append((i + 0) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
            }
        }
    }
    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addConeToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation,
        int slices, int stacks)
{
    Q_ASSERT(slices >= 4);
    Q_ASSERT(stacks >= 2);

    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    for (int i = 0; i <= stacks; i++) {
        float ty = static_cast<float>(i) / stacks;
        for (int j = 0; j <= slices; j++) {
            float tx = static_cast<float>(j) / slices;
            float alpha = tx * two_pi - half_pi;
            float x = ty * std::cos(alpha);
            float y = -(ty * 2.0f - 1.0f);
            float z = ty * std::sin(alpha);
            positions.append(x);
            positions.append(y);
            positions.append(z);
            float nx = x;
            float ny = 0.5f;
            float nz = z;
            float nl = std::sqrt(nx * nx + ny * ny + nz * nz);
            normals.append(nx / nl);
            normals.append(ny / nl);
            normals.append(nz / nl);
            texcoords.append(1.0f - tx);
            texcoords.append(1.0f - ty);
            if (i < stacks && j < slices) {
                indices.append((i + 0) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
                indices.append((i + 0) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 1));
                indices.append((i + 1) * (slices + 1) + (j + 0));
            }
        }
    }
    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addTorusToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation,
        float innerRadius, int sides, int rings)
{
    Q_ASSERT(innerRadius >= 0.0f);
    Q_ASSERT(innerRadius < 1.0f);
    Q_ASSERT(sides >= 4);
    Q_ASSERT(rings >= 4);

    QVector<float> positions, normals, texcoords;
    QVector<unsigned int> indices;

    float ringradius = (1.0f - innerRadius) / 2.0f;
    float ringcenter = innerRadius + ringradius;

    for (int i = 0; i <= sides; i++) {
        float ty = static_cast<float>(i) / sides;
        float alpha = ty * two_pi - half_pi;
        float c = std::cos(alpha);
        float s = std::sin(alpha);
        for (int j = 0; j <= rings; j++) {
            float tx = static_cast<float>(j) / rings;
            float beta = tx * two_pi - pi;

            float x = ringcenter + ringradius * std::cos(beta);
            float y = 0.0f;
            float z = ringradius * std::sin(beta);
            float rx = c * x + s * y;
            float ry = c * y - s * x;
            float rz = z;
            positions.append(rx);
            positions.append(ry);
            positions.append(rz);

            float rcx = c * ringcenter;
            float rcy = - s * ringcenter;
            float rcz = 0.0f;
            float nx = rx - rcx;
            float ny = ry - rcy;
            float nz = rz - rcz;
            float nl = std::sqrt(nx * nx + ny * ny + nz * nz);
            normals.append(nx / nl);
            normals.append(ny / nl);
            normals.append(nz / nl);

            texcoords.append(1.0f - tx);
            texcoords.append(1.0f - ty);
            if (i < sides && j < rings) {
                indices.append((i + 0) * (rings + 1) + (j + 0));
                indices.append((i + 0) * (rings + 1) + (j + 1));
                indices.append((i + 1) * (rings + 1) + (j + 0));
                indices.append((i + 0) * (rings + 1) + (j + 1));
                indices.append((i + 1) * (rings + 1) + (j + 1));
                indices.append((i + 1) * (rings + 1) + (j + 0));
            }
        }
    }
    addObjectToScene(scene, materialIndex,
            positions, normals, texcoords, indices,
            transformation, animation);
}

void Generator::addArmadilloToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation)
{
    addObjectToScene(scene, materialIndex,
            armadillo_vertexcount, 
            armadillo_positions,
            armadillo_normals,
            armadillo_texcoords,
            armadillo_indexcount,
            armadillo_indices,
            transformation, animation);
}

void Generator::addBuddhaToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation)
{
    addObjectToScene(scene, materialIndex,
            buddha_vertexcount, 
            buddha_positions,
            buddha_normals,
            buddha_texcoords,
            buddha_indexcount,
            buddha_indices,
            transformation, animation);
}

void Generator::addBunnyToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation)
{
    addObjectToScene(scene, materialIndex,
            bunny_vertexcount, 
            bunny_positions,
            bunny_normals,
            bunny_texcoords,
            bunny_indexcount,
            bunny_indices,
            transformation, animation);
}

void Generator::addDragonToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation)
{
    addObjectToScene(scene, materialIndex,
            dragon_vertexcount, 
            dragon_positions,
            dragon_normals,
            dragon_texcoords,
            dragon_indexcount,
            dragon_indices,
            transformation, animation);
}

void Generator::addTeapotToScene(Scene& scene, int materialIndex,
        const Transformation& transformation,
        const Animation& animation)
{
    addObjectToScene(scene, materialIndex,
            teapot_vertexcount, 
            teapot_positions,
            teapot_normals,
            teapot_texcoords,
            teapot_indexcount,
            teapot_indices,
            transformation, animation);
}

}

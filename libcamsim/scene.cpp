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

#include <cmath>

#include "scene.hpp"
#include "gl.hpp"

namespace CamSim {

Light::Light() :
    type(SpotLight),
    innerConeAngle(20.0f),
    outerConeAngle(30.0f),
    isRelativeToCamera(true),
    position(0.0f, 0.0f, 0.0f),
    direction(0.0f, 0.0f, -1.0f),
    up(0.0f, 1.0f, 0.0f),
    color(1.0f, 1.0f, 1.0f),
    power(0.2f),
    attenuationConstant(1.0f),
    attenuationLinear(0.0f),
    attenuationQuadratic(1.0f),
    shadowMap(true),
    shadowMapSize(256),
    shadowMapDepthBias(0.15f),
    reflectiveShadowMap(true),
    reflectiveShadowMapSize(64),
    powerFactorTex(0),
    powerFactorMapAngleLeft(0.0f),
    powerFactorMapAngleRight(0.0f),
    powerFactorMapAngleBottom(0.0f),
    powerFactorMapAngleTop(0.0f),
    powerFactorMapCallback(nullptr),
    powerFactorMapCallbackData(nullptr)
{
}

void Light::updatePowerFactorTex(unsigned int pbo, long long timestamp)
{
    bool needUpload = false;
    if (powerFactorTex == 0 && (powerFactors.size() > 0 || powerFactorMapCallback)) {
        // texture needs to be created
        auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
        gl->glGenTextures(1, &powerFactorTex);
        gl->glBindTexture(GL_TEXTURE_2D, powerFactorTex);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        needUpload = true;
    }
    if (powerFactorMapCallback) {
        needUpload = powerFactorMapCallback(
                powerFactorMapCallbackData, timestamp,
                &powerFactorMapWidth, &powerFactorMapHeight,
                &powerFactorMapAngleLeft, &powerFactorMapAngleRight,
                &powerFactorMapAngleBottom, &powerFactorMapAngleTop,
                powerFactors) || needUpload;
    }
    if (needUpload) {
        glUploadTex(pbo, powerFactorTex,
                powerFactorMapWidth, powerFactorMapHeight,
                GL_R32F, GL_RED, GL_FLOAT,
                powerFactors.constData(), powerFactors.size() * sizeof(float));
        ASSERT_GLCHECK();
    }
}

Material::Material() :
    type(Phong),
    isTwoSided(false),
    bumpTex(0),
    bumpScaling(8.0f), // arbitrary value
    normalTex(0),
    opacity(1.0f),
    opacityTex(0),
    ambient(0.0f, 0.0f, 0.0f),
    diffuse(0.7f, 0.7f, 0.7f),
    specular(0.3f, 0.3f, 0.3f),
    emissive(0.0f, 0.0f, 0.0f),
    shininess(100.0f),
    ambientTex(0),
    diffuseTex(0),
    specularTex(0),
    emissiveTex(0),
    shininessTex(0),
    lightnessTex(0)
{
}

Material::Material(const QVector3D& diff, const QVector3D& spec, float shin) :
    Material()
{
    diffuse = diff;
    specular = spec;
    shininess = shin;
}

Shape::Shape() :
    materialIndex(-1),
    vao(0),
    indices(0)
{
}

Object::Object() :
    shapes()
{
}

Object::Object(const Shape& shape) :
    shapes()
{
    shapes.append(shape);
}

Scene::Scene() :
    materials(),
    lights(),
    lightAnimations(),
    objects(),
    objectAnimations()
{
}

}

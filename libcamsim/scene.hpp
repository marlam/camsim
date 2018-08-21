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

#ifndef CAMSIM_SCENE_HPP
#define CAMSIM_SCENE_HPP

#include <QList>
#include <QVector3D>

#include "animation.hpp"


/* A scene description suitable for OpenGL-based rendering */

namespace CamSim {

/*!
 * \brief Light source type.
 */
typedef enum {
    /*! \brief A point light source. */
    PointLight = 0,
    /*! \brief A spot light source. */
    SpotLight = 1,
    /*! \brief A directional light source. */
    DirectionalLight = 2
} LightType; // do not change these values; they are reused in shaders!

/**
 * \brief The Light class.
 *
 * This describes a light source for OpenGL-based rendering.
 */
class Light {
public:
    /**
     * \name Type
     */
    /*@{*/

    /*! \brief Light source type */
    LightType type;
    /*! \brief For spot lights: inner cone angle in degrees */
    float innerConeAngle;
    /*! \brief For spot lights: outer cone angle in degrees */
    float outerConeAngle;

    /*@}*/

    /**
     * \name Geometry
     */
    /*@{*/

    /*! \brief Flag: does this light source move with the camera? */
    bool isRelativeToCamera;
    /*! \brief Position */
    QVector3D position;
    /*! \brief Direction (for spot lights and directional lights) */
    QVector3D direction;
    /*! \brief "Up vector" (for shadow mapping and power factor maps). Must be perpendicular to \a direction. */
    QVector3D up;

    /*@}*/

    /**
     * \name Basic simulation parameters
     */
    /*@{*/

    /*! \brief Light color, for classic OpenGL cameras. */
    QVector3D color;
    /*! \brief Power in Watt, for PMD cameras. */
    float power;
    /*! \brief Constant attenuation coefficient. Should be 1 for physically plausible lights. */
    float attenuationConstant;
    /*! \brief Linear attenuation coefficient. Should be 0 for physically plausible lights. */
    float attenuationLinear;
    /*! \brief Quadratic attenuation coefficient. Should be 1 for physically plausible lights. */
    float attenuationQuadratic;

    /*@}*/

    /**
     * \name Shadows and reflective shadow maps
     */
    /*@{*/

    /*! \brief Flag: does this light source create a shadow map? (Used if shadow maps are active in the simulator pipeline.) */
    bool shadowMap;

    /*! \brief Width and height of shadow map (only used if \a shadowMap is true) */
    unsigned int shadowMapSize;

    /*! \brief Bias for comparison with shadow map depths. Value should be greater than or equal to zero. */
    float shadowMapDepthBias;

    /*! \brief Flag: does this light source create reflective shadow maps? (Used if reflective shadow maps are active in the simulator pipeline.) */
    bool reflectiveShadowMap;

    /*! \brief Width and height of reflective shadow map (only used if \a reflectiveShadowMap is true) */
    unsigned int reflectiveShadowMapSize;

    /*@}*/

    /**
     * \name Advanced simulation parameters
     */
    /*@{*/

    /*! \brief Texture containing power factor map (or 0 if unavailable).
     * This will be created and updated automatically by the simulator; you only
     * have to care about the parameters defined below.
     *
     * Each texel corresponds to an outgoing angle (horizontally and vertically) and
     * contains a floating point value in [0,1] that is multiplied
     * with the light source power value ('color' for classic OpenGL cameras;
     * 'power' for PMD cameras). */
    unsigned int powerFactorTex;
    /*! \brief Width of the power factor map */
    int powerFactorMapWidth;
    /*! \brief Height of the power factor map */
    int powerFactorMapHeight;
    /*! \brief Angle in degrees at left border of power distribution map (seen from light source) */
    float powerFactorMapAngleLeft;
    /*! \brief Angle in degrees at right border of power distribution map (seen from light source) */
    float powerFactorMapAngleRight;
    /*! \brief Angle in degrees at bottom border of power distribution map (seen from light source) */
    float powerFactorMapAngleBottom;
    /*! \brief Angle in degrees at top border of power distribution map (seen from light source) */
    float powerFactorMapAngleTop;
    /*! \brief Vector containing power factor map values (or empty). */
    QVector<float> powerFactors;
    /*! \brief Callback function that allows dynamically changing power factor maps.
     * \param callbackData      User defined pointer, see \a powerFactorMapCallbackData
     * \param timestamp         Time stamp for which to generate the map
     * \param mapWidth          Width of the generated map
     * \param mapHeight         Height of the generated map
     * \param angleLeft         Horizontal angle corresponding to left border of map
     * \param angleRight        Horizontal angle corresponding to right border of map
     * \param angleBottom       Vertical angle corresponding to bottom border of map
     * \param angleTop          Vertical angle corresponding to top border of map
     * \param factors           The map; this vector should be resized and filled with values
     * \return                  Whether any of the values changed compared to the last version */
    bool (*powerFactorMapCallback)(
            void* callbackData,
            long long timestamp,
            int* mapWidth, int* mapHeight,
            float* angleLeft, float* angleRight,
            float* angleBottom, float* angleTop,
            QVector<float>& factors);
    /*! \brief Pointer to user-defined data fed into the power factor map callback */
    void* powerFactorMapCallbackData;

    /*@}*/

    /*! \brief Constructor */
    Light();

    /*! \brief Update a power factor texture from the values given. This function
     * is used by the simulator; do not call it in your own code. */
    void updatePowerFactorTex(unsigned int pbo, long long timestamp);
};

/*!
 * \brief Material type.
 */
typedef enum {
    /*! \brief A BRDF model based on modified Phong lighting */
    Phong = 0,
    /*! \brief TODO: A BRDF model based on a microfacet model */
    Microfacets = 1,
    /*! \brief TODO: A BRDF model based on a measurements of a real material */
    Measured = 2
} MaterialType; // do not change these values, they a reused in shaders

/**
 * \brief The Material class
 *
 * Describes a material suitable for OpenGL-based rendering
 */
class Material {
public:
    /**
     * \name Constructors for basic materials
     */
    /*@{*/

    /*! \brief Default material (phong, greyish, with diffuse and specular components) */
    Material();

    /*! \brief Construct a phong-type material with the given diffuse and specular colors.
     * If the specular color is 0 (the default), then the material will be lambertian. */
    Material(const QVector3D& diffuse, const QVector3D& specular = QVector3D(0.0f, 0.0f, 0.0f), float shininess = 100.0f);

    /*@}*/

    /**
     * \name Type description
     */
    /*@{*/

    /*! \brief Material type */
    MaterialType type;
    /*! \brief Flag: is this material two-sided? */
    bool isTwoSided;

    /*@}*/

    /**
     * \name Simulated geometric detail
     */
    /*@{*/

    /*! \brief Texture containing a bump map, or 0. */
    unsigned int bumpTex;
    /*! \brief Scaling factor for the bump map */
    float bumpScaling;
    /*! \brief Texture containing a normal map (overrides \a bumpTex), or 0 */
    unsigned int normalTex;

    /*@}*/

    /**
     * \name Opacity
     */
    /*@{*/

    /*! \brief Opacity value in [0,1] */
    float opacity;
    /*! \brief Texture containing an opacity map (overrides \a opacity) */
    unsigned int opacityTex;

    /*@}*/

    /**
     * \name Simulation parameters (depend on material type)
     */
    /*@{*/

    /*! \brief Ambient color (for Phong) */
    QVector3D ambient;
    /*! \brief Diffuse color (for Phong) */
    QVector3D diffuse;
    /*! \brief Specular color (for Phong) */
    QVector3D specular;
    /*! \brief Emissive color (for Phong) */
    QVector3D emissive;
    /*! \brief Shininess parameter (for Phong) */
    float shininess;
    /*! \brief Texture containing ambient color map (overrides \a ambient), or 0 (for Phong) */
    unsigned int ambientTex;
    /*! \brief Texture containing diffuse color map (overrides \a diffuse), or 0 (for Phong) */
    unsigned int diffuseTex;
    /*! \brief Texture containing specular color map (overrides \a specular), or 0 (for Phong) */
    unsigned int specularTex;
    /*! \brief Texture containing emissive color map (overrides \a emissive), or 0 (for Phong) */
    unsigned int emissiveTex;
    /*! \brief Texture containing shininess map (overrides \a shininess), or 0 (for Phong) */
    unsigned int shininessTex;
    /*! \brief Texture containing lightness map, or 0 (for Phong) */
    unsigned int lightnessTex;
    /*@}*/
};

/**
 * \brief The Shape class
 *
 * Describes a geometric shape suitable for OpenGL-based rendering
 */
class Shape {
public:
    /*! \brief Index of the material description for this shape; see global \a Scene data */
    unsigned int materialIndex;
    /*! \brief Vertex array object containing the vertex data for this shape */
    unsigned int vao;
    /*! \brief Number of indices to render in GL_TRIANGLES mode */
    unsigned int indices;

    /*! \brief Constructor */
    Shape();
};

/**
 * \brief The Object class
 *
 * Describes an object, consisting of one or more shapes, suitable for OpenGL-based rendering
 */
class Object {
public:
    /*! \brief The list of shapes that form this object */
    QList<Shape> shapes;

    /*! \brief Constructor for an empty object */
    Object();

    /*! \brief Constructor for an object with a single shape */
    Object(const Shape& shape);
};

/**
 * \brief The Scene class
 *
 * Describes a scene to render (light sources and objects), suitable for OpenGL-based rendering
 */
class Scene {
public:
    /**
     * \name Global scene data: materials
     */
    /*@{*/

    /*! \brief Global list of materials used in the scene */
    QList<Material> materials;

    /*! \brief Convenience function to add a material. Returns its index. */
    int addMaterial(const Material& material)
    {
        materials.append(material);
        return materials.size() - 1;
    }

    /*@}*/

    /**
     * \name Scene contents: light sources and objects
     */
    /*@{*/

    /*! \brief Light sources */
    QList<Light> lights;
    /*! \brief Light source animations (list must have same size as \a lights list) */
    QList<Animation> lightAnimations;
    /*! \brief Objects */
    QList<Object> objects;
    /*! \brief Object animations (list must have same size as \a objects list) */
    QList<Animation> objectAnimations;

    /*! \brief Convenience function to add a light source and its animation */
    void addLight(const Light& light, const Animation& animation = Animation())
    {
        lights.append(light);
        lightAnimations.append(animation);
    }

    /*! \brief Convenience function to add an object and its animation */
    void addObject(const Object& object, const Animation& animation = Animation())
    {
        objects.append(object);
        objectAnimations.append(animation);
    }

    /*@}*/

    /*! \brief Constructor */
    Scene();
};

}

#endif

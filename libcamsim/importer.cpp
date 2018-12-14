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

#include "importer.hpp"

#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QMap>

#ifdef HAVE_ASSIMP
# include <assimp/DefaultLogger.hpp>
# include <assimp/Importer.hpp>
# include <assimp/scene.h>
# include <assimp/postprocess.h>
#endif

#include "gl.hpp"


namespace CamSim {

class ImporterInternals
{
public:
#ifdef HAVE_ASSIMP
    Assimp::Importer importer;
    const aiScene* scene;
#else
    int dummy;
#endif
};

Importer::Importer()
{
    _internals = new ImporterInternals;
}

Importer::~Importer()
{
    delete _internals;
}

bool Importer::import(const QString& fileName)
{
#ifdef HAVE_ASSIMP
    qInfo("%s: starting import...", qPrintable(fileName));
    Assimp::DefaultLogger::create("", Assimp::Logger::NORMAL, aiDefaultLogStream_STDERR);
    _internals->importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
            aiComponent_TANGENTS_AND_BITANGENTS | aiComponent_COLORS |
            aiComponent_ANIMATIONS | aiComponent_CAMERAS);
    _internals->scene = _internals->importer.ReadFile(qPrintable(fileName),
            aiProcess_GenSmoothNormals
            | aiProcess_JoinIdenticalVertices
            | aiProcess_ImproveCacheLocality
            | aiProcess_Debone
            | aiProcess_RemoveRedundantMaterials
            | aiProcess_Triangulate
            | aiProcess_GenUVCoords
            | aiProcess_SortByPType
            | aiProcess_FindInvalidData
            | aiProcess_FindInstances
            | aiProcess_ValidateDataStructure
            | aiProcess_OptimizeMeshes
            | aiProcess_RemoveComponent
            | aiProcess_PreTransformVertices
            | aiProcess_TransformUVCoords);
    Assimp::DefaultLogger::kill();
    if (!_internals->scene) {
        qCritical("%s: import failed: %s", qPrintable(fileName), _internals->importer.GetErrorString());
        return false;
    }
    qInfo("%s: import finished successfully", qPrintable(fileName));
    _fileName = fileName;
    return true;
#else
    qCritical("%s: import failed: %s", qPrintable(fileName), "libassimp is missing");
    return false;
#endif
}

void Importer::addLightsToScene(Scene& scene) const
{
    for (int i = 0; i < lightCount(); i++)
        addLightToScene(i, scene);
}

int Importer::lightCount() const
{
#ifdef HAVE_ASSIMP
    return _internals->scene->mNumLights;
#else
    return 0;
#endif
}

void Importer::addLightToScene(int lightSourceIndex, Scene& scene) const
{
#ifdef HAVE_ASSIMP
    Light light;

    const aiLight* l = _internals->scene->mLights[lightSourceIndex];
    switch (l->mType) {
    case aiLightSource_DIRECTIONAL:
        light.type = DirectionalLight;
        break;
    case aiLightSource_SPOT:
        light.type = SpotLight;
        break;
    case aiLightSource_POINT:
    default:
        light.type = PointLight;
        break;
    }
    light.innerConeAngle = l->mAngleInnerCone;
    light.outerConeAngle = l->mAngleOuterCone;
    light.isRelativeToCamera = false;
    light.position = _transformationMatrix
        * QVector3D(l->mPosition[0], l->mPosition[1], l->mPosition[2]);
    light.direction = (_transformationMatrix
            * QVector4D(l->mDirection[0], l->mDirection[1], l->mDirection[2], 0.0f)).toVector3D();
    // We ignore ambient and specular parts of the light color since they cannot be
    // used with BRDF-based lighting.
    light.color = QVector3D(l->mColorDiffuse[0], l->mColorDiffuse[1], l->mColorDiffuse[2]);
    light.attenuationConstant = l->mAttenuationConstant;
    light.attenuationLinear = l->mAttenuationLinear;
    light.attenuationQuadratic = l->mAttenuationQuadratic;

    scene.lights.append(light);
    scene.lightAnimations.append(Animation());
#else
    Q_UNUSED(lightSourceIndex);
    Q_UNUSED(scene);
#endif
}

#ifdef HAVE_ASSIMP
static unsigned int createTex(
        const QString& baseDir,
        QMap<QString, unsigned int>& texturemap,
        const aiMaterial* m, aiTextureType t, unsigned int i, bool scalar)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);

    aiString path;
    aiTextureMapMode mapmode = aiTextureMapMode_Decal;
    m->GetTexture(t, i, &path, NULL, NULL, NULL, NULL, &mapmode);

    QString fileName = baseDir + '/' + path.C_Str();
    fileName.replace('\\', '/');

    unsigned int tex = 0;
    auto it = texturemap.find(fileName);
    if (it != texturemap.end()) {
        tex = it.value();
    } else {
        QImage img;
        if (!img.load(fileName)) {
            qWarning("%s: cannot load texture, ignoring it", qPrintable(fileName));
        } else {
            // Using Qt bindTexture() does not work for some reason, maybe it's
            // because we use a core context. So we do it ourselves.
            img = img.mirrored(false, true);
            img = img.convertToFormat(QImage::Format_RGBA8888);
            gl->glGenTextures(1, &tex);
            gl->glBindTexture(GL_TEXTURE_2D, tex);
            gl->glTexImage2D(GL_TEXTURE_2D, 0, scalar ? GL_R8 : GL_RGBA8,
                    img.width(), img.height(), 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
            gl->glGenerateMipmap(GL_TEXTURE_2D);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            switch (mapmode) {
            case aiTextureMapMode_Wrap:
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                break;
            case aiTextureMapMode_Clamp:
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                break;
            case aiTextureMapMode_Decal:
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                break;
            case aiTextureMapMode_Mirror:
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
                break;
            default:
                break;
            }
        }
        texturemap.insert(fileName, tex);
    }
    return tex;
}
#endif

void Importer::addObjectToScene(Scene& scene) const
{
#ifdef HAVE_ASSIMP
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);

    /* first the materials */
    QString baseDir = QFileInfo(_fileName).dir().path();
    QMap<QString, unsigned int> texturemap;
    scene.materials.reserve(_internals->scene->mNumMaterials);
    for (unsigned int i = 0; i < _internals->scene->mNumMaterials; i++) {
        Material mat;
        int itmp;
        float ftmp;
        aiColor3D ctmp;
        const aiMaterial* m = _internals->scene->mMaterials[i];
        mat.type = Phong;
        if (m->Get(AI_MATKEY_TWOSIDED, itmp) == aiReturn_SUCCESS)
            mat.isTwoSided = itmp;
        if (m->Get(AI_MATKEY_BUMPSCALING, ftmp) == aiReturn_SUCCESS)
            mat.bumpScaling = ftmp;
        if (m->Get(AI_MATKEY_OPACITY, ftmp) == aiReturn_SUCCESS)
            mat.opacity = ftmp;
        if (m->Get(AI_MATKEY_COLOR_AMBIENT, ctmp) == aiReturn_SUCCESS)
            mat.ambient = QVector3D(ctmp.r, ctmp.g, ctmp.b);
        if (m->Get(AI_MATKEY_COLOR_DIFFUSE, ctmp) == aiReturn_SUCCESS)
            mat.diffuse = QVector3D(ctmp.r, ctmp.g, ctmp.b);
        if (m->Get(AI_MATKEY_COLOR_SPECULAR, ctmp) == aiReturn_SUCCESS)
            mat.specular = QVector3D(ctmp.r, ctmp.g, ctmp.b);
        if (m->Get(AI_MATKEY_COLOR_EMISSIVE, ctmp) == aiReturn_SUCCESS)
            mat.emissive = QVector3D(ctmp.r, ctmp.g, ctmp.b);
        if (m->Get(AI_MATKEY_SHININESS, ftmp) == aiReturn_SUCCESS)
            mat.shininess = ftmp;
        if (m->GetTextureCount(aiTextureType_AMBIENT) > 0)
            mat.ambientTex = createTex(baseDir, texturemap, m, aiTextureType_AMBIENT, 0, false);
        if (m->GetTextureCount(aiTextureType_DIFFUSE) > 0)
            mat.diffuseTex = createTex(baseDir, texturemap, m, aiTextureType_DIFFUSE, 0, false);
        if (m->GetTextureCount(aiTextureType_SPECULAR) > 0)
            mat.specularTex = createTex(baseDir, texturemap, m, aiTextureType_SPECULAR, 0, false);
        if (m->GetTextureCount(aiTextureType_EMISSIVE) > 0)
            mat.emissiveTex = createTex(baseDir, texturemap, m, aiTextureType_EMISSIVE, 0, false);
        if (m->GetTextureCount(aiTextureType_SHININESS) > 0)
            mat.shininessTex = createTex(baseDir, texturemap, m, aiTextureType_SHININESS, 0, true);
        if (m->GetTextureCount(aiTextureType_LIGHTMAP) > 0)
            mat.lightnessTex = createTex(baseDir, texturemap, m, aiTextureType_LIGHTMAP, 0, true);
        if (m->GetTextureCount(aiTextureType_HEIGHT) > 0)
            mat.bumpTex = createTex(baseDir, texturemap, m, aiTextureType_HEIGHT, 0, true);
        if (m->GetTextureCount(aiTextureType_NORMALS) > 0)
            mat.normalTex = createTex(baseDir, texturemap, m, aiTextureType_NORMALS, 0, false);
        if (m->GetTextureCount(aiTextureType_OPACITY) > 0)
            mat.opacityTex = createTex(baseDir, texturemap, m, aiTextureType_OPACITY, 0, true);
        scene.materials.append(mat);
    }

    /* then the shapes */
    QMatrix4x4 normalMatrix = QMatrix4x4(_transformationMatrix.normalMatrix());
    std::vector<float> data;
    Object object;
    object.shapes.reserve(_internals->scene->mNumMeshes);
    for (unsigned int i = 0; i < _internals->scene->mNumMeshes; i++) {
        Shape shape;
        const aiMesh* m = _internals->scene->mMeshes[i];
        if (m->mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
            qWarning("%s: ignoring shape %u because it is not triangle-based", qPrintable(_fileName), i);
            continue;
        }
        if (m->mNumVertices > std::numeric_limits<unsigned int>::max()) {
            qWarning("%s: ignoring shape %u because it has more vertices than I can handle", qPrintable(_fileName), i);
            continue;
        }
        shape.materialIndex = m->mMaterialIndex;
        gl->glGenVertexArrays(1, &(shape.vao));
        gl->glBindVertexArray(shape.vao);
        GLuint positionBuffer;
        gl->glGenBuffers(1, &positionBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        data.resize(m->mNumVertices * 3);
        for (unsigned int j = 0; j < m->mNumVertices; j++) {
            QVector3D v = QVector3D(m->mVertices[j].x, m->mVertices[j].y, m->mVertices[j].z);
            QVector3D w = _transformationMatrix * v;
            data[3 * j + 0] = w.x();
            data[3 * j + 1] = w.y();
            data[3 * j + 2] = w.z();
        }
        gl->glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), data.data(), GL_STATIC_DRAW);
        gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        gl->glEnableVertexAttribArray(0);
        GLuint normalBuffer;
        gl->glGenBuffers(1, &normalBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
        data.resize(m->mNumVertices * 3);
        for (unsigned int j = 0; j < m->mNumVertices; j++) {
            QVector3D v = QVector3D(m->mNormals[j].x, m->mNormals[j].y, m->mNormals[j].z);
            QVector3D w = normalMatrix * v;
            data[3 * j + 0] = w.x();
            data[3 * j + 1] = w.y();
            data[3 * j + 2] = w.z();
        }
        gl->glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 3 * sizeof(float), data.data(), GL_STATIC_DRAW);
        gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        gl->glEnableVertexAttribArray(1);
        GLuint texcoordBuffer;
        gl->glGenBuffers(1, &texcoordBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
        data.resize(m->mNumVertices * 2, 0.0f);
        if (m->mTextureCoords[0]) {
            for (unsigned int j = 0; j < m->mNumVertices; j++) {
                data[2 * j + 0] = m->mTextureCoords[0][j][0];
                data[2 * j + 1] = m->mTextureCoords[0][j][1];
            }
        }
        gl->glBufferData(GL_ARRAY_BUFFER, m->mNumVertices * 2 * sizeof(float), data.data(), GL_STATIC_DRAW);
        gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
        gl->glEnableVertexAttribArray(2);
        GLuint index_buffer;
        gl->glGenBuffers(1, &index_buffer);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
        std::vector<unsigned int> indices(m->mNumFaces * 3);
        for (unsigned int j = 0; j < m->mNumFaces; j++) {
            indices[3 * j + 0] = m->mFaces[j].mIndices[0];
            indices[3 * j + 1] = m->mFaces[j].mIndices[1];
            indices[3 * j + 2] = m->mFaces[j].mIndices[2];
        }
        gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        shape.indices = m->mNumFaces * 3;
        object.shapes.append(shape);
    }
    scene.objects.append(object);
    scene.objectAnimations.append(Animation());
#else
    Q_UNUSED(scene);
#endif
}

unsigned int Importer::importTexture(const QString& fileName)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);

    unsigned int tex = 0;
    QImage img;
    if (img.load(fileName)) {
        // Using Qt bindTexture() does not work for some reason, maybe it's
        // because we use a core context. So we do it ourselves.
        img = img.mirrored(false, true);
        img = img.convertToFormat(QImage::Format_RGBA8888);
        gl->glGenTextures(1, &tex);
        gl->glBindTexture(GL_TEXTURE_2D, tex);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                img.width(), img.height(), 0,
                GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());
        gl->glGenerateMipmap(GL_TEXTURE_2D);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    return tex;
}

}

/*
 * Copyright (C) 2012, 2013, 2014, 2016, 2017, 2018
 * Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>,
 * Hendrik Sommerhoff <hendrik.sommerhoff@student.uni-siegen.de>
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
#include <random>
#include <utility>

#include <QtMath>
#include <QString>
#include <QFile>
#include <QOffscreenSurface>
#include <QOpenGLContext>

#include "simulator.hpp"
#include "gl.hpp"


namespace CamSim {

ChipTiming::ChipTiming() :
    exposureTime(1.0 / 60.0),
    readoutTime(1.0 / 60.0),
    pauseTime(0.0)
{
}

ChipTiming ChipTiming::fromSubFramesPerSecond(float sfps)
{
    ChipTiming t;
    t.exposureTime = 0.0;
    t.readoutTime = 1.0 / sfps;
    t.pauseTime = 0.0;
    return t;
}

PMD::PMD() :
    pixelSize(12.0 * 12.0),
    pixelContrast(0.75),
    modulationFrequency(10e6),
    wavelength(880.0),
    quantumEfficiency(0.8),
    maxElectrons(100000)
{
}

Projection::Projection()
{
    _w = 640;
    _h = 480;
    _t = std::tan(qDegreesToRadians(70.0f) / 2.0f);
    _b = -_t;
    _r = _t * _w / _h;
    _l = -_r;
    _k1 = 0.0f;
    _k2 = 0.0f;
    _p1 = 0.0f;
    _p2 = 0.0f;
}

QMatrix4x4 Projection::projectionMatrix(float n, float f) const
{
    QMatrix4x4 m;
    m.frustum(_l * n, _r * n, _b * n, _t * n, n, f);
    return m;
}

QSize Projection::imageSize() const
{
    return { _w, _h };
}

QVector2D Projection::centerPixel() const
{
    return QVector2D(
        _r / (_r - _l) * _w - 0.5f,
        _t / (_t - _b) * _h - 0.5f);
}

QVector2D Projection::focalLengths() const
{
    return QVector2D(
        1.0f / ((_r - _l) / _w),
        1.0f / ((_t - _b) / _h));
}

Projection Projection::fromFrustum(int imageWidth, int imageHeight, float l, float r, float b, float t)
{
    Projection p;
    p._w = imageWidth;
    p._h = imageHeight;
    p._l = l;
    p._r = r;
    p._b = b;
    p._t = t;
    return p;
}

Projection Projection::fromOpeningAngle(int imageWidth, int imageHeight, float fovyDegrees)
{
    float t = std::tan(qDegreesToRadians(fovyDegrees / 2.0f));
    float b = -t;
    float r = t * imageWidth / imageHeight;
    float l = -r;
    return fromFrustum(imageWidth, imageHeight, l, r, b, t);
}

Projection Projection::fromIntrinsics(int imageWidth, int imageHeight, float centerX, float centerY, float focalLengthX, float focalLengthY)
{
    float r_minus_l = imageWidth / focalLengthX;
    float l = -(centerX + 0.5f) * r_minus_l / imageWidth;
    float r = r_minus_l + l;
    float t_minus_b = imageHeight / focalLengthY;
    float b = -(centerY + 0.5f) * t_minus_b / imageHeight;
    float t = t_minus_b + b;
    return fromFrustum(imageWidth, imageHeight, l, r, b, t);
}

void Projection::setDistortion(float k1, float k2, float p1, float p2)
{
    _k1 = k1;
    _k2 = k2;
    _p1 = p1;
    _p2 = p2;
}

void Projection::distortion(float* k1, float* k2, float* p1, float* p2)
{
    *k1 = _k1;
    *k2 = _k2;
    *p1 = _p1;
    *p2 = _p2;
}

Pipeline::Pipeline() :
    nearClippingPlane(0.1f),
    farClippingPlane(100.0f),
    mipmapping(true),
    anisotropicFiltering(true),
    transparency(false),
    normalMapping(true),
    ambientLight(false),
    thinLensVignetting(false),
    thinLensApertureDiameter(8.89f), // 0.889 cm aperture (so that F-number is 1.8)
    thinLensFocalLength(16.0f),
    shotNoise(false),
    gaussianWhiteNoise(false),
    gaussianWhiteNoiseMean(0.0f),
    gaussianWhiteNoiseStddev(0.05f),
    preprocLensDistortion(false),
    preprocLensDistortionMargin(0.0f),
    postprocLensDistortion(false),
    shadowMaps(false),
    shadowMapFiltering(true),
    reflectiveShadowMaps(false),
    lightPowerFactorMaps(false),
    subFrameTemporalSampling(true),
    spatialSamples(1, 1),
    temporalSamples(1)
{
}

Output::Output() :
    rgb(true),
    srgb(false),
    pmd(false),
    pmdCoordinates(false),
    eyeSpacePositions(false),
    customSpacePositions(false),
    eyeSpaceNormals(false),
    customSpaceNormals(false),
    depthAndRange(false),
    indices(false),
    forwardFlow3D(false),
    forwardFlow2D(false),
    backwardFlow3D(false),
    backwardFlow2D(false)
{
}

static void glDebugMessageCallback(
        GLenum /* source */,
        GLenum type,
        GLuint /* id */,
        GLenum severity,
        GLsizei /* length */,
        const GLchar* message,
        const void* /* userParam */)
{
    qDebug("GL%s: type=0x%x severity=0x%x: %s",
            (type == GL_DEBUG_TYPE_ERROR ? " ERROR" : ""),
            type, severity, message);
}


Context::Context(bool enableOpenGLDebugging)
{
    _surface = new QOffscreenSurface;
    _surface->create();
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 5);
    if (enableOpenGLDebugging)
        format.setOption(QSurfaceFormat::DebugContext);
    _context = new QOpenGLContext;
    _context->setFormat(format);
    _context->create();
    if (!_context->isValid()) {
        qFatal("Cannot create a valid OpenGL context");
    }
    if (_context->format().majorVersion() < 4
            || (_context->format().majorVersion() == 4 && _context->format().minorVersion() < 5)) {
        qFatal("Cannot create OpenGL context of version >= 4.5");
    }
    makeCurrent();
    if (enableOpenGLDebugging) {
        if (!_context->format().testOption(QSurfaceFormat::DebugContext)) {
            qWarning("OpenGL debugging not available");
        }
        auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
        gl->glEnable(GL_DEBUG_OUTPUT);
        gl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        gl->glDebugMessageCallback(glDebugMessageCallback, 0);
    }
}

void Context::makeCurrent()
{
    if (!_context->makeCurrent(_surface)) {
        qFatal("Cannot make OpenGL context current");
    }
}

Simulator::Simulator() :
    _recreateTimestamps(true),
    _recreateShaders(true),
    _recreateOutput(true),
    _pbo(0),
    _depthBufferOversampled(0),
    _rgbTexOversampled(0),
    _pmdEnergyTexOversampled(0),
    _pmdEnergyTex(0),
    _pmdCoordinatesTex(0),
    _postProcessingTex(0),
    _fbo(0),
    _fullScreenQuadVao(0)
{
}

void Simulator::setCameraAnimation(const Animation& animation)
{
    _cameraAnimation = animation;
    _recreateTimestamps = true;
}

void Simulator::setCameraTransformation(const Transformation& transformation)
{
    _cameraTransformation = transformation;
}

void Simulator::setScene(const Scene& scene)
{
    _scene = scene;
    _recreateShaders = true;    // number of light sources might have changed
    _recreateTimestamps = true; // animations changed
    _recreateOutput = true;     // number of light sources and objects changed
}

void Simulator::setChipTiming(const ChipTiming& chipTiming)
{
    _chipTiming = chipTiming;
    _recreateTimestamps = true;
}

void Simulator::setPMD(const PMD& pmd)
{
    _pmd = pmd;
}

void Simulator::setProjection(const Projection& projection)
{
    _projection = projection;
    _recreateOutput = true;
}

void Simulator::setPipeline(const Pipeline& pipeline)
{
    _pipeline = pipeline;
    _recreateShaders = true;
    _recreateOutput = true;
}

void Simulator::setOutput(const Output& output)
{
    _output = output;
    _recreateShaders = true;
    _recreateOutput = true;
}

void Simulator::setCustomTransformation(const Transformation& transformation)
{
    _customTransformation = transformation;
}

int Simulator::subFrames() const
{
    return (_output.pmd ? 4 : 1);
}

void Simulator::recreateTimestampsIfNecessary()
{
    if (!_recreateTimestamps)
        return;

    _startTimestamp = _cameraAnimation.startTime();
    _endTimestamp = _cameraAnimation.endTime();
    for (int i = 0; i < _scene.lightAnimations.size(); i++) {
        long long s = _scene.lightAnimations[i].startTime();
        if (s < _startTimestamp)
            _startTimestamp = s;
        long long e = _scene.lightAnimations[i].endTime();
        if (e > _endTimestamp)
            _endTimestamp = e;
    }
    for (int i = 0; i < _scene.objectAnimations.size(); i++) {
        long long s = _scene.objectAnimations[i].startTime();
        if (s < _startTimestamp)
            _startTimestamp = s;
        long long e = _scene.objectAnimations[i].endTime();
        if (e > _endTimestamp)
            _endTimestamp = e;
    }
    _haveLastFrameTimestamp = false;
    _recreateTimestamps = false;
}

long long Simulator::startTimestamp()
{
    recreateTimestampsIfNecessary();
    return _startTimestamp;
}

long long Simulator::endTimestamp()
{
    recreateTimestampsIfNecessary();
    return _endTimestamp;
}

long long Simulator::subFrameDuration()
{
    return _chipTiming.subFrameDuration() * 1e6;
}

long long Simulator::frameDuration()
{
    return subFrameDuration() * subFrames() + _chipTiming.pauseTime * 1e6;
}

float Simulator::framesPerSecond()
{
    return 1e6f / frameDuration();
}

long long Simulator::nextFrameTimestamp()
{
    recreateTimestampsIfNecessary();
    if (_haveLastFrameTimestamp) {
        return _lastFrameTimestamp + frameDuration();
    } else {
        return startTimestamp();
    }
}

static QString readFile(const QString& filename)
{
    QFile f(filename);
    f.open(QIODevice::ReadOnly);
    QTextStream in(&f);
    return in.readAll();
}

void Simulator::recreateShadersIfNecessary()
{
    if (!_recreateShaders)
        return;

    // Sanity checks
    if (_scene.lights.size() < 1) {
        qCritical("No lights in scene");
        std::exit(1);
    }
    if (_scene.lights.size() != _scene.lightAnimations.size()) {
        qCritical("Invalid number of light animations in scene");
        std::exit(1);
    }
    if (_scene.objects.size() != _scene.objectAnimations.size()) {
        qCritical("Invalid number of object animations in scene");
        std::exit(1);
    }
    if (_pipeline.spatialSamples.width() < 1 || _pipeline.spatialSamples.width() % 2 != 1
            || _pipeline.spatialSamples.height() < 1 || _pipeline.spatialSamples.height() % 2 != 1) {
        qCritical("Invalid number of spatial samples in pipeline configuration");
        std::exit(1);
    }
    if (_pipeline.spatialSampleWeights.size() > 0
            && _pipeline.spatialSampleWeights.size()
            != _pipeline.spatialSamples.width() * _pipeline.spatialSamples.height()) {
        qCritical("Invalid number of spatial sample weights in pipeline configuration");
        std::exit(1);
    }
    if (_pipeline.temporalSamples < 1) {
        qCritical("Invalid number of temporal samples in pipeline configuration");
        std::exit(1);
    }
    if (_pipeline.preprocLensDistortion && _pipeline.postprocLensDistortion) {
        qCritical("Cannot enable both preproc and postproc lens distortion");
        std::exit(1);
    }
    if (_pipeline.postprocLensDistortion
            && (_output.indices || _output.forwardFlow3D || _output.forwardFlow2D
                || _output.backwardFlow3D || _output.backwardFlow2D)) {
        qCritical("Postproc lens distortion cannot be applied to indices, flow3d, flow2D outputs");
        std::exit(1);
    }

    // Clear the existing programs
    _shadowMapPrg.removeAllShaders();
    _reflectiveShadowMapPrg.removeAllShaders();
    _depthPrg.removeAllShaders();
    _lightPrg.removeAllShaders();
    _lightOversampledPrg.removeAllShaders();
    _pmdDigNumPrg.removeAllShaders();
    _rgbResultPrg.removeAllShaders();
    _pmdResultPrg.removeAllShaders();
    _pmdCoordinatesPrg.removeAllShaders();
    _geomPrg.removeAllShaders();
    _flowPrg.removeAllShaders();
    _convertToSRGBPrg.removeAllShaders();
    _postprocLensDistortionPrg.removeAllShaders();

    // Create programs as necessary. The relevant ones are all derived from
    // the following übershaders. Unnecessary input and output statements are
    // deactivated via preprocessor directives. The GLSL compiler is expected
    // to eleminate the resulting unused code (e.g. all light computations
    // when only geometry information is written).
    QString simVs = readFile(":/libcamsim/simulation-everything-vs.glsl");
    QString simFs = readFile(":/libcamsim/simulation-everything-fs.glsl");

    // Shadow map programs
    if (_pipeline.shadowMaps || _pipeline.reflectiveShadowMaps) {
        QString baseShadowMapVs = simVs;
        QString baseShadowMapFs = simFs;
        baseShadowMapVs.replace("$PREPROC_LENS_DISTORTION$", "0");
        baseShadowMapFs.replace("$PREPROC_LENS_DISTORTION$", "0");
        baseShadowMapFs.replace("$LIGHT_SOURCES$", "1");
        baseShadowMapFs.replace("$OUTPUT_RGB$", "0");
        baseShadowMapFs.replace("$OUTPUT_PMD$", "0");
        baseShadowMapFs.replace("$OUTPUT_DEPTH_AND_RANGE$", "0");
        baseShadowMapFs.replace("$OUTPUT_INDICES$", "0");
        baseShadowMapFs.replace("$OUTPUT_FORWARDFLOW3D$", "0");
        baseShadowMapFs.replace("$OUTPUT_FORWARDFLOW2D$", "0");
        baseShadowMapFs.replace("$OUTPUT_BACKWARDFLOW3D$", "0");
        baseShadowMapFs.replace("$OUTPUT_BACKWARDFLOW2D$", "0");
        baseShadowMapFs.replace("$OUTPUT_BACKWARDVISIBILITY$", "0");
        baseShadowMapFs.replace("$TRANSPARENCY$", _pipeline.transparency ? "1" : "0");
        baseShadowMapFs.replace("$NORMALMAPPING$", "0");
        baseShadowMapFs.replace("$SHADOW_MAPS$", "0");
        baseShadowMapFs.replace("$REFLECTIVE_SHADOW_MAPS$", "0");
        baseShadowMapFs.replace("$POWER_FACTOR_MAPS$", powerTexs() ? "1" : "0");
        if (_pipeline.shadowMaps) {
            QString shadowMapFs = baseShadowMapFs;
            shadowMapFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "1");
            shadowMapFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", "0");
            shadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", "0");
            shadowMapFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", "0");
            shadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", "0");
            shadowMapFs.replace("$OUTPUT_RADIANCES$", "0");
            shadowMapFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "0");
            shadowMapFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "0");
            _shadowMapPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, baseShadowMapVs);
            _shadowMapPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, shadowMapFs);
            if (!_shadowMapPrg.link()) {
                qCritical("Cannot link shadow map program");
                std::exit(1);
            }
        }
        if (_pipeline.reflectiveShadowMaps) {
            QString reflectiveShadowMapFs = baseShadowMapFs;
            reflectiveShadowMapFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "0");
            reflectiveShadowMapFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", "0");
            reflectiveShadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", "0");
            reflectiveShadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_RADIANCES$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS_LOCATION$", "0");
            reflectiveShadowMapFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS_LOCATION$", "1");
            reflectiveShadowMapFs.replace("$OUTPUT_RADIANCES_LOCATION$", "2");
            reflectiveShadowMapFs.replace("$OUTPUT_BRDF_DIFF_PARAMS_LOCATION$", "3");
            reflectiveShadowMapFs.replace("$OUTPUT_BRDF_SPEC_PARAMS_LOCATION$", "4");
            _reflectiveShadowMapPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, baseShadowMapVs);
            _reflectiveShadowMapPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, reflectiveShadowMapFs);
            if (!_reflectiveShadowMapPrg.link()) {
                qCritical("Cannot link reflective shadow map program");
                std::exit(1);
            }
        }
    }

    // Prerender depth only
    QString depthVs = simVs;
    QString depthFs = simFs;
    depthVs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
    depthFs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
    depthFs.replace("$LIGHT_SOURCES$", "1");
    depthFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "0");
    depthFs.replace("$OUTPUT_RGB$", "0");
    depthFs.replace("$OUTPUT_PMD$", "0");
    depthFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", "0");
    depthFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", "0");
    depthFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", "0");
    depthFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", "0");
    depthFs.replace("$OUTPUT_DEPTH_AND_RANGE$", "0");
    depthFs.replace("$OUTPUT_INDICES$", "0");
    depthFs.replace("$OUTPUT_FORWARDFLOW3D$", "0");
    depthFs.replace("$OUTPUT_FORWARDFLOW2D$", "0");
    depthFs.replace("$OUTPUT_BACKWARDFLOW3D$", "0");
    depthFs.replace("$OUTPUT_BACKWARDFLOW2D$", "0");
    depthFs.replace("$OUTPUT_BACKWARDVISIBILITY$", "0");
    depthFs.replace("$OUTPUT_RADIANCES$", "0");
    depthFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "0");
    depthFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "0");
    depthFs.replace("$TRANSPARENCY$", _pipeline.transparency ? "1" : "0");
    depthFs.replace("$NORMALMAPPING$", "0");
    depthFs.replace("$SHADOW_MAPS$", "0");
    depthFs.replace("$REFLECTIVE_SHADOW_MAPS$", "0");
    depthFs.replace("$POWER_FACTOR_MAPS$", "0");
    _depthPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, depthVs);
    _depthPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, depthFs);
    if (!_depthPrg.link()) {
        qCritical("Cannot link depth simulation program");
        std::exit(1);
    }

    // Light-based simulation programs
    if (_output.rgb || _output.pmd) {
        QVector<int> tmpInt(_scene.lights.size());
        QVector<float> tmpVec3(3 * _scene.lights.size());
        QVector<float> tmpFloat(_scene.lights.size());
        QString lightVs = simVs;
        QString lightFs = simFs;
        lightVs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        lightFs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        lightFs.replace("$LIGHT_SOURCES$", QString::number(_scene.lights.size()));
        lightFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "0");
        lightFs.replace("$OUTPUT_RGB$", _output.rgb ? "1" : "0");
        lightFs.replace("$GAUSSIAN_WHITE_NOISE$", _pipeline.gaussianWhiteNoise ? "1" : "0");
        lightFs.replace("$OUTPUT_PMD$", _output.pmd ? "1" : "0");
        lightFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", "0");
        lightFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", "0");
        lightFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", "0");
        lightFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", "0");
        lightFs.replace("$OUTPUT_DEPTH_AND_RANGE$", "0");
        lightFs.replace("$OUTPUT_INDICES$", "0");
        lightFs.replace("$OUTPUT_FORWARDFLOW3D$", "0");
        lightFs.replace("$OUTPUT_FORWARDFLOW2D$", "0");
        lightFs.replace("$OUTPUT_BACKWARDFLOW3D$", "0");
        lightFs.replace("$OUTPUT_BACKWARDFLOW2D$", "0");
        lightFs.replace("$OUTPUT_BACKWARDVISIBILITY$", "0");
        lightFs.replace("$OUTPUT_RADIANCES$", "0");
        lightFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "0");
        lightFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "0");
        lightFs.replace("$OUTPUT_RGB_LOCATION$", "0");
        lightFs.replace("$OUTPUT_PMD_LOCATION$", _output.rgb ? "1" : "0");
        lightFs.replace("$TRANSPARENCY$", _pipeline.transparency ? "1" : "0");
        lightFs.replace("$NORMALMAPPING$", _pipeline.normalMapping ? "1" : "0");
        lightFs.replace("$SHADOW_MAPS$", _pipeline.shadowMaps ? "1" : "0");
        lightFs.replace("$SHADOW_MAP_FILTERING$", _pipeline.shadowMapFiltering ? "1" : "0");
        lightFs.replace("$REFLECTIVE_SHADOW_MAPS$", _pipeline.reflectiveShadowMaps ? "1" : "0");
        lightFs.replace("$POWER_FACTOR_MAPS$", powerTexs() ? "1" : "0");
        _lightPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, lightVs);
        _lightPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, lightFs);
        if (!_lightPrg.link()) {
            qCritical("Cannot link light simulation program");
            std::exit(1);
        }
        _lightPrg.bind();
        // set static light source properties
        for (int i = 0; i < _scene.lights.size(); i++)
            tmpInt[i] = _scene.lights[i].type;
        _lightPrg.setUniformValueArray("light_type", tmpInt.constData(), _scene.lights.size());
        for (int i = 0; i < _scene.lights.size(); i++)
            tmpFloat[i] = qDegreesToRadians(_scene.lights[i].innerConeAngle);
        _lightPrg.setUniformValueArray("light_inner_cone_angle", tmpFloat.constData(), _scene.lights.size(), 1);
        for (int i = 0; i < _scene.lights.size(); i++)
            tmpFloat[i] = qDegreesToRadians(_scene.lights[i].outerConeAngle);
        _lightPrg.setUniformValueArray("light_outer_cone_angle", tmpFloat.constData(), _scene.lights.size(), 1);
        for (int i = 0; i < _scene.lights.size(); i++) {
            tmpVec3[3 * i + 0] = _scene.lights[i].attenuationConstant;
            tmpVec3[3 * i + 1] = _scene.lights[i].attenuationLinear;
            tmpVec3[3 * i + 2] = _scene.lights[i].attenuationQuadratic;
        }
        _lightPrg.setUniformValueArray("light_attenuation", tmpVec3.constData(), _scene.lights.size(), 3);
        for (int i = 0; i < _scene.lights.size(); i++)
            for (int j = 0; j < 3; j++)
                tmpVec3[3 * i + j] = _scene.lights[i].color[j];
        _lightPrg.setUniformValueArray("light_color", tmpVec3.constData(), _scene.lights.size(), 3);
        if (_output.pmd) {
            for (int i = 0; i < _scene.lights.size(); i++)
                tmpFloat[i] = lightIntensity(i);
            _lightPrg.setUniformValueArray("light_intensity", tmpFloat.constData(), _scene.lights.size(), 1);
        }
        // additional simple programs for oversampling and subframe combination
        QString lightOversampledVs = readFile(":/libcamsim/simulation-oversampling-vs.glsl");
        QString lightOversampledFs = readFile(":/libcamsim/simulation-oversampling-fs.glsl");
        lightOversampledFs.replace("$TWO_INPUTS$", (_output.rgb && _output.pmd ? "1" : "0"));
        lightOversampledFs.replace("$WEIGHTS_WIDTH$", QString::number(_pipeline.spatialSamples.width()));
        lightOversampledFs.replace("$WEIGHTS_HEIGHT$", QString::number(_pipeline.spatialSamples.height()));
        _lightOversampledPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, lightOversampledVs);
        _lightOversampledPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, lightOversampledFs);
        _lightOversampledPrg.link();
        _lightOversampledPrg.bind();
        _lightOversampledPrg.setUniformValue("oversampled0", 0);
        _lightOversampledPrg.setUniformValue("oversampled1", 1);
        int weightCount = _pipeline.spatialSamples.width() * _pipeline.spatialSamples.height();
        QVector<float> defaultWeights(weightCount, 1.0f);
        _lightOversampledPrg.setUniformValueArray("weights",
                _pipeline.spatialSampleWeights.size() > 0
                ? _pipeline.spatialSampleWeights.constData()
                : defaultWeights.constData(),
                weightCount, 1);
        if (_output.pmd) {
            QString pmdDigNumVs = readFile(":/libcamsim/simulation-pmd-dignums-vs.glsl");
            QString pmdDigNumFs = readFile(":/libcamsim/simulation-pmd-dignums-fs.glsl");
            pmdDigNumFs.replace("$SHOT_NOISE$", _pipeline.shotNoise ? "1" : "0");
            _pmdDigNumPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, pmdDigNumVs);
            _pmdDigNumPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, pmdDigNumFs);
            _pmdDigNumPrg.link();
            _pmdDigNumPrg.bind();
            _pmdDigNumPrg.setUniformValue("pmd_energies", 0);
        }
        if (subFrames() > 1) {
            if (_output.rgb) {
                QString rgbResultVs = readFile(":/libcamsim/simulation-rgb-result-vs.glsl");
                QString rgbResultFs = readFile(":/libcamsim/simulation-rgb-result-fs.glsl");
                rgbResultFs.replace("$SUBFRAMES$", QString::number(subFrames()));
                _rgbResultPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, rgbResultVs);
                _rgbResultPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, rgbResultFs);
                _rgbResultPrg.link();
                _rgbResultPrg.bind();
                QVector<int> samplers(subFrames());
                for (int i = 0; i < samplers.size(); i++)
                    samplers[i] = i;
                _rgbResultPrg.setUniformValueArray("texs", samplers.constData(), samplers.size());
            }
            if (_output.pmd) {
                QString pmdResultVs = readFile(":/libcamsim/simulation-pmd-result-vs.glsl");
                QString pmdResultFs = readFile(":/libcamsim/simulation-pmd-result-fs.glsl");
                _pmdResultPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, pmdResultVs);
                _pmdResultPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, pmdResultFs);
                _pmdResultPrg.link();
                _pmdResultPrg.bind();
                QVector<int> samplers(subFrames());
                for (int i = 0; i < samplers.size(); i++)
                    samplers[i] = i;
                _pmdResultPrg.setUniformValueArray("phase_texs", samplers.constData(), samplers.size());
            }
        }
        // conversion from linear RGB to sRGB
        if (_output.srgb) {
            QString convVs = readFile(":/libcamsim/convert-to-srgb-vs.glsl");
            QString convFs = readFile(":/libcamsim/convert-to-srgb-fs.glsl");
            _convertToSRGBPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, convVs);
            _convertToSRGBPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, convFs);
            _convertToSRGBPrg.link();
        }
        // conversion from PMD range to coordinates
        if (_output.pmdCoordinates) {
            QString pmdCoordinatesVs = readFile(":/libcamsim/simulation-pmd-coords-vs.glsl");
            QString pmdCoordinatesFs = readFile(":/libcamsim/simulation-pmd-coords-fs.glsl");
            _pmdCoordinatesPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, pmdCoordinatesVs);
            _pmdCoordinatesPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, pmdCoordinatesFs);
            _pmdCoordinatesPrg.link();
            _pmdCoordinatesPrg.bind();
        }
    }

    // Geometry simulation program
    if (_output.eyeSpacePositions || _output.customSpacePositions
            || _output.eyeSpaceNormals || _output.customSpaceNormals
            || _output.depthAndRange || _output.indices) {
        QString geomVs = simVs;
        QString geomFs = simFs;
        geomVs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        geomFs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        geomFs.replace("$LIGHT_SOURCES$", "1");
        geomFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "0");
        geomFs.replace("$OUTPUT_RGB$", "0");
        geomFs.replace("$OUTPUT_PMD$", "0");
        geomFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", QString::number(_output.eyeSpacePositions ? 1 : 0));
        geomFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", QString::number(_output.customSpacePositions ? 1 : 0));
        geomFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", QString::number(_output.eyeSpaceNormals ? 1 : 0));
        geomFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", QString::number(_output.customSpaceNormals ? 1 : 0));
        geomFs.replace("$OUTPUT_DEPTH_AND_RANGE$", QString::number(_output.depthAndRange ? 1 : 0));
        geomFs.replace("$OUTPUT_INDICES$", QString::number(_output.indices ? 1 : 0));
        geomFs.replace("$OUTPUT_FORWARDFLOW3D$", "0");
        geomFs.replace("$OUTPUT_FORWARDFLOW2D$", "0");
        geomFs.replace("$OUTPUT_BACKWARDFLOW3D$", "0");
        geomFs.replace("$OUTPUT_BACKWARDFLOW2D$", "0");
        geomFs.replace("$OUTPUT_BACKWARDVISIBILITY$", "0");
        geomFs.replace("$OUTPUT_RADIANCES$", "0");
        geomFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "0");
        geomFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "0");
        geomFs.replace("$TRANSPARENCY$", _pipeline.transparency ? "1" : "0");
        geomFs.replace("$NORMALMAPPING$", _pipeline.normalMapping ? "1" : "0");
        geomFs.replace("$SHADOW_MAPS$", "0");
        geomFs.replace("$REFLECTIVE_SHADOW_MAPS$", "0");
        geomFs.replace("$POWER_FACTOR_MAPS$", "0");
        int outputIndex = 0;
        if (_output.eyeSpacePositions)
            geomFs.replace("$OUTPUT_EYE_SPACE_POSITIONS_LOCATION$", QString::number(outputIndex++));
        if (_output.customSpacePositions)
            geomFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS_LOCATION$", QString::number(outputIndex++));
        if (_output.eyeSpaceNormals)
            geomFs.replace("$OUTPUT_EYE_SPACE_NORMALS_LOCATION$", QString::number(outputIndex++));
        if (_output.customSpaceNormals)
            geomFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS_LOCATION$", QString::number(outputIndex++));
        if (_output.depthAndRange)
            geomFs.replace("$OUTPUT_DEPTH_AND_RANGE_LOCATION$", QString::number(outputIndex++));
        if (_output.indices)
            geomFs.replace("$OUTPUT_INDICES_LOCATION$", QString::number(outputIndex++));
        _geomPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, geomVs);
        _geomPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, geomFs);
        if (!_geomPrg.link()) {
            qCritical("Cannot link geometry simulation program");
            std::exit(1);
        }
    }

    // Flow simulation program
    if (_output.forwardFlow3D || _output.forwardFlow2D || _output.backwardFlow3D || _output.backwardFlow2D) {
        QString flowVs = simVs;
        QString flowFs = simFs;
        flowVs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        flowFs.replace("$PREPROC_LENS_DISTORTION$", _pipeline.preprocLensDistortion ? "1" : "0");
        flowFs.replace("$LIGHT_SOURCES$", "1");
        flowFs.replace("$OUTPUT_SHADOW_MAP_DEPTH$", "0");
        flowFs.replace("$OUTPUT_RGB$", "0");
        flowFs.replace("$OUTPUT_PMD$", "0");
        flowFs.replace("$OUTPUT_EYE_SPACE_POSITIONS$", "0");
        flowFs.replace("$OUTPUT_CUSTOM_SPACE_POSITIONS$", "0");
        flowFs.replace("$OUTPUT_EYE_SPACE_NORMALS$", "0");
        flowFs.replace("$OUTPUT_CUSTOM_SPACE_NORMALS$", "0");
        flowFs.replace("$OUTPUT_DEPTH_AND_RANGE$", "0");
        flowFs.replace("$OUTPUT_INDICES$", "0");
        flowFs.replace("$OUTPUT_FORWARDFLOW3D$", QString::number(_output.forwardFlow3D ? 1 : 0));
        flowFs.replace("$OUTPUT_FORWARDFLOW2D$", QString::number(_output.forwardFlow2D ? 1 : 0));
        flowFs.replace("$OUTPUT_BACKWARDFLOW3D$", QString::number(_output.backwardFlow3D ? 1 : 0));
        flowFs.replace("$OUTPUT_BACKWARDFLOW2D$", QString::number(_output.backwardFlow2D ? 1 : 0));
        flowFs.replace("$OUTPUT_RADIANCES$", "0");
        flowFs.replace("$OUTPUT_BRDF_DIFF_PARAMS$", "0");
        flowFs.replace("$OUTPUT_BRDF_SPEC_PARAMS$", "0");
        flowFs.replace("$TRANSPARENCY$", _pipeline.transparency ? "1" : "0");
        flowFs.replace("$NORMALMAPPING$", _pipeline.normalMapping ? "1" : "0");
        flowFs.replace("$SHADOW_MAPS$", "0");
        flowFs.replace("$REFLECTIVE_SHADOW_MAPS$", "0");
        flowFs.replace("$POWER_FACTOR_MAPS$", "0");
        int outputIndex = 0;
        if (_output.forwardFlow3D)
            flowFs.replace("$OUTPUT_FORWARDFLOW3D_LOCATION$", QString::number(outputIndex++));
        if (_output.forwardFlow2D)
            flowFs.replace("$OUTPUT_FORWARDFLOW2D_LOCATION$", QString::number(outputIndex++));
        if (_output.backwardFlow3D)
            flowFs.replace("$OUTPUT_BACKWARDFLOW3D_LOCATION$", QString::number(outputIndex++));
        if (_output.backwardFlow2D)
            flowFs.replace("$OUTPUT_BACKWARDFLOW2D_LOCATION$", QString::number(outputIndex++));
        _flowPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, flowVs);
        _flowPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, flowFs);
        if (!_flowPrg.link()) {
            qCritical("Cannot link flow simulation program");
            std::exit(1);
        }
    }

    // Postproc lens distortion program
    if (_pipeline.postprocLensDistortion) {
        QString distortionVS = readFile(":/libcamsim/simulation-postproc-lensdistortion-vs.glsl");
        QString distortionFS = readFile(":/libcamsim/simulation-postproc-lensdistortion-fs.glsl");
        _postprocLensDistortionPrg.addShaderFromSourceCode(QOpenGLShader::Vertex, distortionVS);
        _postprocLensDistortionPrg.addShaderFromSourceCode(QOpenGLShader::Fragment, distortionFS);
        if (!_postprocLensDistortionPrg.link()) {
            qCritical("Cannot link postproc lens distortion program");
            std::exit(1);
        }
    }

    _haveLastFrameTimestamp = false;
    _recreateShaders = false;
}

void Simulator::prepareDepthBuffers(QSize size, const QVector<unsigned int>& depthBufs)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    for (int i = 0; i < depthBufs.size(); i++) {
        gl->glBindTexture(GL_TEXTURE_2D, depthBufs[i]);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size.width(), size.height(),
                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    }
    ASSERT_GLCHECK();
}

void Simulator::prepareOutputTexs(QSize size, const QVector<unsigned int>& outputTexs, int internalFormat, bool interpolation)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    bool isIntegerFormat = (internalFormat == GL_R32UI || internalFormat == GL_RG32UI
            || internalFormat == GL_RGB32UI || internalFormat == GL_RGBA32UI);
    GLenum format = isIntegerFormat ? GL_RGBA_INTEGER : GL_RGBA;
    GLenum type = isIntegerFormat ? GL_UNSIGNED_INT : GL_FLOAT;
    for (int i = 0; i < outputTexs.size(); i++) {
        gl->glBindTexture(GL_TEXTURE_2D, outputTexs[i]);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation ? GL_LINEAR : GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation ? GL_LINEAR : GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, size.width(), size.height(),
                0, format, type, NULL);
    }
    ASSERT_GLCHECK();
}

bool Simulator::spatialOversampling() const
{
    return (_pipeline.spatialSamples.width() > 1 || _pipeline.spatialSamples.height() > 1);
}

QSize Simulator::spatialOversamplingSize() const
{
    return QSize(
            _projection.imageSize().width() * _pipeline.spatialSamples.width(),
            _projection.imageSize().height() * _pipeline.spatialSamples.height());
}

bool Simulator::temporalOversampling() const
{
    return (_pipeline.temporalSamples > 1);
}

bool Simulator::powerTexs() const
{
    for (int i = 0; i < _scene.lights.size(); i++)
        if (_scene.lights[i].powerFactorTex != 0
                || _scene.lights[i].powerFactors.size() > 0
                || _scene.lights[i].powerFactorMapCallback)
            return true;
    return false;
}

void Simulator::recreateOutputIfNecessary()
{
    if (!_recreateOutput)
        return;

    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();

    // Delete all existing output
    _timestamps.clear();
    _cameraTransformations.clear();
    _lightTransformations.clear();
    _objectTransformations.clear();
    for (int i = 0; i < _shadowMapDepthBufs.size(); i++)
        gl->glDeleteTextures(_shadowMapDepthBufs[i].size(), _shadowMapDepthBufs[i].constData());
    _shadowMapDepthBufs.clear();
    for (int i = 0; i < _reflectiveShadowMapDepthBufs.size(); i++)
        gl->glDeleteTextures(_reflectiveShadowMapDepthBufs[i].size(), _reflectiveShadowMapDepthBufs[i].constData());
    _reflectiveShadowMapDepthBufs.clear();
    for (int i = 0; i < _reflectiveShadowMapTexs.size(); i++)
        gl->glDeleteTextures(_reflectiveShadowMapTexs[i].size(), _reflectiveShadowMapTexs[i].constData());
    _reflectiveShadowMapTexs.clear();
    gl->glDeleteBuffers(1, &_pbo);
    _pbo = 0;
    gl->glDeleteTextures(1, &_depthBufferOversampled);
    _depthBufferOversampled = 0;
    gl->glDeleteTextures(1, &_rgbTexOversampled);
    _rgbTexOversampled = 0;
    gl->glDeleteTextures(1, &_pmdEnergyTexOversampled);
    _pmdEnergyTexOversampled = 0;
    gl->glDeleteTextures(1, &_pmdEnergyTex);
    _pmdEnergyTex = 0;
    gl->glDeleteTextures(1, &_pmdCoordinatesTex);
    _pmdCoordinatesTex = 0;
    gl->glDeleteTextures(_depthBuffers.size(), _depthBuffers.constData());
    _depthBuffers.clear();
    gl->glDeleteTextures(_rgbTexs.size(), _rgbTexs.constData());
    _rgbTexs.clear();
    gl->glDeleteTextures(_srgbTexs.size(), _srgbTexs.constData());
    _srgbTexs.clear();
    gl->glDeleteTextures(_pmdDigNumTexs.size(), _pmdDigNumTexs.constData());
    _pmdDigNumTexs.clear();
    gl->glDeleteTextures(_eyeSpacePosTexs.size(), _eyeSpacePosTexs.constData());
    _eyeSpacePosTexs.clear();
    gl->glDeleteTextures(_customSpacePosTexs.size(), _customSpacePosTexs.constData());
    _customSpacePosTexs.clear();
    gl->glDeleteTextures(_eyeSpaceNormalTexs.size(), _eyeSpaceNormalTexs.constData());
    _eyeSpaceNormalTexs.clear();
    gl->glDeleteTextures(_customSpaceNormalTexs.size(), _customSpaceNormalTexs.constData());
    _customSpaceNormalTexs.clear();
    gl->glDeleteTextures(_depthAndRangeTexs.size(), _depthAndRangeTexs.constData());
    _depthAndRangeTexs.clear();
    gl->glDeleteTextures(_indicesTexs.size(), _indicesTexs.constData());
    _indicesTexs.clear();
    gl->glDeleteTextures(_forwardFlow3DTexs.size(), _forwardFlow3DTexs.constData());
    _forwardFlow3DTexs.clear();
    gl->glDeleteTextures(_forwardFlow2DTexs.size(), _forwardFlow2DTexs.constData());
    _forwardFlow2DTexs.clear();
    gl->glDeleteTextures(_backwardFlow3DTexs.size(), _backwardFlow3DTexs.constData());
    _backwardFlow3DTexs.clear();
    gl->glDeleteTextures(_backwardFlow2DTexs.size(), _backwardFlow2DTexs.constData());
    _backwardFlow2DTexs.clear();
    _lightSimOutputTexs.clear();
    _geomSimOutputTexs.clear();
    _flowSimOutputTexs.clear();
    _oversampledLightSimOutputTexs.clear();
    gl->glDeleteTextures(1, &_postProcessingTex);
    _postProcessingTex = 0;

    // Create new output as needed
    _timestamps.resize(subFrames());
    _cameraTransformations.resize(subFrames());
    _lightTransformations.resize(subFrames());
    for (int i = 0; i < _lightTransformations.size(); i++)
        _lightTransformations[i].resize(_scene.lights.size());
    _objectTransformations.resize(subFrames());
    for (int i = 0; i < _objectTransformations.size(); i++)
        _objectTransformations[i].resize(_scene.objects.size());
    if (_pipeline.shadowMaps) {
        gl->glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        _shadowMapDepthBufs.resize(subFrames());
        for (int subFrame = 0; subFrame < subFrames(); subFrame++) {
            _shadowMapDepthBufs[subFrame].resize(_scene.lights.size());
            for (int light = 0; light < _scene.lights.size(); light++) {
                _shadowMapDepthBufs[subFrame][light] = 0;
                if (_scene.lights[light].shadowMap) {
                    gl->glGenTextures(1, &(_shadowMapDepthBufs[subFrame][light]));
                    gl->glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMapDepthBufs[subFrame][light]);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, _pipeline.shadowMapFiltering ? GL_NEAREST : GL_LINEAR);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, _pipeline.shadowMapFiltering ? GL_NEAREST : GL_LINEAR);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                    for (int cubeSide = 0; cubeSide < 6; cubeSide++) {
                        gl->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeSide, 0,
                                GL_DEPTH_COMPONENT32F,
                                _scene.lights[light].shadowMapSize,
                                _scene.lights[light].shadowMapSize,
                                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
                    }
                }
            }
        }
    }
    if (_pipeline.reflectiveShadowMaps) {
        gl->glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        _reflectiveShadowMapDepthBufs.resize(subFrames());
        _reflectiveShadowMapTexs.resize(subFrames());
        for (int subFrame = 0; subFrame < subFrames(); subFrame++) {
            _reflectiveShadowMapDepthBufs[subFrame].resize(_scene.lights.size());
            _reflectiveShadowMapTexs[subFrame].resize(_scene.lights.size());
            for (int light = 0; light < _scene.lights.size(); light++) {
                _reflectiveShadowMapDepthBufs[subFrame][light] = 0;
                _reflectiveShadowMapTexs[subFrame][light] = 0;
                if (_scene.lights[light].reflectiveShadowMap) {
                    gl->glGenTextures(1, &(_reflectiveShadowMapDepthBufs[subFrame][light]));
                    gl->glBindTexture(GL_TEXTURE_CUBE_MAP, _reflectiveShadowMapDepthBufs[subFrame][light]);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                    for (int cubeSide = 0; cubeSide < 6; cubeSide++) {
                        gl->glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeSide, 0,
                                GL_DEPTH_COMPONENT24,
                                _scene.lights[light].reflectiveShadowMapSize,
                                _scene.lights[light].reflectiveShadowMapSize,
                                0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
                    }
                    gl->glGenTextures(1, &(_reflectiveShadowMapTexs[subFrame][light]));
                    gl->glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, _reflectiveShadowMapTexs[subFrame][light]);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                    gl->glTexParameteri(GL_TEXTURE_CUBE_MAP_ARRAY, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
                    gl->glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA32F,
                            _scene.lights[light].reflectiveShadowMapSize,
                            _scene.lights[light].reflectiveShadowMapSize, 6 * 5,
                            0, GL_RGBA, GL_FLOAT, NULL);
                }
            }
        }
    }
    ASSERT_GLCHECK();
    gl->glGenBuffers(1, &_pbo);
    if (_output.rgb || _output.pmd) {
        gl->glGenTextures(1, &_depthBufferOversampled);
        prepareDepthBuffers(spatialOversamplingSize(), { _depthBufferOversampled });
    }
    if (_output.rgb) {
        gl->glGenTextures(1, &_rgbTexOversampled);
        prepareOutputTexs(spatialOversamplingSize(), { _rgbTexOversampled }, GL_RGBA32F, false);
    }
    if (_output.pmd) {
        gl->glGenTextures(1, &_pmdEnergyTexOversampled);
        prepareOutputTexs(spatialOversamplingSize(), { _pmdEnergyTexOversampled }, GL_RG32F, false);
        gl->glGenTextures(1, &_pmdEnergyTex);
        prepareOutputTexs(_projection.imageSize(), { _pmdEnergyTex }, GL_RG32F, false);
        if (_output.pmdCoordinates) {
            gl->glGenTextures(1, &_pmdCoordinatesTex);
            prepareOutputTexs(_projection.imageSize(), { _pmdCoordinatesTex }, GL_RGBA32F, false);
        }
    }
    int extra = (subFrames() > 1 ? 1 : 0);
    _depthBuffers.resize(subFrames() + 1);
    gl->glGenTextures(subFrames() + 1, _depthBuffers.data());
    prepareDepthBuffers(_projection.imageSize(), _depthBuffers);
    bool interpolation = (_pipeline.postprocLensDistortion);
    if (_output.rgb) {
        _rgbTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _rgbTexs.data());
        prepareOutputTexs(_projection.imageSize(), _rgbTexs, GL_RGBA32F, interpolation);
        if (_output.srgb) {
            _srgbTexs.resize(subFrames() + extra);
            gl->glGenTextures(subFrames() + extra, _srgbTexs.data());
            prepareOutputTexs(_projection.imageSize(), _srgbTexs, GL_RGBA8, false);
        }
    }
    if (_output.pmd) {
        _pmdDigNumTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _pmdDigNumTexs.data());
        prepareOutputTexs(_projection.imageSize(), _pmdDigNumTexs, GL_RGBA32F, interpolation);
    }
    if (_output.eyeSpacePositions) {
        _eyeSpacePosTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _eyeSpacePosTexs.data());
        prepareOutputTexs(_projection.imageSize(), _eyeSpacePosTexs, GL_RGBA32F, interpolation);
    }
    if (_output.customSpacePositions) {
        _customSpacePosTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _customSpacePosTexs.data());
        prepareOutputTexs(_projection.imageSize(), _customSpacePosTexs, GL_RGBA32F, interpolation);
    }
    if (_output.eyeSpaceNormals) {
        _eyeSpaceNormalTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _eyeSpaceNormalTexs.data());
        prepareOutputTexs(_projection.imageSize(), _eyeSpaceNormalTexs, GL_RGBA32F, interpolation);
    }
    if (_output.customSpaceNormals) {
        _customSpaceNormalTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _customSpaceNormalTexs.data());
        prepareOutputTexs(_projection.imageSize(), _customSpaceNormalTexs, GL_RGBA32F, interpolation);
    }
    if (_output.depthAndRange) {
        _depthAndRangeTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _depthAndRangeTexs.data());
        prepareOutputTexs(_projection.imageSize(), _depthAndRangeTexs,
                _pipeline.postprocLensDistortion ? GL_RGBA32F : GL_RG32F, interpolation);
        // the reason for using 4 instead of 2 components if lens distortion is active
        // is that only then the internal formats of this texture and _postProcessingTex
        // are compatible for glCopySubImageData().
    }
    if (_output.indices) {
        _indicesTexs.resize(subFrames());
        gl->glGenTextures(subFrames(), _indicesTexs.data());
        prepareOutputTexs(_projection.imageSize(), _indicesTexs, GL_RGBA32UI, false);
    }
    if (_output.forwardFlow3D) {
        _forwardFlow3DTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _forwardFlow3DTexs.data());
        prepareOutputTexs(_projection.imageSize(), _forwardFlow3DTexs, GL_RGBA32F, false);
    }
    if (_output.forwardFlow2D) {
        _forwardFlow2DTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _forwardFlow2DTexs.data());
        prepareOutputTexs(_projection.imageSize(), _forwardFlow2DTexs, GL_RG32F, false);
    }
    if (_output.backwardFlow3D) {
        _backwardFlow3DTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _backwardFlow3DTexs.data());
        prepareOutputTexs(_projection.imageSize(), _backwardFlow3DTexs, GL_RGBA32F, false);
    }
    if (_output.backwardFlow2D) {
        _backwardFlow2DTexs.resize(subFrames() + extra);
        gl->glGenTextures(subFrames() + extra, _backwardFlow2DTexs.data());
        prepareOutputTexs(_projection.imageSize(), _backwardFlow2DTexs, GL_RG32F, false);
    }
    if (_pipeline.postprocLensDistortion) {
        gl->glGenTextures(1, &_postProcessingTex);
        prepareOutputTexs(_projection.imageSize(), { _postProcessingTex }, GL_RGBA32F, false);
        gl->glBindTexture(GL_TEXTURE_2D, _postProcessingTex);
    }

    // Prepare output texture lists for light simulation, flow simulation, and geometry simulation
    _lightSimOutputTexs.resize(subFrames());
    _geomSimOutputTexs.resize(subFrames());
    _flowSimOutputTexs.resize(subFrames() + extra);
    if (_output.rgb)
        for (int i = 0; i < _lightSimOutputTexs.size(); i++)
            _lightSimOutputTexs[i].append(_rgbTexs[i]);
    if (_output.pmd)
        for (int i = 0; i < _lightSimOutputTexs.size(); i++)
            _lightSimOutputTexs[i].append(_pmdEnergyTex);
    if (_output.eyeSpacePositions)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_eyeSpacePosTexs[i]);
    if (_output.customSpacePositions)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_customSpacePosTexs[i]);
    if (_output.eyeSpaceNormals)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_eyeSpaceNormalTexs[i]);
    if (_output.customSpaceNormals)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_customSpaceNormalTexs[i]);
    if (_output.depthAndRange)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_depthAndRangeTexs[i]);
    if (_output.indices)
        for (int i = 0; i < _geomSimOutputTexs.size(); i++)
            _geomSimOutputTexs[i].append(_indicesTexs[i]);
    if (_output.forwardFlow3D)
        for (int i = 0; i < _flowSimOutputTexs.size(); i++)
            _flowSimOutputTexs[i].append(_forwardFlow3DTexs[i]);
    if (_output.forwardFlow2D)
        for (int i = 0; i < _flowSimOutputTexs.size(); i++)
            _flowSimOutputTexs[i].append(_forwardFlow2DTexs[i]);
    if (_output.backwardFlow3D)
        for (int i = 0; i < _flowSimOutputTexs.size(); i++)
            _flowSimOutputTexs[i].append(_backwardFlow3DTexs[i]);
    if (_output.backwardFlow2D)
        for (int i = 0; i < _flowSimOutputTexs.size(); i++)
            _flowSimOutputTexs[i].append(_backwardFlow2DTexs[i]);
    if (_output.rgb)
        _oversampledLightSimOutputTexs.append(_rgbTexOversampled);
    if (_output.pmd)
        _oversampledLightSimOutputTexs.append(_pmdEnergyTexOversampled);

    _haveLastFrameTimestamp = false;
    _recreateOutput = false;
    ASSERT_GLCHECK();
}

float Simulator::lightIntensity(int lightSourceIndex) const
{
    const Light& light = _scene.lights[lightSourceIndex];
    float intensity = light.power * 1e3f;
    if (light.type == SpotLight) {
        float apertureAngle = qDegreesToRadians(light.outerConeAngle);
        float solidAngle = 2.0f * static_cast<float>(M_PI) * (1.0f - std::cos(apertureAngle / 2.0f));
        intensity /= solidAngle;
    } else {
        intensity /= 4.0f * static_cast<float>(M_PI);
    }
    return intensity;
}

void Simulator::simulateTimestamps(long long t)
{
    for (int subFrame = 0; subFrame < subFrames(); subFrame++) {
        if (_pipeline.subFrameTemporalSampling) {
            _timestamps[subFrame] = t + subFrame * subFrameDuration();
        } else {
            _timestamps[subFrame] = t;
        }
        simulateSampleTimestamp(_timestamps[subFrame],
                _cameraTransformations[subFrame],
                _lightTransformations[subFrame],
                _objectTransformations[subFrame]);
    }
}

void Simulator::simulateSampleTimestamp(long long t,
        Transformation& cameraTransformation,
        QVector<Transformation>& lightTransformations,
        QVector<Transformation>& objectTransformations)
{
    cameraTransformation = _cameraAnimation.interpolate(t);
    for (int i = 0; i < _scene.lights.size(); i++) {
        lightTransformations[i] = _scene.lightAnimations[i].interpolate(t);
    }
    for (int i = 0; i < _scene.objects.size(); i++) {
        objectTransformations[i] = _scene.objectAnimations[i].interpolate(t);
    }
}

void Simulator::prepareFBO(QSize size,
        unsigned int depthBuf, bool reuseDepthBufData,
        const QList<unsigned int>& colorAttachments,
        int cubeMapSide, int arrayTextureLayers,
        bool enableBlending, bool clearBlendingColorBuffer)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();

    if (_fbo == 0) {
        gl->glGenFramebuffers(1, &_fbo);
    }
    gl->glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    ASSERT_GLCHECK();

    // Set new color attachments and select them with drawBuffers
    int texTarget;
    if (cubeMapSide >= 0)
        texTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + cubeMapSide;
    else
        texTarget = GL_TEXTURE_2D;
    gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texTarget, depthBuf, 0);
    QVector<GLenum> drawBuffers;
    if (arrayTextureLayers >= 1) {
        Q_ASSERT(colorAttachments.size() == 1); // currently we handle only one array texture as output
        drawBuffers.resize(arrayTextureLayers);
        for (int i = 0; i < 8; i++) {
            if (i < arrayTextureLayers) {
                int layer = i;
                if (cubeMapSide >= 0)
                    layer = 6 * i + cubeMapSide;
                gl->glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorAttachments[0], 0, layer);
                drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
            } else {
                gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
            }
        }
    } else if (arrayTextureLayers == 0) {
        drawBuffers.resize(colorAttachments.size());
        for (int i = 0; i < 8; i++) {
            if (i < colorAttachments.size()) {
                gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, texTarget, colorAttachments[i], 0);
                drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
            } else {
                gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
            }
        }
    } else { // arrayTextureLayers < 0 means we want layered rendering into the array texture
        Q_ASSERT(colorAttachments.size() == 1); // currently we handle only one array texture for layered output
        drawBuffers.resize(1);
        drawBuffers[0] = GL_COLOR_ATTACHMENT0;
        gl->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorAttachments[0], 0);
    }
    gl->glDrawBuffers(drawBuffers.size(), drawBuffers.constData());
    ASSERT_GLCHECK();

    // Set remaining parameters
    gl->glViewport(0, 0, size.width(), size.height());
    gl->glEnable(GL_DEPTH_TEST);
    if (reuseDepthBufData) {
        gl->glDepthMask(GL_FALSE);
        gl->glDepthFunc(GL_LEQUAL);
    } else {
        gl->glClear(GL_DEPTH_BUFFER_BIT);
        gl->glDepthMask(GL_TRUE);
        gl->glDepthFunc(GL_LESS);
    }
    ASSERT_GLCHECK();

    // Set up additive blending if requested
    if (enableBlending) {
        gl->glBlendEquation(GL_FUNC_ADD);
        gl->glBlendFunc(GL_ONE, GL_ONE);
        gl->glEnable(GL_BLEND);
        if (clearBlendingColorBuffer)
            gl->glClear(GL_COLOR_BUFFER_BIT);
    } else {
        gl->glDisable(GL_BLEND);
        gl->glClear(GL_COLOR_BUFFER_BIT);
    }
    ASSERT_GLCHECK();

    // Create the fullscreen quad vao if it does not exist yet
    if (_fullScreenQuadVao == 0) {
        QVector3D quadPositions[] = {
            QVector3D(-1.0f, -1.0f, 0.0f), QVector3D(+1.0f, -1.0f, 0.0f),
            QVector3D(+1.0f, +1.0f, 0.0f), QVector3D(-1.0f, +1.0f, 0.0f)
        };
        QVector2D quadTexcoords[] = {
            QVector2D(0.0f, 0.0f), QVector2D(1.0f, 0.0f),
            QVector2D(1.0f, 1.0f), QVector2D(0.0f, 1.0f)
        };
        unsigned int quadIndices[] = {
            0, 1, 3, 1, 2, 3
        };
        gl->glGenVertexArrays(1, &_fullScreenQuadVao);
        gl->glBindVertexArray(_fullScreenQuadVao);
        GLuint positionBuffer;
        gl->glGenBuffers(1, &positionBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, positionBuffer);
        gl->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector3D), quadPositions, GL_STATIC_DRAW);
        gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        gl->glEnableVertexAttribArray(0);
        GLuint texcoordBuffer;
        gl->glGenBuffers(1, &texcoordBuffer);
        gl->glBindBuffer(GL_ARRAY_BUFFER, texcoordBuffer);
        gl->glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QVector2D), quadTexcoords, GL_STATIC_DRAW);
        gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        gl->glEnableVertexAttribArray(1);
        GLuint indexBuffer;
        gl->glGenBuffers(1, &indexBuffer);
        gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), quadIndices, GL_STATIC_DRAW);
        gl->glBindVertexArray(0);
    }
    ASSERT_GLCHECK();
}

void Simulator::drawScene(QOpenGLShaderProgram& prg,
        const QMatrix4x4& projectionMatrix,
        const QMatrix4x4& viewMatrix,
        const QMatrix4x4& lastViewMatrix,
        const QMatrix4x4& nextViewMatrix,
        const QVector<Transformation>& objectTransformations,
        const QVector<Transformation>& lastObjectTransformations,
        const QVector<Transformation>& nextObjectTransformations)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();

    for (int i = 0; i < _scene.objects.size(); i++) {
        prg.setUniformValue("object_index", i);
        // Set dynamic object uniforms (i.e. matrices)
        QMatrix4x4 invertedViewMatrix = viewMatrix.inverted();
        QMatrix4x4 modelMatrix = objectTransformations[i].toMatrix4x4();
        QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
        QMatrix3x3 normalMatrix = modelViewMatrix.normalMatrix();
        QMatrix4x4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
        prg.setUniformValue("projection_matrix", projectionMatrix);
        prg.setUniformValue("modelview_matrix", modelViewMatrix);
        prg.setUniformValue("modelview_projection_matrix", modelViewProjectionMatrix);
        prg.setUniformValue("normal_matrix", normalMatrix);
        QMatrix4x4 lastModelMatrix = lastObjectTransformations[i].toMatrix4x4();
        QMatrix4x4 lastModelViewMatrix = lastViewMatrix * lastModelMatrix;
        QMatrix4x4 lastModelViewProjectionMatrix = projectionMatrix * lastModelViewMatrix;
        prg.setUniformValue("last_modelview_matrix", lastModelViewMatrix);
        prg.setUniformValue("last_modelview_projection_matrix", lastModelViewProjectionMatrix);
        QMatrix4x4 nextModelMatrix = nextObjectTransformations[i].toMatrix4x4();
        QMatrix4x4 nextModelViewMatrix = nextViewMatrix * nextModelMatrix;
        QMatrix4x4 nextModelViewProjectionMatrix = projectionMatrix * nextModelViewMatrix;
        prg.setUniformValue("next_modelview_matrix", nextModelViewMatrix);
        prg.setUniformValue("next_modelview_projection_matrix", nextModelViewProjectionMatrix);
        QMatrix4x4 customMatrix = _customTransformation.toMatrix4x4() * invertedViewMatrix;
        QMatrix3x3 customNormalMatrix = customMatrix.normalMatrix();
        prg.setUniformValue("custom_matrix", customMatrix);
        prg.setUniformValue("custom_normal_matrix", customNormalMatrix);
        if (_pipeline.shadowMaps) {
            prg.setUniformValue("inverted_view_matrix", invertedViewMatrix.toGenericMatrix<3, 3>());
        }
        for (int j = 0; j < _scene.objects[i].shapes.size(); j++) {
            prg.setUniformValue("shape_index", j);
            // Set dynamic shape uniforms (i.e. material)
            const Shape& shape = _scene.objects[i].shapes[j];
            const Material& material = _scene.materials[shape.materialIndex];
            if (material.isTwoSided)
                gl->glDisable(GL_CULL_FACE);
            else
                gl->glEnable(GL_CULL_FACE);
            prg.setUniformValue("material_index", shape.materialIndex);
            prg.setUniformValue("material_type", static_cast<int>(material.type));
            prg.setUniformValue("material_ambient", _pipeline.ambientLight ? material.ambient : QVector3D(0.0f, 0.0f, 0.0f));
            prg.setUniformValue("material_diffuse", material.diffuse);
            prg.setUniformValue("material_specular", material.specular);
            prg.setUniformValue("material_emissive", material.emissive);
            prg.setUniformValue("material_shininess", material.shininess);
            prg.setUniformValue("material_opacity", material.opacity);
            prg.setUniformValue("material_bumpscaling", material.bumpScaling);
            prg.setUniformValue("material_have_ambient_tex", _pipeline.ambientLight && material.ambientTex > 0 ? 1 : 0);
            prg.setUniformValue("material_ambient_tex", 0);
            prg.setUniformValue("material_have_diffuse_tex", material.diffuseTex > 0 ? 1 : 0);
            prg.setUniformValue("material_diffuse_tex", 1);
            prg.setUniformValue("material_have_specular_tex", material.specularTex > 0 ? 1 : 0);
            prg.setUniformValue("material_specular_tex", 2);
            prg.setUniformValue("material_have_emissive_tex", material.emissiveTex > 0 ? 1 : 0);
            prg.setUniformValue("material_emissive_tex", 3);
            prg.setUniformValue("material_have_shininess_tex", material.shininessTex > 0 ? 1 : 0);
            prg.setUniformValue("material_shininess_tex", 4);
            prg.setUniformValue("material_have_lightness_tex", material.lightnessTex > 0 ? 1 : 0);
            prg.setUniformValue("material_lightness_tex", 5);
            prg.setUniformValue("material_have_opacity_tex", material.opacityTex > 0 ? 1 : 0);
            prg.setUniformValue("material_opacity_tex", 6);
            prg.setUniformValue("material_have_bump_tex", material.bumpTex > 0 ? 1 : 0);
            prg.setUniformValue("material_bump_tex", 7);
            prg.setUniformValue("material_have_normal_tex", material.normalTex > 0 ? 1 : 0);
            prg.setUniformValue("material_normal_tex", 8);
            // Bind textures
            unsigned int textures[9] = { material.ambientTex, material.diffuseTex, material.specularTex,
                material.emissiveTex, material.shininessTex, material.lightnessTex, material.opacityTex,
                material.bumpTex, material.normalTex };
            for (int i = 0; i < 9; i++) {
                gl->glActiveTexture(GL_TEXTURE0 + i);
                gl->glBindTexture(GL_TEXTURE_2D, textures[i]);
                gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        _pipeline.mipmapping ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
                gl->glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                        _pipeline.anisotropicFiltering ? 4.0f : 1.0f);
            }
            // Draw shape
            gl->glBindVertexArray(shape.vao);
            gl->glDrawElements(GL_TRIANGLES, shape.indices, GL_UNSIGNED_INT, 0);
        }
    }
    ASSERT_GLCHECK();
}

static QVector2D undistortPoint(QVector2D point,
        float k1, float k2, float p1, float p2,
        QVector2D focalLengths, QVector2D centerPixel, QSize imageSize)
{
    QVector2D viewPortCoord = QVector2D(
            point.x() * 0.5f + 0.5f,
            0.5f - point.y() * 0.5f);
    QVector2D pixelCoord = QVector2D(
            viewPortCoord.x() * imageSize.width(),
            viewPortCoord.y() * imageSize.height());
    float x = (pixelCoord.x() - centerPixel.x()) / focalLengths.x();
    float y = (pixelCoord.y() - centerPixel.y()) / focalLengths.y();
    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float invFactor = 1.0 / (4.0 * k1 * r2 + 6.0 * k2 * r4 + 8.0 * p1 * y + 8.0 * p2 * x + 1.0);
    float dx = x * (k1 * r2 + k2 * r4) + (2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x));
    float dy = y * (k1 * r2 + k2 * r4) + (p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y);
    x -= invFactor * dx;
    y -= invFactor * dy;
    x = x * focalLengths.x() + centerPixel.x();
    y = y * focalLengths.y() + centerPixel.y();
    viewPortCoord = QVector2D(x / (float)imageSize.width(), y / (float)imageSize.height());
    float ndcX = viewPortCoord.x() * 2.0f - 1.0f;
    float ndcY = 1.0f - viewPortCoord.y() * 2.0f;
    return QVector2D(ndcX, ndcY);
}

void Simulator::simulate(
        QOpenGLShaderProgram& prg, int subFrame,
        long long t, long long lastT, long long nextT, unsigned int lastDepthBuf,
        const Transformation& cameraTransformation,
        const QVector<Transformation>& lightTransformations,
        const QVector<Transformation>& objectTransformations)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();

    prg.bind();

    // Basic matrices
    QMatrix4x4 cameraMatrix = _cameraTransformation.toMatrix4x4() * cameraTransformation.toMatrix4x4();
    QMatrix4x4 viewMatrix = cameraMatrix.inverted();
    QMatrix4x4 projectionMatrix = _projection.projectionMatrix(_pipeline.nearClippingPlane, _pipeline.farClippingPlane);
    QMatrix4x4 lastViewMatrix = viewMatrix;
    QVector<Transformation> lastObjectTransformations = objectTransformations;
    QMatrix4x4 nextViewMatrix = viewMatrix;
    QVector<Transformation> nextObjectTransformations = objectTransformations;
    if (lastT != t) {
        Transformation lastCameraTransformation;
        QVector<Transformation> lastLightTransformations = lightTransformations;;
        simulateSampleTimestamp(lastT, lastCameraTransformation, lastLightTransformations, lastObjectTransformations);
        QMatrix4x4 lastCameraMatrix = _cameraTransformation.toMatrix4x4() * lastCameraTransformation.toMatrix4x4();
        lastViewMatrix = lastCameraMatrix.inverted();
    }
    if (nextT != t) {
        Transformation nextCameraTransformation;
        QVector<Transformation> nextLightTransformations = lightTransformations;;
        simulateSampleTimestamp(nextT, nextCameraTransformation, nextLightTransformations, nextObjectTransformations);
        QMatrix4x4 nextCameraMatrix = _cameraTransformation.toMatrix4x4() * nextCameraTransformation.toMatrix4x4();
        nextViewMatrix = nextCameraMatrix.inverted();
    }

    // Set static parameters that do not trigger recreateShaders
    prg.setUniformValue("viewport_width", _projection.imageSize().width());
    prg.setUniformValue("viewport_height", _projection.imageSize().height());
    prg.setUniformValue("far_plane", _pipeline.farClippingPlane);
    gl->glActiveTexture(GL_TEXTURE9);
    gl->glBindTexture(GL_TEXTURE_2D, lastDepthBuf);
    prg.setUniformValue("last_depth_buf", 9);
    if (_pipeline.thinLensVignetting) {
        prg.setUniformValue("thin_lens_vignetting", 1);
        prg.setUniformValue("frac_apdiam_foclen", _pipeline.thinLensApertureDiameter / _pipeline.thinLensFocalLength);
    } else {
        prg.setUniformValue("thin_lens_vignetting", 0);
    }
    prg.setUniformValue("temporal_samples", _pipeline.temporalSamples);
    prg.setUniformValue("exposure_time", static_cast<float>(_chipTiming.exposureTime * 1e6));
    prg.setUniformValue("pixel_area_factor", 1.0f / (_pipeline.spatialSamples.width() * _pipeline.spatialSamples.height()));
    if (_output.pmd) {
        prg.setUniformValue("pixel_area", static_cast<float>(_pmd.pixelSize));
        prg.setUniformValue("frac_modfreq_c", static_cast<float>(_pmd.modulationFrequency / speedOfLight));
        prg.setUniformValue("contrast", static_cast<float>(_pmd.pixelContrast));
        prg.setUniformValue("tau", subFrame * static_cast<float>(M_PI_2));
    }
    if (_pipeline.preprocLensDistortion) {
        float k1, k2, p1, p2;
        _projection.distortion(&k1, &k2, &p1, &p2);
        QVector2D focalLengths = _projection.focalLengths();
        QVector2D centerPixel = _projection.centerPixel();
        QSize imageSize = _projection.imageSize();
        // Undistort all cube edges and find max x and y components
        QVector2D undistortedCubeCorner[4];
        undistortedCubeCorner[0] = undistortPoint(QVector2D(1.0, 1.0),
                k1, k2, p1, p2, focalLengths, centerPixel, imageSize);
        undistortedCubeCorner[1] = undistortPoint(QVector2D(1.0, -1.0),
                k1, k2, p1, p2, focalLengths, centerPixel, imageSize);
        undistortedCubeCorner[2] = undistortPoint(QVector2D(-1.0, 1.0),
                k1, k2, p1, p2, focalLengths, centerPixel, imageSize);
        undistortedCubeCorner[3] = undistortPoint(QVector2D(-1.0, -1.0),
                k1, k2, p1, p2, focalLengths, centerPixel, imageSize);
        float maxX = std::abs(undistortedCubeCorner[0].x());
        float maxY = std::abs(undistortedCubeCorner[0].y());
        for (int i = 0; i < 3; i++) {
            if (std::abs(undistortedCubeCorner[i].x()) > maxX)
                maxX = std::abs(undistortedCubeCorner[i].x());
            if (std::abs(undistortedCubeCorner[i].y()) > maxY)
                maxY = std::abs(undistortedCubeCorner[i].y());
        }
        prg.setUniformValue("k1", k1);
        prg.setUniformValue("k2", k2);
        prg.setUniformValue("p1", p1);
        prg.setUniformValue("fx", focalLengths.x());
        prg.setUniformValue("fy", focalLengths.y());
        prg.setUniformValue("cx", centerPixel.x());
        prg.setUniformValue("cy", centerPixel.y());
        prg.setUniformValue("width", imageSize.width());
        prg.setUniformValue("height", imageSize.height());
        prg.setUniformValue("undistortedCubeCorner", QVector2D(maxX, maxY));
        prg.setUniformValue("lensDistMargin", _pipeline.preprocLensDistortionMargin);
    }

    // Set dynamic light properties. This is costly; do it only when we actually use light sources
    if (&prg == &_lightPrg) {
        if (_output.rgb && _pipeline.gaussianWhiteNoise) {
            QVector4D rn0, rn1;
            std::uniform_real_distribution<float> distribution(0.0f, 1000.0f);
            rn0.setX(distribution(_randomGenerator));
            rn0.setY(distribution(_randomGenerator));
            rn0.setZ(distribution(_randomGenerator));
            rn0.setW(distribution(_randomGenerator));
            rn1.setX(distribution(_randomGenerator));
            rn1.setY(distribution(_randomGenerator));
            rn1.setZ(distribution(_randomGenerator));
            rn1.setW(distribution(_randomGenerator));
            prg.setUniformValue("random_noise_0", rn0);
            prg.setUniformValue("random_noise_1", rn1);
            prg.setUniformValue("gwn_stddev", _pipeline.gaussianWhiteNoiseStddev);
            prg.setUniformValue("gwn_mean", _pipeline.gaussianWhiteNoiseMean);
        }
        int samplersForPowerFactorMaps = (_pipeline.lightPowerFactorMaps ? _scene.lights.size() : 0);
        int samplersForShadowMaps = (_pipeline.shadowMaps ? _scene.lights.size() : 0);
        QVector<float> tmpVec3_0(3 * _scene.lights.size());
        QVector<float> tmpVec3_1(3 * _scene.lights.size());
        QVector<float> tmpVec3_2(3 * _scene.lights.size());
        QVector<int> tmpInt_0(_scene.lights.size());
        QVector<int> tmpInt_1(_scene.lights.size());
        QVector<int> tmpInt_2(_scene.lights.size());
        QVector<int> tmpInt_3(_scene.lights.size());
        QVector<int> tmpInt_4(_scene.lights.size());
        QVector<int> tmpInt_5(_scene.lights.size());
        QVector<int> tmpInt_6(_scene.lights.size());
        QVector<float> tmpFloat_0(_scene.lights.size());
        QVector<float> tmpFloat_1(_scene.lights.size());
        QVector<float> tmpFloat_2(_scene.lights.size());
        QVector<float> tmpFloat_3(_scene.lights.size());
        QVector<float> tmpFloat_4(_scene.lights.size());
        for (int i = 0; i < _scene.lights.size(); i++) {
            QVector3D lightPosition = lightTransformations[i].translation + _scene.lights[i].position;
            QVector3D lightDirection = lightTransformations[i].rotation * _scene.lights[i].direction;
            QVector3D lightUp = lightTransformations[i].rotation * _scene.lights[i].up;
            if (!_scene.lights[i].isRelativeToCamera) {
                lightPosition = viewMatrix.map(lightPosition);
                lightDirection = viewMatrix.mapVector(lightDirection);
                lightUp = viewMatrix.mapVector(lightUp);
            }
            for (int j = 0; j < 3; j++)
                tmpVec3_0[3 * i + j] = lightPosition[j];
            for (int j = 0; j < 3; j++)
                tmpVec3_1[3 * i + j] = lightDirection[j];
            for (int j = 0; j < 3; j++)
                tmpVec3_2[3 * i + j] = lightUp[j];
            if (_pipeline.shadowMaps) {
                tmpInt_0[i] = (_scene.lights[i].shadowMap ? 1 : 0);
                tmpInt_1[i] = 10 + samplersForPowerFactorMaps + i;
                gl->glActiveTexture(GL_TEXTURE0 + tmpInt_1[i]);
                gl->glBindTexture(GL_TEXTURE_CUBE_MAP, _shadowMapDepthBufs[subFrame][i]);
                tmpFloat_0[i] = _scene.lights[i].shadowMapDepthBias;
            }
            if (_pipeline.reflectiveShadowMaps) {
                tmpInt_2[i] = (_scene.lights[i].reflectiveShadowMap ? 1 : 0);
                tmpInt_3[i] = 10 + samplersForPowerFactorMaps + samplersForShadowMaps + i;
                tmpInt_4[i] = std::sqrt(3.0f) * _scene.lights[i].reflectiveShadowMapSize;
                gl->glActiveTexture(GL_TEXTURE0 + tmpInt_3[i]);
                gl->glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, _reflectiveShadowMapTexs[subFrame][i]);
            }
            if (_pipeline.lightPowerFactorMaps) {
                tmpInt_5[i] = (_scene.lights[i].powerFactorTex == 0 ? 0 : 1);
                tmpInt_6[i] = 10 + i;
                gl->glActiveTexture(GL_TEXTURE10 + i);
                gl->glBindTexture(GL_TEXTURE_2D, _scene.lights[i].powerFactorTex);
                tmpFloat_1[i] = qDegreesToRadians(_scene.lights[i].powerFactorMapAngleLeft);
                tmpFloat_2[i] = qDegreesToRadians(_scene.lights[i].powerFactorMapAngleRight);
                tmpFloat_3[i] = qDegreesToRadians(_scene.lights[i].powerFactorMapAngleBottom);
                tmpFloat_4[i] = qDegreesToRadians(_scene.lights[i].powerFactorMapAngleTop);
            }
        }
        prg.setUniformValueArray("light_position", tmpVec3_0.constData(), _scene.lights.size(), 3);
        prg.setUniformValueArray("light_direction", tmpVec3_1.constData(), _scene.lights.size(), 3);
        prg.setUniformValueArray("light_up", tmpVec3_2.constData(), _scene.lights.size(), 3);
        if (_pipeline.shadowMaps) {
            prg.setUniformValueArray("light_have_shadowmap", tmpInt_0.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_shadowmap", tmpInt_1.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_depth_bias", tmpFloat_0.constData(), _scene.lights.size(), 1);
        }
        if (_pipeline.reflectiveShadowMaps) {
            prg.setUniformValueArray("light_have_reflective_shadowmap", tmpInt_2.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_reflective_shadowmap", tmpInt_3.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_reflective_shadowmap_hemisphere_samples_root", tmpInt_4.constData(), _scene.lights.size());
        }
        if (_pipeline.lightPowerFactorMaps) {
            prg.setUniformValueArray("light_have_power_factor_tex", tmpInt_5.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_power_factor_tex", tmpInt_6.constData(), _scene.lights.size());
            prg.setUniformValueArray("light_power_factor_left", tmpFloat_1.constData(), _scene.lights.size(), 1);
            prg.setUniformValueArray("light_power_factor_right", tmpFloat_2.constData(), _scene.lights.size(), 1);
            prg.setUniformValueArray("light_power_factor_bottom", tmpFloat_3.constData(), _scene.lights.size(), 1);
            prg.setUniformValueArray("light_power_factor_top", tmpFloat_4.constData(), _scene.lights.size(), 1);
        }
    }
    ASSERT_GLCHECK();

    drawScene(prg, projectionMatrix, viewMatrix, lastViewMatrix, nextViewMatrix,
            objectTransformations, lastObjectTransformations, nextObjectTransformations);
}

void Simulator::simulateShadowMap(bool reflective,
        int subFrame, int lightIndex,
        const Transformation& cameraTransformation,
        const QVector<Transformation>& lightTransformations,
        const QVector<Transformation>& objectTransformations)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();

    QOpenGLShaderProgram& prg = (reflective ? _reflectiveShadowMapPrg : _shadowMapPrg);
    prg.bind();
    const Light& light = _scene.lights[lightIndex];
    const Transformation& lightTransformation = lightTransformations[lightIndex];

    // Light pose
    QVector3D lightPosition = lightTransformation.translation + light.position;
    QVector3D lightDirection = lightTransformation.rotation * light.direction;
    QVector3D lightUp = lightTransformation.rotation * light.up;
    if (light.isRelativeToCamera) {
        QMatrix4x4 cameraMatrix = _cameraTransformation.toMatrix4x4() * cameraTransformation.toMatrix4x4();
        lightPosition = cameraMatrix.map(lightPosition);
        lightDirection = cameraMatrix.mapVector(lightDirection);
        lightUp = cameraMatrix.mapVector(lightUp);
    }

    // Set static parameters that do not trigger recreateShaders
    prg.setUniformValue("far_plane", _pipeline.farClippingPlane);
    prg.setUniformValue("last_depth_buf", 0);
    prg.setUniformValue("thin_lens_vignetting", 0);

    // Set light properties
    int itmp;
    float ftmp;
    float vtmp[3];
    itmp = light.type;
    prg.setUniformValueArray("light_type", &itmp, 1);
    vtmp[0] = 0.0f;
    vtmp[1] = 0.0f;
    vtmp[2] = 0.0f;
    prg.setUniformValueArray("light_position", vtmp, 1, 3);
    vtmp[2] = -1.0f;
    prg.setUniformValueArray("light_direction", vtmp, 1, 3);
    vtmp[2] = 0.0f;
    vtmp[1] = 1.0f;
    prg.setUniformValueArray("light_up", vtmp, 1, 3);
    ftmp = qDegreesToRadians(light.innerConeAngle);
    prg.setUniformValueArray("light_inner_cone_angle", &ftmp, 1, 1);
    ftmp = qDegreesToRadians(light.outerConeAngle);
    prg.setUniformValueArray("light_outer_cone_angle", &ftmp, 1, 1);
    vtmp[0] = light.attenuationConstant;
    vtmp[1] = light.attenuationLinear;
    vtmp[2] = light.attenuationQuadratic;
    prg.setUniformValueArray("light_attenuation", vtmp, 1, 3);
    vtmp[0] = light.color[0];
    vtmp[1] = light.color[1];
    vtmp[2] = light.color[2];
    prg.setUniformValueArray("light_color", vtmp, 1, 3);
    ftmp = lightIntensity(lightIndex);
    prg.setUniformValueArray("light_intensity", &ftmp, 1, 1);
    itmp = (light.powerFactorTex == 0 ? 0 : 1);
    if (_pipeline.lightPowerFactorMaps) {
        prg.setUniformValueArray("light_have_power_factor_tex", &itmp, 1);
        itmp = 10;
        prg.setUniformValueArray("light_power_factor_tex", &itmp, 1);
        gl->glActiveTexture(GL_TEXTURE10);
        gl->glBindTexture(GL_TEXTURE_2D, light.powerFactorTex);
        ftmp = qDegreesToRadians(light.powerFactorMapAngleLeft);
        prg.setUniformValueArray("light_power_factor_left", &ftmp, 1, 1);
        ftmp = qDegreesToRadians(light.powerFactorMapAngleRight);
        prg.setUniformValueArray("light_power_factor_right", &ftmp, 1, 1);
        ftmp = qDegreesToRadians(light.powerFactorMapAngleBottom);
        prg.setUniformValueArray("light_power_factor_bottom", &ftmp, 1, 1);
        ftmp = qDegreesToRadians(light.powerFactorMapAngleTop);
        prg.setUniformValueArray("light_power_factor_top", &ftmp, 1, 1);
    }
    ASSERT_GLCHECK();

    // Configure the custom space for positions and normals,
    // which in our case is the eye space of the camera.
    Transformation customTransformationBak;
    if (reflective) {
        QMatrix4x4 cameraMatrix = _cameraTransformation.toMatrix4x4() * cameraTransformation.toMatrix4x4();
        customTransformationBak = _customTransformation;
        _customTransformation = Transformation::fromMatrix4x4(cameraMatrix.inverted());
    }

    QMatrix4x4 projectionMatrix;
    projectionMatrix.perspective(90.0f, 1.0f, _pipeline.nearClippingPlane, _pipeline.farClippingPlane);

    const QVector3D cubeLightDir[6] = {
        QVector3D(+1.0f, 0.0f, 0.0f),
        QVector3D(-1.0f, 0.0f, 0.0f),
        QVector3D(0.0f, +1.0f, 0.0f),
        QVector3D(0.0f, -1.0f, 0.0f),
        QVector3D(0.0f, 0.0f, +1.0f),
        QVector3D(0.0f, 0.0f, -1.0f),
    };
    const QVector3D cubeLightUp[6] = {
        // Note that these up vectors are the opposite of what one would expect...
        QVector3D(0.0f, -1.0f, 0.0f),
        QVector3D(0.0f, -1.0f, 0.0f),
        QVector3D(0.0f, 0.0f, +1.0f),
        QVector3D(0.0f, 0.0f, -1.0f),
        QVector3D(0.0f, -1.0f, 0.0f),
        QVector3D(0.0f, -1.0f, 0.0f),
    };
    for (int cubeSide = 0; cubeSide < 6; cubeSide++) {
        QMatrix4x4 viewMatrix;
        viewMatrix.lookAt(lightPosition, lightPosition + cubeLightDir[cubeSide], cubeLightUp[cubeSide]);
        if (reflective) {
            prepareFBO(QSize(light.reflectiveShadowMapSize, light.reflectiveShadowMapSize),
                    _reflectiveShadowMapDepthBufs[subFrame][lightIndex], false,
                    { _reflectiveShadowMapTexs[subFrame][lightIndex] }, cubeSide, 5);
        } else {
            prepareFBO(QSize(light.shadowMapSize, light.shadowMapSize),
                    _shadowMapDepthBufs[subFrame][lightIndex], false,
                    {}, cubeSide);
        }
        drawScene(prg, projectionMatrix, viewMatrix, viewMatrix, viewMatrix,
                objectTransformations, objectTransformations, objectTransformations);
    }

    if (reflective) {
        _customTransformation = customTransformationBak;
    }
}

void Simulator::simulateShadowMaps(int subFrame,
        const Transformation& cameraTransformation,
        const QVector<Transformation>& lightTransformations,
        const QVector<Transformation>& objectTransformations)
{
    for (int l = 0; l < _scene.lights.size(); l++) {
        const Light& light = _scene.lights[l];
        if (_pipeline.shadowMaps && light.shadowMap) {
            simulateShadowMap(false, subFrame, l,
                    cameraTransformation, lightTransformations, objectTransformations);
        }
        if (_pipeline.reflectiveShadowMaps && light.reflectiveShadowMap) {
            simulateShadowMap(true, subFrame, l,
                    cameraTransformation, lightTransformations, objectTransformations);
        }
    }
}

void Simulator::simulateDepth(int subFrame, long long t,
        const Transformation& cameraTransformation,
        const QVector<Transformation>& lightTransformations,
        const QVector<Transformation>& objectTransformations)
{
    simulate(_depthPrg, subFrame, t, t, t, 0,
            cameraTransformation, lightTransformations, objectTransformations);
}

void Simulator::simulateLight(int subFrame, long long t,
        const Transformation& cameraTransformation,
        const QVector<Transformation>& lightTransformations,
        const QVector<Transformation>& objectTransformations)
{
    simulate(_lightPrg, subFrame, t, t, t, 0,
            cameraTransformation, lightTransformations, objectTransformations);
}

void Simulator::simulateOversampledLight()
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _lightOversampledPrg.bind();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, _oversampledLightSimOutputTexs[0]);
    if (_oversampledLightSimOutputTexs.size() > 1) {
        gl->glActiveTexture(GL_TEXTURE1);
        gl->glBindTexture(GL_TEXTURE_2D, _oversampledLightSimOutputTexs[1]);
    }
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulatePMDDigNums()
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _pmdDigNumPrg.bind();
    _pmdDigNumPrg.setUniformValue("wavelength", _pmd.wavelength);
    _pmdDigNumPrg.setUniformValue("quantum_efficiency", _pmd.quantumEfficiency);
    _pmdDigNumPrg.setUniformValue("max_electrons", _pmd.maxElectrons);
    if (_pipeline.shotNoise) {
        QVector4D rn;
        std::uniform_real_distribution<float> distribution(0.0f, 1000.0f);
        rn.setX(distribution(_randomGenerator));
        rn.setY(distribution(_randomGenerator));
        rn.setZ(distribution(_randomGenerator));
        rn.setW(distribution(_randomGenerator));
        _pmdDigNumPrg.setUniformValue("random_noise", rn);
    }
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, _pmdEnergyTex);
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulateRGBResult()
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _rgbResultPrg.bind();
    for (int i = 0; i < subFrames(); i++) {
        gl->glActiveTexture(GL_TEXTURE0 + i);
        gl->glBindTexture(GL_TEXTURE_2D, _rgbTexs[i]);
    }
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulatePMDResult()
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _pmdResultPrg.bind();
    _pmdResultPrg.setUniformValue("frac_c_modfreq",
            static_cast<float>(speedOfLight / _pmd.modulationFrequency));
    for (int i = 0; i < subFrames(); i++) {
        gl->glActiveTexture(GL_TEXTURE0 + i);
        gl->glBindTexture(GL_TEXTURE_2D, _pmdDigNumTexs[i]);
    }
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulatePMDCoordinates()
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _pmdCoordinatesPrg.bind();
    _pmdCoordinatesPrg.setUniformValue("w", static_cast<float>(_projection.imageSize().width()));
    _pmdCoordinatesPrg.setUniformValue("h", static_cast<float>(_projection.imageSize().height()));
    _pmdCoordinatesPrg.setUniformValue("fx", _projection.focalLengths().x());
    _pmdCoordinatesPrg.setUniformValue("fy", _projection.focalLengths().y());
    _pmdCoordinatesPrg.setUniformValue("cx", _projection.centerPixel().x());
    _pmdCoordinatesPrg.setUniformValue("cy", _projection.centerPixel().y());
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, _pmdDigNumTexs[subFrames()]);
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulateGeometry(int subFrame)
{
    simulate(_geomPrg, subFrame,
            _timestamps[subFrame], _timestamps[subFrame], _timestamps[subFrame], 0,
            _cameraTransformations[subFrame], _lightTransformations[subFrame],
            _objectTransformations[subFrame]);
}

void Simulator::simulateFlow(int subFrame, long long lastT, long long nextT, unsigned int lastDepthBuf)
{
    simulate(_flowPrg, subFrame,
            _timestamps[subFrame], lastT, nextT, lastDepthBuf,
            _cameraTransformations[subFrame], _lightTransformations[subFrame],
            _objectTransformations[subFrame]);
}

void Simulator::simulatePostprocLensDistortion(const QList<unsigned int>& textures)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _postprocLensDistortionPrg.bind();
    float k1, k2, p1, p2;
    _projection.distortion(&k1, &k2, &p1, &p2);
    QVector2D centerPixel = _projection.centerPixel();
    QVector2D focalLengths = _projection.focalLengths();
    _postprocLensDistortionPrg.setUniformValue("k1", k1);
    _postprocLensDistortionPrg.setUniformValue("k2", k2);
    _postprocLensDistortionPrg.setUniformValue("p1", p1);
    _postprocLensDistortionPrg.setUniformValue("p2", p2);
    _postprocLensDistortionPrg.setUniformValue("fx", focalLengths.x());
    _postprocLensDistortionPrg.setUniformValue("fy", focalLengths.y());
    _postprocLensDistortionPrg.setUniformValue("cx", centerPixel.x());
    _postprocLensDistortionPrg.setUniformValue("cy", centerPixel.y());
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < textures.size(); i++) {
        prepareFBO(_projection.imageSize(), 0, false, { _postProcessingTex });
        gl->glBindTexture(GL_TEXTURE_2D, textures[i]);
        float borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        gl->glTexParameterfv(GL_TEXTURE_2D,  GL_TEXTURE_BORDER_COLOR, borderColor);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl->glCopyImageSubData(_postProcessingTex, GL_TEXTURE_2D, 0, 0, 0, 0,
                textures[i], GL_TEXTURE_2D, 0, 0, 0, 0,
                _projection.imageSize().width(), _projection.imageSize().height(), 1);
    }
    ASSERT_GLCHECK();
}

void Simulator::convertToSRGB(int texIndex)
{
    auto gl = getGlFunctionsFromCurrentContext(Q_FUNC_INFO);
    ASSERT_GLCHECK();
    _convertToSRGBPrg.bind();
    gl->glActiveTexture(GL_TEXTURE0);
    gl->glBindTexture(GL_TEXTURE_2D, _rgbTexs[texIndex]);
    gl->glBindVertexArray(_fullScreenQuadVao);
    gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    ASSERT_GLCHECK();
}

void Simulator::simulate(long long frameTimestamp)
{
    recreateShadersIfNecessary();
    recreateOutputIfNecessary();

    // Determine sub frame timestamps as well as camera, light, and object
    // transformations at these timestamps. This handles _pipeline.subFrameTemporalSampling.
    simulateTimestamps(frameTimestamp);

    // Simulate sub frames
    for (int subFrame = 0; subFrame < subFrames(); subFrame++) {
        long long tempSampleDuration = subFrameDuration() / _pipeline.temporalSamples;
        if (_output.rgb || _output.pmd) {
            long long tempSampleTimestamp = _timestamps[subFrame];
            Transformation cameraTransformation = _cameraTransformations[subFrame];
            QVector<Transformation> lightTransformations = _lightTransformations[subFrame];
            QVector<Transformation> objectTransformations = _objectTransformations[subFrame];
            for (int tempSample = 0; tempSample < _pipeline.temporalSamples; tempSample++) {
                if (tempSample > 0) {
                    tempSampleTimestamp = _timestamps[subFrame] + tempSample * tempSampleDuration;
                    simulateSampleTimestamp(tempSampleTimestamp, cameraTransformation, lightTransformations, objectTransformations);
                }
                for (int l = 0; l < _scene.lights.size(); l++)
                    _scene.lights[l].updatePowerFactorTex(_pbo, tempSampleTimestamp);
                if (_pipeline.shadowMaps || _pipeline.reflectiveShadowMaps)
                    simulateShadowMaps(subFrame, cameraTransformation, lightTransformations, objectTransformations);
                // The following is the core rendering step for light simulation
                if (spatialOversampling() || temporalOversampling()) {
                    prepareFBO(spatialOversamplingSize(), _depthBufferOversampled, false, _oversampledLightSimOutputTexs);
                    simulateLight(subFrame, tempSampleTimestamp, cameraTransformation, lightTransformations, objectTransformations);
                    prepareFBO(_projection.imageSize(), 0, false, _lightSimOutputTexs[subFrame], -1, 0, temporalOversampling(), tempSample == 0);
                    simulateOversampledLight();
                } else {
                    // short cut for common case; the above would also be correct but requires an additional render pass
                    prepareFBO(_projection.imageSize(), _depthBuffers[subFrame], false, _lightSimOutputTexs[subFrame]);
                    simulateLight(subFrame, tempSampleTimestamp, cameraTransformation, lightTransformations, objectTransformations);
                }
            }
            if (_output.pmd) {
                prepareFBO(_projection.imageSize(), 0, false, { _pmdDigNumTexs[subFrame] });
                simulatePMDDigNums();
            }
            if (_pipeline.postprocLensDistortion) {
                simulatePostprocLensDistortion(_lightSimOutputTexs[subFrame]);
            }
            if (_output.srgb) {
                prepareFBO(_projection.imageSize(), 0, false, { _srgbTexs[subFrame] });
                convertToSRGB(subFrame);
            }
        }
        if (_output.eyeSpacePositions || _output.customSpacePositions
                || _output.eyeSpaceNormals || _output.customSpaceNormals
                || _output.depthAndRange || _output.indices) {
            prepareFBO(_projection.imageSize(), _depthBuffers[subFrame], false, _geomSimOutputTexs[subFrame]);
            simulateGeometry(subFrame);
            if (_pipeline.postprocLensDistortion) {
                simulatePostprocLensDistortion(_geomSimOutputTexs[subFrame]);
            }
        }
        if (_output.forwardFlow2D || _output.forwardFlow3D || _output.backwardFlow3D || _output.backwardFlow2D) {
            long long lastSubFrameTimestamp;
            if (subFrame == 0 && !_haveLastFrameTimestamp) {
                lastSubFrameTimestamp = frameTimestamp;
            } else if (subFrame == 0) {
                lastSubFrameTimestamp = _lastFrameTimestamp;
                if (_pipeline.subFrameTemporalSampling)
                    lastSubFrameTimestamp += (subFrames() - 1) * subFrameDuration();
            } else {
                lastSubFrameTimestamp = _timestamps[subFrame - 1];
            }
            long long nextSubFrameTimestamp;
            if (subFrame == subFrames() - 1) {
                nextSubFrameTimestamp = frameTimestamp + frameDuration();
            } else {
                nextSubFrameTimestamp = _timestamps[subFrame + 1];
            }
            static bool depthBufferPingPong = true; // static: initialized only once and value kept between calls!
            unsigned long lastSubFrameDepthBuffer;
            unsigned long depthBuffer = _depthBuffers[subFrame];
            if (subFrame == 0 && !_haveLastFrameTimestamp) {
                lastSubFrameDepthBuffer = 0;
            } else {
                if (subFrame == 0) {
                    if (subFrames() == 1) {
                        if (depthBufferPingPong) {
                            depthBuffer = _depthBuffers[1];
                            lastSubFrameDepthBuffer = _depthBuffers[0];
                        } else {
                            depthBuffer = _depthBuffers[0];
                            lastSubFrameDepthBuffer = _depthBuffers[1];
                        }
                        depthBufferPingPong = !depthBufferPingPong;
                    } else {
                        lastSubFrameDepthBuffer = _depthBuffers[subFrames() - 1];
                    }
                } else {
                    lastSubFrameDepthBuffer = _depthBuffers[subFrame - 1];
                }
            }
            prepareFBO(_projection.imageSize(), depthBuffer, false, _flowSimOutputTexs[subFrame]);
            simulateFlow(subFrame, lastSubFrameTimestamp, nextSubFrameTimestamp, lastSubFrameDepthBuffer);
        }
    }

    // Simulate final frames.
    // Note that this is not necessary for geometry simulation since we have that info
    // already in the first subframe.
    // It is however necessary for flow simulation because full-frame timestamps differ
    // from sub-frame timestamps.
    if (subFrames() > 1) {
        if (_output.rgb) {
            prepareFBO(_projection.imageSize(), 0, false, { _rgbTexs[subFrames()] });
            simulateRGBResult();
            if (_output.srgb) {
                prepareFBO(_projection.imageSize(), 0, false, { _srgbTexs[subFrames()] });
                convertToSRGB(subFrames());
            }
        }
        if (_output.pmd) {
            prepareFBO(_projection.imageSize(), 0, false, { _pmdDigNumTexs[subFrames()] });
            simulatePMDResult();
            if (_output.pmdCoordinates) {
                prepareFBO(_projection.imageSize(), 0, false, { _pmdCoordinatesTex });
                simulatePMDCoordinates();
            }
        }
        if (_output.forwardFlow3D || _output.forwardFlow2D || _output.backwardFlow3D || _output.backwardFlow2D) {
            long long lastFrameTimestamp = frameTimestamp - frameDuration();
            long long nextFrameTimestamp = frameTimestamp + frameDuration();
            unsigned long lastFrameDepthBuffer = 0;
            if (_haveLastFrameTimestamp) {
                lastFrameDepthBuffer = _depthBuffers[0]; // first subframe timestamp == frame timestamp
            }
            prepareFBO(_projection.imageSize(), _depthBuffers[subFrames()], false, _flowSimOutputTexs[subFrames()]);
            simulateFlow(0, lastFrameTimestamp, nextFrameTimestamp, lastFrameDepthBuffer);
        }
    }

    _lastFrameTimestamp = frameTimestamp;
    _haveLastFrameTimestamp = true;
}

long long Simulator::getTimestamp(int i) const
{
    if (!_haveLastFrameTimestamp || i < -1 || i >= subFrames())
        return 0;
    else
        return _timestamps[i < 0 ? 0 : i];
}

bool Simulator::haveValidOutput(int i) const
{
    return (!_recreateOutput && _haveLastFrameTimestamp && i >= -1 && i < subFrames());
}

bool Simulator::haveShadowMap(int lightIndex) const
{
    return (_pipeline.shadowMaps && lightIndex >= 0 && lightIndex < _scene.lights.size() && _scene.lights[lightIndex].shadowMap);
}

bool Simulator::haveReflectiveShadowMap(int lightIndex) const
{
    return (_pipeline.reflectiveShadowMaps && lightIndex >= 0 && lightIndex < _scene.lights.size() && _scene.lights[lightIndex].reflectiveShadowMap);
}

unsigned int Simulator::getShadowMapCubeTex(int lightIndex, int i) const
{
    return ((haveValidOutput(i) && haveShadowMap(lightIndex))
            ? (i == -1 ? _shadowMapDepthBufs[0][lightIndex] : _shadowMapDepthBufs[i][lightIndex]) : 0);
}

unsigned int Simulator::getReflectiveShadowMapCubeArrayTex(int lightIndex, int i) const
{
    return ((haveValidOutput(i) && haveReflectiveShadowMap(lightIndex))
            ? (i == -1 ? _reflectiveShadowMapTexs[0][lightIndex] : _reflectiveShadowMapTexs[i][lightIndex]) : 0);
}

unsigned int Simulator::getDepthTex(int i) const
{
    return ((((!spatialOversampling() && !temporalOversampling())
                    || _output.eyeSpacePositions || _output.customSpacePositions
                    || _output.eyeSpaceNormals || _output.customSpaceNormals
                    || _output.depthAndRange || _output.indices
                    || _output.forwardFlow3D || _output.forwardFlow2D
                    || _output.backwardFlow3D || _output.backwardFlow2D)
                && haveValidOutput(i)) ? (i == -1 ? _depthBuffers[0] : _depthBuffers[i]) : 0);
}

unsigned int Simulator::getRGBTex(int i) const
{
    return ((_output.rgb && haveValidOutput(i)) ? (i == -1 ? _rgbTexs.last() : _rgbTexs[i]) : 0);
}

unsigned int Simulator::getSRGBTex(int i) const
{
    return ((_output.rgb && _output.srgb && haveValidOutput(i)) ? (i == -1 ? _srgbTexs.last() : _srgbTexs[i]) : 0);
}

unsigned int Simulator::getPMDTex(int i) const
{
    return ((_output.pmd && haveValidOutput(i)) ? (i == -1 ? _pmdDigNumTexs.last() : _pmdDigNumTexs[i]) : 0);
}

unsigned int Simulator::getPMDCoordinatesTex() const
{
    return ((_output.pmd && _output.pmdCoordinates && haveValidOutput(-1)) ? _pmdCoordinatesTex : 0);
}

unsigned int Simulator::getEyeSpacePositionsTex(int i) const
{
    return ((_output.eyeSpacePositions && haveValidOutput(i)) ? (i == -1 ? _eyeSpacePosTexs[0] : _eyeSpacePosTexs[i]) : 0);
}

unsigned int Simulator::getCustomSpacePositionsTex(int i) const
{
    return ((_output.customSpacePositions && haveValidOutput(i)) ? (i == -1 ? _customSpacePosTexs[0] : _customSpacePosTexs[i]) : 0);
}

unsigned int Simulator::getEyeSpaceNormalsTex(int i) const
{
    return ((_output.eyeSpaceNormals && haveValidOutput(i)) ? (i == -1 ? _eyeSpaceNormalTexs[0] : _eyeSpaceNormalTexs[i]) : 0);
}

unsigned int Simulator::getCustomSpaceNormalsTex(int i) const
{
    return ((_output.customSpaceNormals && haveValidOutput(i)) ? (i == -1 ? _customSpaceNormalTexs[0] : _customSpaceNormalTexs[i]) : 0);
}

unsigned int Simulator::getDepthAndRangeTex(int i) const
{
    return ((_output.depthAndRange && haveValidOutput(i)) ? (i == -1 ? _depthAndRangeTexs[0] : _depthAndRangeTexs[i]) : 0);
}

unsigned int Simulator::getIndicesTex(int i) const
{
    return ((_output.indices && haveValidOutput(i)) ? (i == -1 ? _indicesTexs[0] : _indicesTexs[i]) : 0);
}

unsigned int Simulator::getForwardFlow3DTex(int i) const
{
    return ((_output.forwardFlow3D && haveValidOutput(i)) ? (i == -1 ? _forwardFlow3DTexs.last() : _forwardFlow3DTexs[i]) : 0);
}

unsigned int Simulator::getForwardFlow2DTex(int i) const
{
    return ((_output.forwardFlow2D && haveValidOutput(i)) ? (i == -1 ? _forwardFlow2DTexs.last() : _forwardFlow2DTexs[i]) : 0);
}

unsigned int Simulator::getBackwardFlow3DTex(int i) const
{
    return ((_output.backwardFlow3D && haveValidOutput(i)) ? (i == -1 ? _backwardFlow3DTexs.last() : _backwardFlow3DTexs[i]) : 0);
}

unsigned int Simulator::getBackwardFlow2DTex(int i) const
{
    return ((_output.backwardFlow2D && haveValidOutput(i)) ? (i == -1 ? _backwardFlow2DTexs.last() : _backwardFlow2DTexs[i]) : 0);
}

static const Transformation NullTransformation;

const Transformation& Simulator::getCameraTransformation(int i) const
{
    return (haveValidOutput(i) ? (i == -1 ? _cameraTransformations[0] : _cameraTransformations[i]) : NullTransformation);
}

const Transformation& Simulator::getLightTransformation(int lightIndex, int i) const
{
    if (lightIndex >= 0 && lightIndex < _scene.lights.size() && haveValidOutput(i))
        return (i == -1 ? _lightTransformations[lightIndex][0] : _lightTransformations[lightIndex][i]);
    else
        return NullTransformation;
}

const Transformation& Simulator::getObjectTransformation(int objectIndex, int i) const
{
    if (objectIndex >= 0 && objectIndex < _scene.objects.size() && haveValidOutput(i))
        return (i == -1 ? _objectTransformations[objectIndex][0] : _objectTransformations[objectIndex][i]);
    else
        return NullTransformation;
}

}

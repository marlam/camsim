/*
 * Copyright (C) 2012, 2013, 2014, 2016, 2017, 2018
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

#ifndef CAMSIM_SIMULATOR_HPP
#define CAMSIM_SIMULATOR_HPP

#include <random>

#include <QList>
#include <QVector>
#include <QSize>
#include <QVector2D>
#include <QOpenGLShaderProgram>

class QOffscreenSurface;
class QOpenGLContext;

#include "scene.hpp"
#include "animation.hpp"
#include "texdata.hpp"


namespace CamSim {

/*! \brief Speed of light in m/s */
constexpr double speedOfLight = 299792458;
/*! \brief Elementary charge in Attocoulomb (1e-18 C). */
constexpr double elementaryCharge = 0.1602176565;

/**
 * \brief Defines chip timings and therefore frames-per-second
 *
 * In physically-based simulations, the exposure time is important
 * since it determines how much energy a chip pixel receives.
 *
 * For simple RGB simulation, physical quantities are irrelevant
 * and you can simply set the exposure time to zero and the readout
 * time to the duration of one frame.
 */
class ChipTiming
{
public:
    /*! \brief Constructor */
    ChipTiming();

    /*! \brief Exposure time for each subframe in seconds */
    double exposureTime;
    /*! \brief Readout time for each subframe in seconds */
    double readoutTime;
    /*! \brief Pause time after all subframes, before the start of a new frame */
    double pauseTime;

    /*! \brief Return the duration of one subframe resulting from these timings */
    double subFrameDuration() const { return exposureTime + readoutTime; }

    /*! \brief Return the subframes-per-second rate resulting from these timings */
    double subFramesPerSecond() const { return 1.0 / subFrameDuration(); }

    /*! Generate chip timings from subframes-per-second. This ignores exposure
     * time, so this is not suitable for physically-based simulations such
     * as PMD simulation. */
    static ChipTiming fromSubFramesPerSecond(float sfps);
};

/**
 * \brief Defines a PMD chip
 */
class PMD
{
public:
    /*! \brief Constructor */
    PMD();

    /*! \brief Pixel size in micrometers² (also known as pixel pitch) */
    double pixelSize;
    /*! \brief Pixel achievable contrast in [0,1] */
    double pixelContrast;
    /*! \brief Modulation Frequency in Hz */
    double modulationFrequency;
    /* \brief wavelength of light pulse in nm */
    float wavelength;
    /* \brief quantum efficiency of camera */
    float quantumEfficiency;
    /* \brief maximum number of electrons per pixel */
    int maxElectrons;
};

/**
 * \brief Defines the camera projection onto the image plane
 *
 * For OpenGL, this means viewport and projection matrix information.
 * For OpenCV, this means image size and intrinsic parameter information.
 */
class Projection
{
private:
    int _w, _h;
    float _l, _r, _b, _t; // frustum for near=1
    float _k1, _k2, _p1, _p2; // lens distortion parameters, compatible to OpenCV

public:
    /*! \brief Constructor */
    Projection();

    /*! \brief Get the image size */
    QSize imageSize() const;

    /*! \brief For OpenGL: Get the projection matrix for the given near and far plane values \a n and \a f */
    QMatrix4x4 projectionMatrix(float n, float f) const;

    /*! \brief For OpenCV: get the center pixel coordinates */
    QVector2D centerPixel() const;
    /*! \brief For OpenCV: get the focal lengths in x and y direction, in millimeters */
    QVector2D focalLengths() const;

    /*! \brief Create a camera transformation from image size and frustum information (relative to a near plane value of 1) */
    static Projection fromFrustum(int imageWidth, int imageHeight,
            float l, float r, float b, float t);
    /*! \brief Create a camera transformation from image size and vertical opening angle (in degrees) */
    static Projection fromOpeningAngle(int imageWidth, int imageHeight, float fovy);
    /*! \brief Create a camera transformation from image size and camera intrinsic parameters as used by OpenCV */
    static Projection fromIntrinsics(int imageWidth, int imageHeight,
            float centerX, float centerY, float focalLengthX, float focalLengthY);

    /*! \brief Set lens distortion parameters, compatible to OpenCV.
     * Set everything to zero to disable lens distortion. */
    void setDistortion(float k1, float k2, float p1, float p2);
    /*! \brief Get lens distortion parameters, compatible to OpenCV. */
    void distortion(float* k1, float* k2, float* p1, float* p2);
};

/**
 * \brief Defines the rendering pipeline
 */
class Pipeline
{
public:
    /*! \brief Constructor */
    Pipeline();

    /**
     * \name Basic parameters
     */
    /*@{*/

    /*! \brief Near clipping plane value */
    float nearClippingPlane;
    /*! \brief Far clipping plane value */
    float farClippingPlane;

    /*@}*/

    /**
     * \name Simple feature switches
     */
    /*@{*/

    /*! \brief Flag: enable mipmapping? */
    bool mipmapping;
    /*! \brief Flag: enable anisotropic filtering? */
    bool anisotropicFiltering;
    /*! \brief Flag: enable transparency (discard fragments with opacity < 0.5)? */
    bool transparency;
    /*! \brief Flag: enable normal mapping via bump map or normal map? */
    bool normalMapping;
    /*! \brief Flag: use ambient light and colors? (should be off, but may be needed for imported scenes) */
    bool ambientLight;
    /*! \brief Flag: enable thin lens vignetting based on lens aperture diameter (see \a Projection)? */
    bool thinLensVignetting;
    /*! \brief Thin lens aperture diameter in millimeter. Only used if \a thinLensVignetting is true. */
    float thinLensApertureDiameter;
    /*! \brief Thin lens focal length in millimeter. Only used if \a thinLensVignetting is true. */
    float thinLensFocalLength;
    /*! \brief Flag: enable shot noise (also known as poisson noise) for PMD output? */
    bool shotNoise;
    /*! \brief Flag: enable Gaussian white noise for RGB output? */
    bool gaussianWhiteNoise;
    /*! \brief Gaussian noise mean; should be zero in almost all cases. Only used if \a gaussianWhiteNoise is true. */
    float gaussianWhiteNoiseMean;
    /*! \brief Gaussian noise standard deviation in (0,1]; typically in [0.01, 0.1]. For RGB simulation, this applies
     * to the color values in [0,1]. For PMD simulation, this is first stretched to the maximum per-pixel energy estimated for
     * the current simulation parameters, so that in effect you can assume the PMD energy values to also be in [0,1].
     * Only used if \a gaussianWhiteNoise is true. */
    float gaussianWhiteNoiseStddev;

    /*@}*/

    /**
     * \name Other features
     */
    /*@{*/

    /*! \brief Lens distortion computation in the vertex shader. Please read the documentation!
     * See also \a postprocLensDistortion. */
    bool preprocLensDistortion;
    /*! \brief Extra margin for early clipping for \a preprocLensDistortion. Zero by default.
     * A greater margin means that more triangles outside of the cube are rendered.
     * This decreases the amount of discarded otherwise visible triangles but increases
     * the chance of broken geometry from lens distortion. */
    float preprocLensDistortionMargin;
    /*! \brief Lens distortion computation in the fragment shader. Please read the documentation!
     * See also \a preprocLensDistortion. */
    bool postprocLensDistortion;

    /*@}*/

    /**
     * \name Shadows and light maps
     */
    /*@{*/

    /*! \brief Flag: enable shadow maps? */
    bool shadowMaps;
    /*! \brief Flag: enable filtering of shadow maps? */
    bool shadowMapFiltering;
    /*! \brief Flag: enable reflective shadow maps? */
    bool reflectiveShadowMaps;
    /*! \brief Flag: enable light source power factor maps? */
    bool lightPowerFactorMaps;

    /*@}*/

    /**
     * \name Oversampling
     */
    /*@{*/

    /*! \brief Flag: should subframes be simulated at different points in time, according to \a ChipTiming?
     * This should always be true; if it is disabled, all subframes are simulated for the timestamp
     * of the first subframe. */
    bool subFrameTemporalSampling;
    /*! \brief Samples per fragment for spatial oversampling (1x1 means no oversampling).
     * The number of samples in both horizontal and vertical direction must be odd! */
    QSize spatialSamples;
    /*! \brief Weights for spatial oversampling. If not given (vector is empty), then
     * default weights will be used (1 / number_of_samples for each sample).
     * Otherwise the vector size must match the number of samples. */
    QVector<float> spatialSampleWeights;
    /*! \brief Number of temporal samples (1 means no oversampling) */
    int temporalSamples;

    /*@}*/
};

/**
 * \brief Defines the simulator output
 */
class Output
{
public:
    /*! Constructor */
    Output();

    /**
     * \name Light-based simulation output
     */
    /*@{*/

    /*! \brief Flag: enable output of linear RGB colors? */
    bool rgb;
    /*! \brief Flag: enable output of sRGB colors (only if \a rgb is also true)? */
    bool srgb;
    /*! \brief Flag: enable output of PMD results (subframes/phase images and final result)? */
    bool pmd;
    /*! \brief Flag: enable output of PMD cartesian coordinates (only if \a pmd is also true)? */
    bool pmdCoordinates;

    /*@}*/

    /**
     * \name Geometry-related simulation output
     */
    /*@{*/

    /*! \brief Flag: enable output of eye space positions? */
    bool eyeSpacePositions;
    /*! \brief Flag: enable output of custom space positions (see \a Simulator::setCustomTransformation)? */
    bool customSpacePositions;
    /*! \brief Flag: enable output of eye space normals? */
    bool eyeSpaceNormals;
    /*! \brief Flag: enable output of custom space normals (see \a Simulator::setCustomTransformation)? */
    bool customSpaceNormals;
    /*! \brief Flag: enable output of eye space depth and range? */
    bool depthAndRange;
    /*! \brief Flag: enable output of indices (object, shape-in-object, triangle-in-shape, material)? */
    bool indices;

    /*@}*/

    /**
     * \name Temporal flow-related simulation output
     */
    /*@{*/

    /*! \brief Flag: enable output of forward 3D flow (offset from current to next 3D position)? */
    bool forwardFlow3D;
    /*! \brief Flag: enable output of forward 2D flow (offset from current to next 2D pixel position)? */
    bool forwardFlow2D;
    /*! \brief Flag: enable output of backward 3D flow (offset from current to last 3D position)? */
    bool backwardFlow3D;
    /*! \brief Flag: enable output of backward 2D flow (offset from current to last 2D pixel position)? */
    bool backwardFlow2D;

    /*@}*/
};

/**
 * \brief Provides a suitable OpenGL context for the simulator and related classes
 */
class Context {
private:
    // Our private OpenGL context handling
    QOffscreenSurface* _surface;
    QOpenGLContext* _context;

public:
    /*! \brief Constructor */
    Context(bool enableOpenGLDebugging = false);

    /*! \brief Return the OpenGL context */
    QOpenGLContext* getOpenGLContext() { return _context; }

    /*! \brief Make this context current */
    void makeCurrent();
};

/**
 * \brief Simulates a camera frame
 */
class Simulator {
private:
    // Our random number generator (e.g. for noise generation)
    std::mt19937 _randomGenerator;

    // The configuration: animated camera and scene
    Animation _cameraAnimation;
    Transformation _cameraTransformation;
    Scene _scene;

    // The configuration: chip and optics
    ChipTiming _chipTiming;
    PMD _pmd;
    Projection _projection;

    // The configuration: rendering pipeline and output
    Pipeline _pipeline;
    Output _output;
    Transformation _customTransformation;

    // Timestamp management
    bool _recreateTimestamps;
    long long _startTimestamp;
    long long _endTimestamp;
    bool _haveLastFrameTimestamp;
    long long _lastFrameTimestamp;

    // Shader pipeline management
    bool _recreateShaders;
    QOpenGLShaderProgram _shadowMapPrg;          // create shadow map for a single light source
    QOpenGLShaderProgram _reflectiveShadowMapPrg;// create reflective shadow map for a single light source
    QOpenGLShaderProgram _depthPrg;              // pre-render the depth buf only
    QOpenGLShaderProgram _lightPrg;              // simulate rgb and pmd phase for current time stamp
    QOpenGLShaderProgram _lightOversampledPrg;   // reduce spatially oversampled rgb and pmd phase
    QOpenGLShaderProgram _pmdDigNumPrg;          // convert energies to PMD digital numbers, possibly with shot noise
    QOpenGLShaderProgram _rgbResultPrg;          // combine rgb subframes to final result
    QOpenGLShaderProgram _pmdResultPrg;          // combine pmd phase subframes to final result
    QOpenGLShaderProgram _pmdCoordinatesPrg;     // compute cartesian coordinates from pmd range
    QOpenGLShaderProgram _geomPrg;               // simulate geometry (pos, normals, depth, range, indices) information
    QOpenGLShaderProgram _flowPrg;               // simulate 2D/3D flow information
    QOpenGLShaderProgram _convertToSRGBPrg;      // convert linear RGB to sRGB
    QOpenGLShaderProgram _postprocLensDistortionPrg; // postprocessing: apply lens distortion

    // Simulation output management
    bool _recreateOutput;
    QVector<long long> _timestamps;                              // subFrames
    QVector<Transformation> _cameraTransformations;              // subFrames
    QVector<QVector<Transformation>> _lightTransformations;      // subFrames; each contained vector stores one transf for each light source in the scene
    QVector<QVector<Transformation>> _objectTransformations;     // subFrames; each contained vector stores one transf for each object in the scene
    QVector<QVector<unsigned int>> _shadowMapDepthBufs;          // subFrames; each contained vector stores one cube depth buffer for each light source
    QVector<QVector<unsigned int>> _reflectiveShadowMapDepthBufs;// subFrames; each contained vector stores one cube depth buffer for each light source
    QVector<QVector<unsigned int>> _reflectiveShadowMapTexs;     // subFrames; each contained vector stores one cube array tex with 5 layers for each light source
    unsigned int _pbo;
    unsigned int _depthBufferOversampled;
    unsigned int _rgbTexOversampled;
    unsigned int _pmdEnergyTexOversampled;
    unsigned int _pmdEnergyTex;
    unsigned int _pmdCoordinatesTex;
    QVector<unsigned int> _depthBuffers;          // subFrames+1
    QVector<unsigned int> _rgbTexs;               // subFrames+1
    QVector<unsigned int> _srgbTexs;              // subFrames+1
    QVector<unsigned int> _pmdDigNumTexs;         // subFrames+1
    QVector<unsigned int> _eyeSpacePosTexs;       // subFrames
    QVector<unsigned int> _customSpacePosTexs;    // subFrames
    QVector<unsigned int> _eyeSpaceNormalTexs;    // subFrames
    QVector<unsigned int> _customSpaceNormalTexs; // subFrames
    QVector<unsigned int> _depthAndRangeTexs;     // subFrames
    QVector<unsigned int> _indicesTexs;           // subFrames
    QVector<unsigned int> _forwardFlow3DTexs;     // subFrames+1
    QVector<unsigned int> _forwardFlow2DTexs;     // subFrames+1
    QVector<unsigned int> _backwardFlow3DTexs;    // subFrames+1
    QVector<unsigned int> _backwardFlow2DTexs;    // subFrames+1
    QVector<QList<unsigned int>> _lightSimOutputTexs;
    QVector<QList<unsigned int>> _geomSimOutputTexs;
    QVector<QList<unsigned int>> _flowSimOutputTexs;
    QList<unsigned int> _oversampledLightSimOutputTexs;
    unsigned int _postProcessingTex; // temporary texture for post-processing, e.g. lens distortion

    // Our FBO
    unsigned int _fbo; // managed by prepareFBO()
    unsigned int _fullScreenQuadVao; // managed by prepareFBO()

private:
    bool spatialOversampling() const;
    QSize spatialOversamplingSize() const;
    bool temporalOversampling() const;
    bool powerTexs() const;

    void recreateTimestampsIfNecessary();
    void recreateShadersIfNecessary();
    void prepareDepthBuffers(QSize size, const QVector<unsigned int>& depthBufs);
    void prepareOutputTexs(QSize size, const QVector<unsigned int>& outputTexs, int internalFormat, bool interpolation);
    void recreateOutputIfNecessary();

    float lightIntensity(int lightSourceIndex) const;

    void simulateTimestamps(long long t);
    void simulateSampleTimestamp(long long t,
            Transformation& cameraTransformation,
            QVector<Transformation>& lightTransformations,
            QVector<Transformation>& objectTransformations);
    void prepareFBO(QSize size, unsigned int depthBuf, bool reuseDepthBufData,
            const QList<unsigned int>& colorAttachments, int cubeMapSide = -1, int arrayTextureLayers = 0,
            bool enableBlending = false, bool clearBlendingColorBuffer = true);

    void drawScene(QOpenGLShaderProgram& prg,
            const QMatrix4x4& projectionMatrix,
            const QMatrix4x4& viewMatrix,
            const QMatrix4x4& lastViewMatrix,
            const QMatrix4x4& nextViewMatrix,
            const QVector<Transformation>& objectTransformations,
            const QVector<Transformation>& lastObjectTransformations,
            const QVector<Transformation>& nextObjectTransformations);
    void simulate(QOpenGLShaderProgram& prg,
            int subFrame, long long t, long long lastT, long long nextT, unsigned int lastDepthBuf,
            const Transformation& cameraTransformation,
            const QVector<Transformation>& lightTransformations,
            const QVector<Transformation>& objectTransformations);
    void simulateShadowMap(bool reflective,
            int subFrame, int lightIndex,
            const Transformation& cameraTransformation,
            const QVector<Transformation>& lightTransformations,
            const QVector<Transformation>& objectTransformations);
    void simulateShadowMaps(int subFrame,
            const Transformation& cameraTransformation,
            const QVector<Transformation>& lightTransformations,
            const QVector<Transformation>& objectTransformations);
    void simulateDepth(int subFrame, long long t,
            const Transformation& cameraTransformation,
            const QVector<Transformation>& lightTransformations,
            const QVector<Transformation>& objectTransformations);
    void simulateLight(int subFrame, long long t,
            const Transformation& cameraTransformation,
            const QVector<Transformation>& lightTransformations,
            const QVector<Transformation>& objectTransformations);
    void simulateOversampledLight();
    void simulatePMDDigNums();
    void simulateRGBResult();
    void simulatePMDResult();
    void simulatePMDCoordinates();
    void simulateGeometry(int subFrame);
    void simulateFlow(int subFrame, long long lastT, long long nextT, unsigned int lastDepthBuf);
    void simulatePostprocLensDistortion(const QList<unsigned int>& textures);

    void convertToSRGB(int texIndex);

    bool haveValidOutput(int i) const;
    bool haveShadowMap(int lightIndex) const;
    bool haveReflectiveShadowMap(int lightIndex) const;

public:
    /*! \brief Constructor */
    Simulator();

    /**
     * \name Configuration: animation and scene
     */
    /*@{*/

    /*! \brief Get camera animation */
    const Animation& cameraAnimation() const { return _cameraAnimation; }
    /*! \brief Set camera animation */
    void setCameraAnimation(const Animation& animation);

    /*! \brief Get camera transformation (relative to the camera animation) */
    const Transformation& cameraTransformation() const { return _cameraTransformation; }
    /*! \brief Set camera transformation (relative to the camera animation) */
    void setCameraTransformation(const Transformation& transformation);

    /*! \brief Get scene */
    const Scene& scene() const { return _scene; }
    /*! \brief Set scene */
    void setScene(const Scene& scene);

    /*@}*/

    /**
     * \name Configuration: chip and optics
     */
    /*@{*/

    /*! \brief Get chip timing parameters */
    const ChipTiming& chipTiming() const { return _chipTiming; }
    /*! \brief Set chip timing parameters */
    void setChipTiming(const ChipTiming& chipTiming);

    /*! \brief Get PMD chip parameters */
    const PMD& pmd() const { return _pmd; }
    /*! \brief Set PMD chip parameters */
    void setPMD(const PMD& pmd);

    /*! \brief Get projection parameters */
    const Projection& projection() const { return _projection; }
    /*! \brief Set projection parameters */
    void setProjection(const Projection& projection);

    /*@}*/

    /**
     * \name Configuration: rendering pipeline and output
     */
    /*@{*/

    /*! \brief Get pipeline parameters */
    const Pipeline& pipeline() const { return _pipeline; }
    /*! \brief Set pipeline parameters */
    void setPipeline(const Pipeline& pipeline);

    /*! \brief Get output parameters */
    const Output& output() const { return _output; }
    /*! \brief Set output parameters */
    void setOutput(const Output& output);

    /*! \brief Set custom transformation, for output of custom space position and normals
     * (see \a Output::customSpacePositions, \a Output::customSpaceNormals).
     * By default, the custom transformation does nothing, and the custom space is
     * equivalent to world space. */
    void setCustomTransformation(const Transformation& transformation);

    /*@}*/

    /**
     * \name Simulation
     */
    /*@{*/

    /*! \brief Get the number of subframes simulated for the current configuration.
     * This is usually 4 if PMD simulation is active (see \a output), and 1 otherwise. */
    int subFrames() const;

    /*! \brief Get the start time in microseconds (earliest starting point of all involved animations) */
    long long startTimestamp();
    /*! \brief Get the end time in microseconds (latest end point of all involved animations) */
    long long endTimestamp();
    /*! \brief Get subframe duration in microseconds (depends on chip timings) */
    long long subFrameDuration();
    /*! \brief Get frame duration in microseconds (depends on chip timings and number of subframes) */
    long long frameDuration();
    /*! \brief Get frames per second rate (depends on chip timings and number of subframes) */
    float framesPerSecond();
    /*! \brief Get the next time stamp in microseconds for simulation via \a simulate(). If you did not call
     * \a simulate() yet with the current configuration, this will be the same as
     * \a startTimestamp. Otherwise, it will be the last time stamp passed to \a simulate()
     * plus the \a frameDuration(). You should stop to call \a simulate() once the returned
     * time stamp is later than \a endTimestamp(). */
    long long nextFrameTimestamp();

    /*! \brief Simulate a camera frame at the time given by \a frameTimestamp.
     *
     * This function simulates a camera viewing the scene using the current
     * configuration at the given point in time.
     *
     * It uses its own internal framebuffer object and manages its own textures
     * for simulation results. You can get these textures individually.
     *
     * If the simulation requires the simulation of subframes, then all subframes
     * and the final result frame are simulated in one step, but the results
     * from every subframe remain accessible. For example, if PMD simulation is
     * activated (see \a output), then the four phase images as well as the final
     * result are simulated in one step.
     */
    void simulate(long long frameTimestamp);

    /*@}*/

    /**
     * \name Retrieving simulation results
     *
     * Many of these function return texture handles. See the \a TexData class
     * on how to access the data within the texture conveniently, and the \a Exporter class
     * on how to write the data to various file formats.
     */
    /*@{*/

    /*! \brief Get the time stamp associated with the given subframe \a i or the final
     * result (if \a i is -1; however, this is always the same as for subframe 0). */
    long long getTimestamp(int i = -1) const;

    /*! \brief Get the texture containing the shadow map cube
     * for the light source with index \a lightIndex in the scene
     * for the given subframe \a i or the final result (if \a i is -1).
     * Shadow maps for the final result are always the same as those for the first subframe. */
    unsigned int getShadowMapCubeTex(int lightIndex, int i = -1) const;

    /*! \brief Convenience wrapper for \a getShadowMapTex().
     * You need to specify the cube side (0 = pos-x, 1 = neg-x, 2 = pos-y, 3 = neg-y, 4 = pos-z, 5 = neg-z). */
    TexData getShadowMap(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getShadowMapCubeTex(lightIndex, i), cubeSide, -1, GL_R32F, { "gldepth" }, _pbo); }

    /*! \brief Get the cube array texture containing the reflective shadow map information
     * for the light source with index \a lightIndex in the scene,
     * for the given subframe \a i or the final result (if \a i is -1).
     * Shadow maps for the final result are always the same as those for the first subframe.
     * The cube array texture contains the following layers:
     * - 0: Positions in camera space (not light space and not world space)
     * - 1: Normals in camera space
     * - 2: Radiances for r, g, b, and power
     * - 3: BRDF diffuse parameters kd_r, kd_g, kd_b
     * - 4: BRDF specular parameters ks_r, ks_g, ks_b, shininess
     */
    unsigned int getReflectiveShadowMapCubeArrayTex(int lightIndex, int i = -1) const;

    /*! \brief Convenience wrapper for \a getReflectiveShadowMapTex()
     * that gets the position data.
     * You need to specify the cube side (0 = pos-x, 1 = neg-x, 2 = pos-y, 3 = neg-y, 4 = pos-z, 5 = neg-z). */
    TexData getReflectiveShadowMapPositions(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getReflectiveShadowMapCubeArrayTex(lightIndex, i), cubeSide, 0, GL_RGB32F, { "x", "y", "z" }, _pbo); }

    /*! \brief Convenience wrapper for \a getReflectiveShadowMapTex()
     * that gets the normal data. */
    TexData getReflectiveShadowMapNormals(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getReflectiveShadowMapCubeArrayTex(lightIndex, i), cubeSide, 1, GL_RGB32F, { "nx", "ny", "nz" }, _pbo); }

    /*! \brief Convenience wrapper for \a getReflectiveShadowMapTex()
     * that gets the radiance data. */
    TexData getReflectiveShadowMapRadiances(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getReflectiveShadowMapCubeArrayTex(lightIndex, i), cubeSide, 2, GL_RGBA32F, { "r", "g", "b", "radiances" }, _pbo); }

    /*! \brief Convenience wrapper for \a getReflectiveShadowMapTex()
     * that gets the BRDF diffuse parameter data. */
    TexData getReflectiveShadowMapBRDFDiffuseParameters(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getReflectiveShadowMapCubeArrayTex(lightIndex, i), cubeSide, 3, GL_RGB32F, { "kdr", "kdg", "kdb" }, _pbo); }

    /*! \brief Convenience wrapper for \a getReflectiveShadowMapTex()
     * that gets the BRDF specular parameter data. */
    TexData getReflectiveShadowMapBRDFSpecularParameters(int lightIndex, int cubeSide, int i = -1) const
    { return TexData(getReflectiveShadowMapCubeArrayTex(lightIndex, i), cubeSide, 4, GL_RGBA32F, { "ksr", "ksg", "ksb", "shininess" }, _pbo); }

    /*! \brief Get the output texture containing OpenGL depth buffer data
     * for the given subframe \a i or the final result (if \a i is -1)
     * Depths of the final result are always the same as those for the first subframe.
     * Note that depth buffer data is only available if (1) any output other than rgb or pmd is active
     * or (2) neither spatial nor temporal oversampling is active. */
    unsigned int getDepthTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getDepthTex() */
    TexData getDepth(int i = -1) const
    { return TexData(getDepthTex(i), -1, -1, GL_R32F, { "gldepth" }, _pbo); }

    /*! \brief Get the output texture containing linear RGB colors
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getRGBTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getRGBTex() */
    TexData getRGB(int i = -1) const
    { return TexData(getRGBTex(i), -1, -1, GL_RGB32F, { "r", "g", "b" }, _pbo); }

    /*! \brief Get the output texture containing sRGB colors
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getSRGBTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getSRGBTex() */
    TexData getSRGB(int i = -1) const
    { return TexData(getSRGBTex(i), -1, -1, GL_RGB8, { "r", "g", "b" }, _pbo); }

    /*! \brief Get the output texture containing PMD simulation
     * for the given subframe \a i or the final result (if \a i is -1).
     * The subframe texture will contain a phase image in digital numbers with the
     * three channels (A-B, A+B, A, B);
     * the final result will contain simulated range, amplitude, and intensity. */
    unsigned int getPMDTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getPMDTex() */
    TexData getPMD(int i = -1) const
    {
        if (i == -1)
            return TexData(getPMDTex(i), -1, -1, GL_RGB32F, { "range", "amplitude", "intensity" }, _pbo);
        else
            return TexData(getPMDTex(i), -1, -1, GL_RGBA32F, { "a_minus_b", "a_plus_b", "a", "b" }, _pbo);
    }

    /*! \brief Get the output texture containing PMD simulated cartesian
     * coordinates. These are computed from the simulated PMD range value, taking the camera intrinsic parameters into
     * account (image size, center pixel and focal lengths). Note that lens distortion is currently ignored. */
    unsigned int getPMDCoordinatesTex() const;

    /*! \brief Convenience wrapper for \a getPMDCoordinatesTex() */
    TexData getPMDCoordinates() const
    {
        return TexData(getPMDCoordinatesTex(), -1, -1, GL_RGB32F, { "x", "y", "z" }, _pbo);
    }

    /*! \brief Get the output texture containing eye space positions
     * for the given subframe \a i or the final result (if \a i is -1).
     * Positions of the final result are always the same as those for the first subframe. */
    unsigned int getEyeSpacePositionsTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getEyeSpacePositionsTex() */
    TexData getEyeSpacePositions(int i = -1) const
    { return TexData(getEyeSpacePositionsTex(i), -1, -1, GL_RGB32F, { "x", "y", "z" }, _pbo); }

    /*! \brief Get the output texture containing custom space positions
     * for the given subframe \a i or the final result (if \a i is -1).
     * Positions of the final result are always the same as those for the first subframe. */
    unsigned int getCustomSpacePositionsTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getCustomSpacePositionsTex() */
    TexData getCustomSpacePositions(int i = -1) const
    { return TexData(getCustomSpacePositionsTex(i),-1,  -1, GL_RGB32F, { "x", "y", "z" }, _pbo); }

    /*! \brief Get the output texture containing eye space normals
     * for the given subframe \a i or the final result (if \a i is -1)
     * Normals of the final result are always the same as those for the first subframe. */
    unsigned int getEyeSpaceNormalsTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getEyeSpaceNormalsTex() */
    TexData getEyeSpaceNormals(int i = -1) const
    { return TexData(getEyeSpaceNormalsTex(i), -1, -1, GL_RGB32F, { "nx", "ny", "nz" }, _pbo); }

    /*! \brief Get the output texture containing custom space normals
     * for the given subframe \a i or the final result (if \a i is -1)
     * Normals of the final result are always the same as those for the first subframe. */
    unsigned int getCustomSpaceNormalsTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getCustomSpaceNormalsTex() */
    TexData getCustomSpaceNormals(int i = -1) const
    { return TexData(getCustomSpaceNormalsTex(i), -1, -1, GL_RGB32F, { "nx", "ny", "nz" }, _pbo); }

    /*! \brief Get the output texture containing depth and range
     * for the given subframe \a i or the final result (if \a i is -1)
     * Depth and range of the final result are always the same as those for the first subframe. */
    unsigned int getDepthAndRangeTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getDepthAndRangeTex() */
    TexData getDepthAndRange(int i = -1) const
    { return TexData(getDepthAndRangeTex(i), -1, -1, GL_RG32F, { "depth", "range" }, _pbo); }

    /*! \brief Get the output texture containing indices
     * for the given subframe \a i or the final result (if \a i is -1)
     * Indices of the final result are always the same as those for the first subframe.
     * Note that all indices are incremented by one, so that unrendered areas in the
     * resulting texture can be identified by checking for index zero (which never corresponds
     * to a rendered object). */
    unsigned int getIndicesTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getIndicesTex() */
    TexData getIndices(int i = -1) const
    { return TexData(getIndicesTex(i), -1, -1, GL_RGBA32UI, { "object_index", "shape_index", "triangle_index", "material_index" }, _pbo); }

    /*! \brief Get the output texture containing forward 3D flow
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getForwardFlow3DTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getForwardFlow3DTex() */
    TexData getForwardFlow3D(int i = -1) const
    { return TexData(getForwardFlow3DTex(i), -1, -1, GL_RGB32F, { "flow3d_x", "flow3d_y", "flow3d_z" }, _pbo); }

    /*! \brief Get the output texture containing forward 2D flow
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getForwardFlow2DTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getForwardFlow2DTex() */
    TexData getForwardFlow2D(int i = -1) const
    { return TexData(getForwardFlow2DTex(i), -1, -1, GL_RG32F, { "flow2d_x", "flow2d_y" }, _pbo); }

    /*! \brief Get the output texture containing backward 3D flow
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getBackwardFlow3DTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getBackwardFlow3DTex() */
    TexData getBackwardFlow3D(int i = -1) const
    { return TexData(getBackwardFlow3DTex(i), -1, -1, GL_RGB32F, { "flow3d_x", "flow3d_y", "flow3d_z" }, _pbo); }

    /*! \brief Get the output texture containing backward 2D flow
     * for the given subframe \a i or the final result (if \a i is -1) */
    unsigned int getBackwardFlow2DTex(int i = -1) const;

    /*! \brief Convenience wrapper for \a getBackwardFlow2DTex() */
    TexData getBackwardFlow2D(int i = -1) const
    { return TexData(getBackwardFlow2DTex(i), -1, -1, GL_RG32F, { "flow2d_x", "flow2d_y" }, _pbo); }

    /*! \brief Get the camera transformation
     * for the given subframe \a i or the final result (if \a i is -1)
     * Transformations of the final result are always the same as those for the first subframe. */
    const Transformation& getCameraTransformation(int i = -1) const;

    /*! \brief Get the transformation for the light source with index \a lightIndex in the scene
     * for the given subframe \a i or the final result (if \a i is -1)
     * Transformations of the final result are always the same as those for the first subframe. */
    const Transformation& getLightTransformation(int lightIndex, int i) const;

    /*! \brief Get the transformation for the object with index \a objectIndex in the scene
     * for the given subframe \a i or the final result (if \a i is -1)
     * Transformations of the final result are always the same as those for the first subframe. */
    const Transformation& getObjectTransformation(int objectIndex, int i) const;

    /*@}*/
};

}

#endif

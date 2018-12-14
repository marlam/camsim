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

#include <QGuiApplication>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QFile>

#include <camsim/camsim.hpp>

int main(int argc, char* argv[])
{
    /* Initialize Qt and an OpenGL context */
    QGuiApplication app(argc, argv);
    CamSim::Context context;

    /* Define a simple animated scene: at the top,
     * a quad moves from left to right through the view,
     * and at the bottom, a teapot rotates. */

    CamSim::Scene scene;
    CamSim::Generator generator;

    CamSim::Light light;
    light.type = CamSim::SpotLight;
    light.innerConeAngle = 90.0f;
    light.outerConeAngle = 90.0f;
    light.isRelativeToCamera = true;
    light.position = QVector3D(0.0f, 0.0f, 0.0f);
    light.color = QVector3D(9.0f, 9.0f, 9.0f); // for RGB simulation
    light.power = 0.2f;                        // for PMD simulation
    scene.addLight(light);

    CamSim::Material backgroundMaterial;
    backgroundMaterial.ambient = QVector3D(0.0f, 0.0f, 0.0f);
    backgroundMaterial.diffuse = QVector3D(1.0f, 1.0f, 1.0f);
    backgroundMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int backgroundMaterialIndex = scene.addMaterial(backgroundMaterial);
    CamSim::Transformation backgroundTransformation;
    backgroundTransformation.translation = QVector3D(0.0f, 0.0f, -2.0f);
    backgroundTransformation.scaling = QVector3D(5.0f, 5.0f, 5.0f);
    generator.addQuadToScene(scene, backgroundMaterialIndex, backgroundTransformation);

    CamSim::Material quadMaterial;
    quadMaterial.ambient = QVector3D(0.0f, 0.0f, 0.0f);
    quadMaterial.diffuse = QVector3D(0.7f, 0.7f, 0.7f);
    quadMaterial.specular = QVector3D(0.3f, 0.3f, 0.3f);
    int quadMaterialIndex = scene.addMaterial(quadMaterial);
    CamSim::Transformation quadAnimationTransformations[2];
    quadAnimationTransformations[0].translation = QVector3D(-1.0f, 0.5f, -1.5f);
    quadAnimationTransformations[1].translation = QVector3D(+1.0f, 0.5f, -1.5f);
    CamSim::Animation quadAnimation;
    quadAnimation.addKeyframe(      0, quadAnimationTransformations[0]);
    quadAnimation.addKeyframe(5000000, quadAnimationTransformations[1]);
    CamSim::Transformation quadTransformation;
    quadTransformation.scaling = QVector3D(0.2f, 0.2f, 0.2f);
    generator.addQuadToScene(scene, quadMaterialIndex, quadTransformation, quadAnimation);

    CamSim::Material teapotMaterial;
    teapotMaterial.isTwoSided = true; // because there is a gap between pot and lid
    teapotMaterial.ambient = QVector3D(0.0f, 0.0f, 0.0f);
    teapotMaterial.diffuse = QVector3D(0.5f, 0.5f, 0.5f);
    teapotMaterial.specular = QVector3D(1.0f, 1.0f, 1.0f);
    int teapotMaterialIndex = scene.addMaterial(teapotMaterial);
    CamSim::Transformation teapotAnimationTransformations[5];
    teapotAnimationTransformations[0].rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.5f,   0.0f);
    teapotAnimationTransformations[0].translation = QVector3D(0.0f, -0.3f, -1.0f);
    teapotAnimationTransformations[1].rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.5f,  60.0f);
    teapotAnimationTransformations[1].translation = QVector3D(0.0f, -0.3f, -1.0f);
    teapotAnimationTransformations[2].rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.5f, 120.0f);
    teapotAnimationTransformations[2].translation = QVector3D(0.0f, -0.3f, -1.0f);
    teapotAnimationTransformations[3].rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.5f, 180.0f);
    teapotAnimationTransformations[3].translation = QVector3D(0.0f, -0.3f, -1.0f);
    teapotAnimationTransformations[4].rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.5f, 240.0f);
    teapotAnimationTransformations[4].translation = QVector3D(0.0f, -0.3f, -1.0f);
    CamSim::Animation teapotAnimation;
    teapotAnimation.addKeyframe(      0, teapotAnimationTransformations[0]);
    teapotAnimation.addKeyframe(1250000, teapotAnimationTransformations[1]);
    teapotAnimation.addKeyframe(2500000, teapotAnimationTransformations[2]);
    teapotAnimation.addKeyframe(3750000, teapotAnimationTransformations[3]);
    teapotAnimation.addKeyframe(5000000, teapotAnimationTransformations[4]);
    CamSim::Transformation teapotTransformation;
    teapotTransformation.scaling = QVector3D(0.33f, 0.33f, 0.33f);
    generator.addTeapotToScene(scene, teapotMaterialIndex, teapotTransformation, teapotAnimation);

    /* Simulate */
    
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(352, 288, 70.0f);
    qInfo("Camera intrinsic parameters: cx=%g cy=%g fx=%g fy=%g",
            projection.centerPixel().x(), projection.centerPixel().y(),
            projection.focalLengths().x(), projection.focalLengths().y());

    CamSim::ChipTiming chipTiming;
    chipTiming.exposureTime = 1000e-6;         // realistic: 1000e-6
    chipTiming.readoutTime = 1000e-6;          // realistic: 1000e-6
    chipTiming.pauseTime = 0.036;              // typically used to set a target FPS

    CamSim::PMD pmd;
    pmd.pixelSize = 12.0 * 12.0;                // 12 micrometer pixel pitch
    pmd.pixelContrast = 0.75;                   // TODO: should be per-pixel to simulate different gains
    pmd.modulationFrequency = 10e6;
    pmd.wavelength = 880;
    pmd.quantumEfficiency = 0.8f;
    pmd.maxElectrons = 100000;

    CamSim::Pipeline pipeline;
    pipeline.thinLensVignetting = true;
    pipeline.thinLensApertureDiameter = 8.89f;  // 0.889 cm aperture (so that F-number is 1.8)
    pipeline.thinLensFocalLength = 16.0f;
    pipeline.spatialSamples = QSize(5, 5);
    pipeline.temporalSamples = 19;
    pipeline.shotNoise = true;

    CamSim::Output output;
    output.rgb = true;
    output.srgb = true;
    output.pmd = true;
    output.pmdCoordinates = true;
    output.depthAndRange = true;

    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setProjection(projection);
    simulator.setChipTiming(chipTiming);
    simulator.setPMD(pmd);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    CamSim::Exporter exporter;
    int frameCounter = 0;
    QFile::remove("rgb-subframes.ppm");
    QFile::remove("rgb-result.ppm");
    QFile::remove("pmd-subframes.pfs");
    QFile::remove("groundtruth-depthrange.pfs");
    QFile::remove("pmd-result.pfs");
    QFile::remove("pmd-coordinates.pfs");
    for (long long t = simulator.startTimestamp();
            t < simulator.endTimestamp();
            t = simulator.nextFrameTimestamp()) {

        qInfo("simulating for %08lld", t);
        simulator.simulate(t);

        // Each new frame is appended to the output files, so these files
        // contain all frames of the sequence. Alternatively, you could
        // export each frame to its own set of files.

        exporter.waitForAsyncExports();

        // PMD phase images
        exporter.asyncExportData("pmd-subframes.pfs",
                { simulator.getPMD(0), simulator.getPMD(1), simulator.getPMD(2), simulator.getPMD(3) });
        // RGB results for each sub frame, i.e. each PMD phase image
        exporter.asyncExportData("rgb-subframes.ppm",
                { simulator.getSRGB(0), simulator.getSRGB(1), simulator.getSRGB(2), simulator.getSRGB(3) });
        // PMD simulation result: range, amplitude, intensity
        exporter.asyncExportData("pmd-result.pfs", simulator.getPMD());
        // PMD simulation results: cartesian coordinates (computed from range)
        exporter.asyncExportData("pmd-coordinates.pfs", simulator.getPMDCoordinates());
        // RGB simulation result
        exporter.asyncExportData("rgb-result.ppm", simulator.getSRGB());
        // Grount Truth depth and range values
        exporter.asyncExportData("groundtruth-depthrange.pfs", simulator.getDepthAndRange());

        frameCounter++;
    }
    exporter.waitForAsyncExports();
}

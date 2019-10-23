/*
 * Copyright (C) 2019
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
    if (argc < 2) {
        qCritical("Usage: %s </path/to/sceneDescription.obj>", argv[0]);
        return 1;
    }

    /* Initialize Qt and an OpenGL context */
    QGuiApplication app(argc, argv);
    CamSim::Context context;

    /* Create an importer and import the model. You need a separate importer instance
     * for each file you want to import. */
    CamSim::Importer importer;
    if (!importer.import(argv[1])) {
        return 1;
    }

    /* Create our scene from the imported model. */
    CamSim::Scene scene;
    importer.addObjectToScene(scene);

    /* Add the PMD light source. */
    CamSim::Light light;
    light.type = CamSim::SpotLight;
    light.innerConeAngle = 90.0f;
    light.outerConeAngle = 90.0f;
    light.isRelativeToCamera = true;
    light.position = QVector3D(0.0f, 0.0f, 0.0f);
    light.color = QVector3D(9.0f, 9.0f, 9.0f); // for RGB simulation
    light.power = 0.2f;                        // for PMD simulation
    scene.addLight(light);

    /* Set PMD simulation parameters. */
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(320, 200, 38.4f);

    CamSim::ChipTiming chipTiming;
    chipTiming.exposureTime = 1000e-6;         // realistic: 1000e-6
    chipTiming.readoutTime = 1000e-6;          // realistic: 1000e-6
    chipTiming.pauseTime = 0.036;              // typically used to set a target FPS

    CamSim::PMD pmd;
    pmd.pixelSize = 12.0 * 12.0;                // 12 micrometer pixel pitch
    pmd.pixelContrast = 1.0f;
    pmd.modulationFrequency = 10e6;             // 15m max unique range
    pmd.wavelength = 880;
    pmd.quantumEfficiency = 0.8f;
    pmd.maxElectrons = 100000;

    CamSim::Pipeline pipeline;
    pipeline.shotNoise = true;

    CamSim::Output output;
    output.rgb = false;
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

    CamSim::Transformation cameraTransformation;
#if 0
    cameraTransformation.translation = QVector3D(1.64670f, 0.52936f, 1.03903f);
    cameraTransformation.rotation = QQuaternion::fromAxisAndAngle(QVector3D(0.0f, 1.0f, 0.0f), 46.9f);
#else
    cameraTransformation.translation = QVector3D(4.4f, 3.2f, -0.54f);
    cameraTransformation.rotation = QQuaternion::fromAxisAndAngle(QVector3D(-0.332739, 0.884304, 0.327553), 96.133f);
    cameraTransformation.translation += cameraTransformation.rotation * QVector3D(-0.15f, -0.05f, -4.0f);
#endif
    simulator.setCameraTransformation(cameraTransformation);

    simulator.simulate(0);

    /* Export results */
    
    QFile::remove("pmd-phase0.pfs");
    QFile::remove("pmd-phase1.pfs");
    QFile::remove("pmd-phase2.pfs");
    QFile::remove("pmd-phase3.pfs");
    QFile::remove("pmd-amplitude.pfs");
    QFile::remove("pmd-intensity.pfs");
    QFile::remove("pmd-distance.pfs");
    QFile::remove("pmd-coordinates.pfs");
    QFile::remove("groundtruth-distance.pfs");
    CamSim::Exporter exporter;
    //exporter.exportData("pmd-phase0.pfs", simulator.getPMD(0));
    //exporter.exportData("pmd-phase1.pfs", simulator.getPMD(1));
    //exporter.exportData("pmd-phase2.pfs", simulator.getPMD(2));
    //exporter.exportData("pmd-phase3.pfs", simulator.getPMD(3));
    exporter.exportData("pmd-amplitude.pfs", simulator.getPMD(), { 1 });
    exporter.exportData("pmd-intensity.pfs", simulator.getPMD(), { 2 });
    exporter.exportData("pmd-distance.pfs", simulator.getPMD(), { 0 });
    exporter.exportData("pmd-coordinates.pfs", simulator.getPMDCoordinates());
    exporter.exportData("groundtruth-distance.pfs", simulator.getDepthAndRange(), { 1 });

    return 0;
}

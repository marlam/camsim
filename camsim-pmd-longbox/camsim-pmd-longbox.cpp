/*
 * Copyright (C) 2018
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

#include <camsim/camsim.hpp>

int main(int argc, char* argv[])
{
    /* Initialize Qt and an OpenGL context */
    QGuiApplication app(argc, argv);
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4, 5);
    QSurfaceFormat::setDefaultFormat(format);
    QOffscreenSurface surface;
    surface.create();
    QOpenGLContext context;
    context.create();
    context.makeCurrent(&surface);

    /* Define a simple static scene: a look into a very long box.
     * In the simulated PMD results, you should see the range wraparound,
     * e.g. at 15m for a modulation frequency of 10e6 Hz.
     * The ground truth range shows the true value. */

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

    CamSim::Material boxMaterial;
    boxMaterial.isTwoSided = true; // we want to look at the inner side of the box
    boxMaterial.diffuse = QVector3D(1.0f, 1.0f, 1.0f);
    boxMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int boxMaterialIndex = scene.addMaterial(boxMaterial);
    CamSim::Transformation boxTransformation;
    boxTransformation.scaling = QVector3D(0.5f, 0.5f, 25.0f);
    generator.addCubeToScene(scene, boxMaterialIndex, boxTransformation);

    /* Simulate */
    
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(1024, 768, 50.0f);

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
    //pipeline.shotNoise = true;

    CamSim::Output output;
    output.rgb = false;
    output.pmd = true;
    output.depthAndRange = true;

    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setProjection(projection);
    simulator.setChipTiming(chipTiming);
    simulator.setPMD(pmd);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    CamSim::Transformation cameraTransformation;
    cameraTransformation.translation = QVector3D(0.0f, 0.0f, 0.5f);
    simulator.setCameraTransformation(cameraTransformation);

    simulator.simulate(0);

    /* Export results */
    
    QFile::remove("pmd-0.pfs");
    QFile::remove("pmd-1.pfs");
    QFile::remove("pmd-2.pfs");
    QFile::remove("pmd-3.pfs");
    QFile::remove("pmd-result.pfs");
    QFile::remove("depthrange.pfs");
    CamSim::Exporter exporter;
    exporter.asyncExportData("pmd-0.pfs", simulator.getPMD(0));
    exporter.asyncExportData("pmd-1.pfs", simulator.getPMD(1));
    exporter.asyncExportData("pmd-2.pfs", simulator.getPMD(2));
    exporter.asyncExportData("pmd-3.pfs", simulator.getPMD(3));
    exporter.asyncExportData("pmd-result.pfs", simulator.getPMD());
    exporter.asyncExportData("depthrange.pfs", simulator.getDepthAndRange());

    exporter.waitForAsyncExports();
}

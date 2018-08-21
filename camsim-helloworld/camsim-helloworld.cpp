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

    /* Define a simple demo scene: a torus in a box,
     * and a light source at the camera position */

    CamSim::Scene scene;
    CamSim::Generator generator;

    CamSim::Light light;
    light.type = CamSim::PointLight;
    light.isRelativeToCamera = true;
    light.position = QVector3D(0.0f, 0.0f, 0.0f);
    light.color = QVector3D(2.0f, 2.0f, 2.0f);
    light.attenuationConstant = 1.0f;
    light.attenuationLinear = 0.0f;
    light.attenuationQuadratic = 0.0f;
    scene.addLight(light);

    CamSim::Material boxMaterial;
    boxMaterial.isTwoSided = true; // we want to look at the inner side of the box
    boxMaterial.diffuse = QVector3D(0.5f, 0.5f, 0.5f);
    boxMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int boxMaterialIndex = scene.addMaterial(boxMaterial);
    CamSim::Transformation boxTransformation;
    boxTransformation.scaling = QVector3D(0.5f, 0.5f, 1.0f);
    generator.addCubeToScene(scene, boxMaterialIndex, boxTransformation);

    CamSim::Material torusMaterial;
    torusMaterial.diffuse = QVector3D(0.0f, 1.0f, 0.0f);
    torusMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int torusMaterialIndex = scene.addMaterial(torusMaterial);
    CamSim::Transformation torusTransformation;
    torusTransformation.scaling = QVector3D(0.3f, 0.3f, 0.3f);
    torusTransformation.translation = QVector3D(0.0f, 0.0f, -0.5f);
    generator.addTorusToScene(scene, torusMaterialIndex, torusTransformation);
//    generator.addArmadilloToScene(scene, torusMaterialIndex, torusTransformation);
//    generator.addBuddhaToScene(scene, torusMaterialIndex, torusTransformation);
//    generator.addBunnyToScene(scene, torusMaterialIndex, torusTransformation);
//    generator.addDragonToScene(scene, torusMaterialIndex, torusTransformation);
//    generator.addTeapotToScene(scene, torusMaterialIndex, torusTransformation);

    /* Simulate */

    CamSim::Pipeline pipeline;
    pipeline.spatialSamples = QSize(3, 3); // spatial oversampling

    CamSim::Output output;
    output.rgb = true;
    output.srgb = true;
    output.pmd = true;
    output.eyeSpacePositions = true;
    output.depthAndRange = true;

    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);
    CamSim::Transformation cameraTransformation;
    cameraTransformation.translation = QVector3D(0.0f, 0.0f, 0.5f);
    simulator.setCameraTransformation(cameraTransformation);

    simulator.simulate(0);

    /* Export results */
    
    CamSim::Exporter exporter;
    exporter.asyncExportData("rgb.png", simulator.getSRGB());
    exporter.asyncExportData("pmd-0.csv", simulator.getPMD(0));
    exporter.asyncExportData("pmd-1.csv", simulator.getPMD(1));
    exporter.asyncExportData("pmd-2.csv", simulator.getPMD(2));
    exporter.asyncExportData("pmd-3.csv", simulator.getPMD(3));
    exporter.asyncExportData("pmd-result.csv", simulator.getPMD());
    exporter.asyncExportData("positions.csv", simulator.getEyeSpacePositions());
    exporter.asyncExportData("depthrange.csv", simulator.getDepthAndRange());

    exporter.waitForAsyncExports();
}

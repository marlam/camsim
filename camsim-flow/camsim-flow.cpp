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
    CamSim::Context context;

    /* Define a simple animated scene: at the top,
     * a quad moves from left to right through the view,
     * and at the bottom, a teapot rotates. */

    CamSim::Scene scene;
    CamSim::Generator generator;

    CamSim::Light light;
    light.type = CamSim::PointLight;
    light.isRelativeToCamera = true;
    light.position = QVector3D(0.0f, 0.0f, 0.0f);
    light.color = QVector3D(1.0f, 1.0f, 1.0f);
    light.attenuationConstant = 1.0f;
    light.attenuationLinear = 0.0f;
    light.attenuationQuadratic = 0.0f;
    scene.addLight(light);

    CamSim::Material backgroundMaterial;
    backgroundMaterial.ambient = QVector3D(0.0f, 0.0f, 0.0f);
    backgroundMaterial.diffuse = QVector3D(0.25f, 0.5f, 1.0f);
    backgroundMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int backgroundMaterialIndex = scene.addMaterial(backgroundMaterial);
    CamSim::Transformation backgroundTransformation;
    backgroundTransformation.translation = QVector3D(0.0f, 0.0f, -2.0f);
    backgroundTransformation.scaling = QVector3D(5.0f, 5.0f, 5.0f);
    generator.addQuadToScene(scene, backgroundMaterialIndex, backgroundTransformation);

    CamSim::Material quadMaterial;
    quadMaterial.ambient = QVector3D(0.0f, 0.0f, 0.0f);
    quadMaterial.diffuse = QVector3D(0.66f, 1.0f, 0.33f);
    quadMaterial.specular = QVector3D(0.33f, 0.5f, 0.16f);
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
    teapotMaterial.diffuse = QVector3D(1.0f, 0.75f, 0.5f);
    teapotMaterial.specular = QVector3D(1.0f, 0.75f, 0.5f);
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
    
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(800, 600, 70.0f);

    CamSim::ChipTiming chipTiming = CamSim::ChipTiming::fromSubFramesPerSecond(5.0f);

    CamSim::Pipeline pipeline;
    pipeline.temporalSamples = 20;

    CamSim::Output output;
    output.rgb = true;
    output.srgb = true;
    output.eyeSpacePositions = true; // for the default camera this is the same as world space
    output.customSpacePositions = false;
    output.eyeSpaceNormals = true;
    output.customSpaceNormals = false;
    output.forwardFlow3D = true;
    output.forwardFlow2D = true;
    output.backwardFlow3D = true;
    output.backwardFlow2D = true;

    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setProjection(projection);
    simulator.setChipTiming(chipTiming);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    CamSim::Exporter exporter;
    int frameCounter = 0;
    for (long long t = simulator.startTimestamp();
            t < simulator.endTimestamp();
            t = simulator.nextFrameTimestamp()) {

        qInfo("simulating for %08lld", t);
        simulator.simulate(t);

        exporter.waitForAsyncExports();
        QString prefix = QString::asprintf("%04d-", frameCounter);
        //exporter.asyncExportData(prefix + "rgb.ppm", simulator.getSRGB());
        exporter.asyncExportData(prefix + "rgb.png", simulator.getSRGB());
        exporter.asyncExportData(prefix + "positions.pfs", simulator.getEyeSpacePositions());
        exporter.asyncExportData(prefix + "normals.pfs", simulator.getEyeSpaceNormals());
        exporter.asyncExportData(prefix + "forwardflow3d.pfs", simulator.getForwardFlow3D());
        exporter.asyncExportData(prefix + "forwardflow2d.pfs", simulator.getForwardFlow2D());
        exporter.asyncExportData(prefix + "backwardflow3d.pfs", simulator.getBackwardFlow3D());
        exporter.asyncExportData(prefix + "backwardflow2d.pfs", simulator.getBackwardFlow2D());

        frameCounter++;
    }
    exporter.waitForAsyncExports();
}

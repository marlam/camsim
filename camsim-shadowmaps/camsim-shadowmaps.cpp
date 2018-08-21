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
#include <QProcess>

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

    /* Define an animated shadow demo scene */

    CamSim::Scene scene;
    CamSim::Generator generator;

    CamSim::Material boxMaterial;
    boxMaterial.isTwoSided = true; // we are inside the box
    boxMaterial.diffuse = QVector3D(0.75f, 0.75f, 0.75f);
    boxMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int boxMaterialIndex = scene.addMaterial(boxMaterial);
    CamSim::Transformation boxTransformation;
    boxTransformation.scaling = QVector3D(3.0f, 3.0f, 3.0f);
    generator.addCubeToScene(scene, boxMaterialIndex, boxTransformation);

    CamSim::Material planeMaterial;
    planeMaterial.diffuse = 1.0f * QVector3D(0.5f, 0.5f, 0.75f);
    planeMaterial.specular = 0.5f * planeMaterial.diffuse;
    planeMaterial.shininess = 100.0f;
    int planeMaterialIndex = scene.addMaterial(planeMaterial);
    CamSim::Transformation planeTransformation;
    planeTransformation.rotation = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0f);
    generator.addQuadToScene(scene, planeMaterialIndex, planeTransformation);

    CamSim::Material torusMaterial;
    torusMaterial.diffuse = 1.0f * QVector3D(0.5f, 0.75f, 0.5f);
    torusMaterial.specular = 0.5f * torusMaterial.diffuse;
    torusMaterial.shininess = 100.0f;
    int torusMaterialIndex = scene.addMaterial(torusMaterial);
    CamSim::Transformation torusTransformation;
    torusTransformation.translation = QVector3D(0.3f, 0.2f, 0.0f);
    torusTransformation.scaling = QVector3D(0.45f, 0.45f, 0.45f);
    torusTransformation.rotation = QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 1.0f, 15.0f)
        * QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, 90.0f);
    generator.addTorusToScene(scene, torusMaterialIndex, torusTransformation);

    CamSim::Material sphereMaterial;
    sphereMaterial.diffuse = 0.5f * QVector3D(0.75f, 0.5f, 0.5f);
    sphereMaterial.specular = 0.5f * sphereMaterial.diffuse;
    sphereMaterial.shininess = 100.0f;
    int sphereMaterialIndex = scene.addMaterial(sphereMaterial);
    for (int i = 0; i < 4; i++) {
        CamSim::Animation sphereAnimation;
        for (int t = 0; t < 5000; t += 10) {
            float rotAngle = t * 4.0f * static_cast<float>(M_PI) / 5000;
            float y = std::cos(rotAngle + i * static_cast<float>(M_PI_2));
            float z = std::sin(rotAngle + i * static_cast<float>(M_PI_2));
            CamSim::Transformation sphereTransformation;
            sphereTransformation.translation = QVector3D(-0.4f, 0.5f, 0.0f) + QVector3D(0.0f, y * 0.3f, -z * 0.2f);
            sphereTransformation.scaling = QVector3D(0.15f, 0.15f, 0.15f);
            sphereAnimation.addKeyframe(t * 1000, sphereTransformation);
        }
        CamSim::Transformation sphereBaseTransformation;
        generator.addSphereToScene(scene, sphereMaterialIndex, sphereBaseTransformation, sphereAnimation);
    }

    CamSim::Light light;
    light.type = CamSim::PointLight;
    light.isRelativeToCamera = false;
    light.position = QVector3D(0.0f, 1.5f, 0.0f);
    light.color = QVector3D(1.0f, 1.0f, 1.0f);
    light.shadowMapSize = 512;
    light.shadowMapDepthBias = 0.05;
    CamSim::Material lightBallMaterial;
    lightBallMaterial.ambient = QVector3D(1.0f, 1.0f, 1.0f);
    lightBallMaterial.diffuse = QVector3D(0.0f, 0.0f, 0.0f);
    lightBallMaterial.specular = QVector3D(0.0f, 0.0f, 0.0f);
    int lightBallMaterialIndex = scene.addMaterial(lightBallMaterial);
    const int lightSourceCount = 2;
    light.color /= lightSourceCount / 2.0f;
    for (int i = 0; i < lightSourceCount; i++) {
        CamSim::Animation lightAnimation;
        for (int t = 0; t < 5000; t += 10) {
            float rotAngle = t * 2.0f * static_cast<float>(M_PI) / 5000;
            float x = std::cos(rotAngle + i * static_cast<float>(M_PI) * 2.0f / lightSourceCount);
            float z = std::sin(rotAngle + i * static_cast<float>(M_PI) * 2.0f / lightSourceCount);
            CamSim::Transformation lightTransformation;
            lightTransformation.translation = QVector3D(x * 1.0f, 0.0f, z * -1.0f);
            lightAnimation.addKeyframe(t * 1000, lightTransformation);
        }
        scene.addLight(light, lightAnimation);
        CamSim::Transformation lightBallBaseTransformation;
        lightBallBaseTransformation.translation = light.position;
        lightBallBaseTransformation.scaling = QVector3D(0.03f, 0.03f, 0.03f);
        generator.addSphereToScene(scene, lightBallMaterialIndex, lightBallBaseTransformation, lightAnimation);
    }

    /* Simulate */

#define RENDER_FROM_LIGHTSOURCE 0

    CamSim::Transformation cameraTransformation;
    CamSim::Animation cameraAnimation;
#if RENDER_FROM_LIGHTSOURCE
    cameraTransformation.translation = QVector3D(0.0f, 1.5f, 0.0f);
    for (int t = 0; t < 5000; t += 10) {
        float rotAngle = t * 2.0f * static_cast<float>(M_PI) / 5000;
        float x = std::cos(rotAngle);
        float z = std::sin(rotAngle);
        CamSim::Transformation cameraTransformation;
        cameraTransformation.translation = QVector3D(x * 1.0f, 0.0f, z * -1.0f);
        cameraTransformation.rotation = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -90.0f);
        cameraAnimation.addKeyframe(t * 1000, cameraTransformation);
    }
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(512, 512, 110.0f);
#else
    cameraTransformation.translation = QVector3D(0.0f, 2.5f, 3.0f);
    cameraTransformation.rotation = QQuaternion::fromAxisAndAngle(1.0f, 0.0f, 0.0f, -35.0f);
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(800, 600, 45.0f);
#endif
    CamSim::ChipTiming chipTiming = CamSim::ChipTiming::fromSubFramesPerSecond(25.0f);
    CamSim::Pipeline pipeline;
    pipeline.ambientLight = true; // for the light balls
    pipeline.shadowMaps = true;
    pipeline.shadowMapFiltering = true;
    pipeline.reflectiveShadowMaps = false; // this scene is bad for RSM because of the missing visibility test
    CamSim::Output output;
    output.rgb = true;
    output.srgb = true;
    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setCameraTransformation(cameraTransformation);
    simulator.setCameraAnimation(cameraAnimation);
    simulator.setProjection(projection);
    simulator.setChipTiming(chipTiming);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    QFile::remove("rgb.ppm");
    QFile::remove("rgb.mp4");
    QFile::remove("debug-rgb.pfs");
    QFile::remove("depth.pfs");
    QFile::remove("shadowmap0-posx.pfs");
    QFile::remove("shadowmap0-negx.pfs");
    QFile::remove("shadowmap0-posy.pfs");
    QFile::remove("shadowmap0-negy.pfs");
    QFile::remove("shadowmap0-posz.pfs");
    QFile::remove("shadowmap0-negz.pfs");
    QFile::remove("shadowmap1-posx.pfs");
    QFile::remove("shadowmap1-negx.pfs");
    QFile::remove("shadowmap1-posy.pfs");
    QFile::remove("shadowmap1-negy.pfs");
    QFile::remove("shadowmap1-posz.pfs");
    QFile::remove("shadowmap1-negz.pfs");
    int frameCounter = 0;
    for (long long t = simulator.startTimestamp();
            t < simulator.endTimestamp();
            t = simulator.nextFrameTimestamp()) {

        qInfo("simulating for %08lld", t);
        simulator.simulate(t);
        //QString prefix = QString::asprintf("%04d-", frameCounter);
        //CamSim::Exporter::exportData(prefix + "rgb.png", simulator.getSRGB());
        CamSim::Exporter::exportData("rgb.ppm", simulator.getSRGB());
        //CamSim::Exporter::exportData("debug-rgb.pfs", simulator.getRGB());
        //CamSim::Exporter::exportData("depth.pfs", simulator.getDepth());
        //CamSim::Exporter::exportData("shadowmap0-posx.pfs", simulator.getShadowMap(0, 0));
        //CamSim::Exporter::exportData("shadowmap0-negx.pfs", simulator.getShadowMap(0, 1));
        //CamSim::Exporter::exportData("shadowmap0-posy.pfs", simulator.getShadowMap(0, 2));
        //CamSim::Exporter::exportData("shadowmap0-negy.pfs", simulator.getShadowMap(0, 3));
        //CamSim::Exporter::exportData("shadowmap0-posz.pfs", simulator.getShadowMap(0, 4));
        //CamSim::Exporter::exportData("shadowmap0-negz.pfs", simulator.getShadowMap(0, 5));
        //CamSim::Exporter::exportData("shadowmap1-posx.pfs", simulator.getShadowMap(1, 0));
        //CamSim::Exporter::exportData("shadowmap1-negx.pfs", simulator.getShadowMap(1, 1));
        //CamSim::Exporter::exportData("shadowmap1-posy.pfs", simulator.getShadowMap(1, 2));
        //CamSim::Exporter::exportData("shadowmap1-negy.pfs", simulator.getShadowMap(1, 3));
        //CamSim::Exporter::exportData("shadowmap1-posz.pfs", simulator.getShadowMap(1, 4));
        //CamSim::Exporter::exportData("shadowmap1-negz.pfs", simulator.getShadowMap(1, 5));
        frameCounter++;
    }
    // create a video from the RGB frames:
    QProcess::execute("ffmpeg -i rgb.ppm rgb.mp4");
}

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

#include <cmath>

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QFile>

#include <camsim/camsim.hpp>


int main(int argc, char* argv[])
{
    /* Initialize Qt and an OpenGL context */
    QGuiApplication app(argc, argv);
    CamSim::Context context;

    /* Command line parameters */
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOptions({
            { "pointlight-array-size", "Width and height of point light array.", "odd number", "3" },
            { "shadowmap", "Use shadow maps.", "0|1", "1" },
            { "shadowmap-filtering", "Use filtering with shadow maps.", "0|1", "1" },
            { "rsm", "Use reflective shadow maps.", "0|1", "1" },
            { "file-format-float", "File format for floating point data", "suffix", "pfs" },
            { "export-rgb", "Export linear RGB result in rgb.pfs?", "0|1", "1" },
            { "export-srgb", "Export sRGB result in srgb.ppm?", "0|1", "1" },
            { "export-eye-space-positions", "Export eye space positions?", "0|1", "0" },
            { "export-eye-space-normals", "Export eye space normals?", "0|1", "0" },
            { "export-rsm", "Export the 6 RSM cube sides in rsm-*.pfs?", "0|1", "0" },
            { "export-material-indices", "Export material indices in materials.ppm?", "0|1", "0" },
            });
    parser.process(app);

    /* Create our scene. */
    CamSim::Scene scene;

    CamSim::Material baseMaterial(QVector3D(0.725f, 0.71f, 0.68f)); // "white"
    CamSim::Material leftMaterial(QVector3D(0.63f, 0.065f, 0.05f)); // "red"
    CamSim::Material rightMaterial(QVector3D(0.14f, 0.45f, 0.091f));// "green"
    int baseMaterialIndex = scene.addMaterial(baseMaterial);
    int leftMaterialIndex = scene.addMaterial(leftMaterial);
    int rightMaterialIndex = scene.addMaterial(rightMaterial);
    CamSim::Material lightMaterial(QVector3D(0.0f, 0.0f, 0.0f));
    lightMaterial.ambient = QVector3D(1.0f, 1.0f, 1.0f);
    int lightMaterialIndex = scene.addMaterial(lightMaterial);
    // left wall
    CamSim::Generator::addObjectToScene(scene, leftMaterialIndex,
            { -1.01f, 0.0f, 0.99f, -0.99f, 0.0f, -1.04f, -1.02f, 1.99f, -1.04f, -1.02f, 1.99f, 0.99f },
            { 0.9999874f, 0.005025057f, 0.0f, 0.9998379f, 0.01507292f, 0.009850611f, 0.9999874f, 0.005025057f, 0.0f, 0.9999874f, 0.005025057f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // right wall
    CamSim::Generator::addObjectToScene(scene, rightMaterialIndex,
            { 1.0f, 0.0f, -1.04f, 1.0f, 0.0f, 0.99f, 1.0f, 1.99f, 0.99f, 1.0f, 1.99f, -1.04f },
            { -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // floor
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -1.01f, 0.0f, 0.99f, 1.0f, 0.0f, 0.99f, 1.0f, 0.0f, -1.04f, -0.99f, 0.0f, -1.04f },
            { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
#if 0
    // ceiling
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -1.02f, 1.99f, 0.99f, -1.02f, 1.99f, -1.04f, 1.0f, 1.99f, -1.04f, 1.0f, 1.99f, 0.99f },
            { 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
#endif
    // back wall
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.99f, 0.0f, -1.04f, 1.0f, 0.0f, -1.04f, 1.0f, 1.99f, -1.04f, -1.02f, 1.99f, -1.04f },
            { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box left
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.05f, 0.0f, 0.57f, -0.05f, 0.6f, 0.57f, 0.13f, 0.6f, 0.0f, 0.13f, 0.0f, 0.0f },
            { -0.9535826f, 0.0f, -0.3011314f, -0.9535826f, 0.0f, -0.3011314f, -0.9535826f, 0.0f, -0.3011314f, -0.9535826f, 0.0f, -0.3011314f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box right
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.7f, 0.0f, 0.17f, 0.7f, 0.6f, 0.17f, 0.53f, 0.6f, 0.75f, 0.53f, 0.0f, 0.75f },
            { 0.9596285f, 0.0f, 0.2812705f, 0.9596285f, 0.0f, 0.2812705f, 0.9596285f, 0.0f, 0.2812705f, 0.9596285f, 0.0f, 0.2812705f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box floor
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.53f, 0.0f, 0.75f, 0.7f, 0.0f, 0.17f, 0.13f, 0.0f, 0.0f, -0.05f, 0.0f, 0.57f },
            { 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box ceiling
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.53f, 0.6f, 0.75f, 0.7f, 0.6f, 0.17f, 0.13f, 0.6f, 0.0f, -0.05f, 0.6f, 0.57f },
            { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box back
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.13f, 0.0f, 0.0f, 0.13f, 0.6f, 0.0f, 0.7f, 0.6f, 0.17f, 0.7f, 0.0f, 0.17f },
            { 0.2858051f, 0.0f, -0.9582878f, 0.2858051f, 0.0f, -0.9582878f, 0.2858051f, 0.0f, -0.9582878f, 0.2858051f, 0.0f, -0.9582878f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // short box front
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.53f, 0.0f, 0.75f, 0.53f, 0.6f, 0.75f, -0.05f, 0.6f, 0.57f, -0.05f, 0.0f, 0.57f },
            { -0.2963993f, 0.0f, 0.9550642f, -0.2963993f, 0.0f, 0.9550642f, -0.2963993f, 0.0f, 0.9550642f, -0.2963993f, 0.0f, 0.9550642f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box left
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.53f, 0.0f, 0.09f, -0.53f, 1.2f, 0.09f, -0.71f, 1.2f, -0.49f, -0.71f, 0.0f, -0.49f },
            { -0.9550642f, 0.0f, 0.2963992f, -0.9550642f, 0.0f, 0.2963992f, -0.9550642f, 0.0f, 0.2963992f, -0.9550642f, 0.0f, 0.2963992f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box right
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.14f, 0.0f, -0.67f, -0.14f, 1.2f, -0.67f, 0.04f, 1.2f, -0.09f, 0.04f, 0.0f, -0.09f },
            { 0.9550642f, 0.0f, -0.2963992f, 0.9550642f, 0.0f, -0.2963992f, 0.9550642f, 0.0f, -0.2963992f, 0.9550642f, 0.0f, -0.2963992f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box floor
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.53f, 0.0f, 0.09f, 0.04f, 0.0f, -0.09f, -0.14f, 0.0f, -0.67f, -0.71f, 0.0f, -0.49f },
            { 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box ceiling
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.53f, 1.2f, 0.09f, 0.04f, 1.2f, -0.09f, -0.14f, 1.2f, -0.67f, -0.71f, 1.2f, -0.49f },
            { 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box back
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { -0.71f, 0.0f, -0.49f, -0.71f, 1.2f, -0.49f, -0.14f, 1.2f, -0.67f, -0.14f, 0.0f, -0.67f },
            { -0.3011314f, 0.0f, -0.9535826f, -0.3011314f, 0.0f, -0.9535826f, -0.3011314f, 0.0f, -0.9535826f, -0.3011314f, 0.0f, -0.9535826f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // tall box front
    CamSim::Generator::addObjectToScene(scene, baseMaterialIndex,
            { 0.04f, 0.0f, -0.09f, 0.04f, 1.2f, -0.09f, -0.53f, 1.2f, 0.09f, -0.53f, 0.0f, 0.09f },
            { 0.3011314f, 0.0f, 0.9535826f, 0.3011314f, 0.0f, 0.9535826f, 0.3011314f, 0.0f, 0.9535826f, 0.3011314f, 0.0f, 0.9535826f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
    // light source
#if 0
    CamSim::Generator::addObjectToScene(scene, lightMaterialIndex,
            { -0.24f, 1.98f, 0.16f, -0.24f, 1.98f, -0.22f, 0.23f, 1.98f, -0.22f, 0.23f, 1.98f, 0.16f },
            { 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f },
            QVector<float>(4 * 2, 0.0f),
            { 0, 1, 2, 0, 2, 3 });
#endif

    /* Light source */
    CamSim::Light light;
    light.type = CamSim::PointLight;
    light.isRelativeToCamera = false;
    // The original area light geometry:
    //  v -0.24  1.98   0.16
    //  v -0.24  1.98  -0.22
    //  v  0.23  1.98  -0.22
    //  v  0.23  1.98   0.16
    light.position = QVector3D(-0.005f, 1.98f, -0.03f); // center of area light
    //light.direction = QVector3D(0.0f, -1.0f, 0.0f);
    //light.innerConeAngle = 15.0f;
    //light.outerConeAngle = 90.0f;
    light.color = 4.0f * QVector3D(1.0f, 1.0f, 1.0f);
    light.attenuationConstant = 0.0f;
    light.attenuationLinear = 0.0f;
    light.attenuationQuadratic = 1.0f;
    light.shadowMapSize = 2048;
    light.shadowMapDepthBias = 0.1f;
    light.reflectiveShadowMapSize = 32;
    int pointLightArraySize = parser.value("pointlight-array-size").toInt();
    for (int z = -pointLightArraySize / 2; z <= +pointLightArraySize / 2; z++) {
        for (int x = -pointLightArraySize / 2; x <= +pointLightArraySize / 2; x++) {
            CamSim::Light l = light;
            l.color = l.color / static_cast<float>(pointLightArraySize * pointLightArraySize);
            if (pointLightArraySize > 1) {
                l.position.setX(l.position.x() + x * 0.47f / (pointLightArraySize - 1));
                l.position.setZ(l.position.z() + z * 0.38f / (pointLightArraySize - 1));
            }
            scene.addLight(l);
        }
    }

    /* Set simulation parameters. Here we only create an RGB image. */
    CamSim::Transformation camTransf;
    camTransf.translation = QVector3D(0.0f, 1.0f, 3.2f);
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(800, 800, 50.0f);
    CamSim::Pipeline pipeline;
    pipeline.nearClippingPlane = 0.05f;
    pipeline.farClippingPlane = 4.5f;
    pipeline.ambientLight = true; // for the area light source
    pipeline.shadowMaps = parser.value("shadowmap").toInt();
    pipeline.shadowMapFiltering = parser.value("shadowmap-filtering").toInt();
    pipeline.reflectiveShadowMaps = parser.value("rsm").toInt();
    CamSim::Output output;
    output.rgb = parser.value("export-rgb").toInt() || parser.value("export-srgb").toInt();
    output.srgb = parser.value("export-srgb").toInt();
    output.eyeSpacePositions = parser.value("export-eye-space-positions").toInt();
    output.eyeSpaceNormals = parser.value("export-eye-space-normals").toInt();
    output.indices = parser.value("export-material-indices").toInt();

    /* Initialize the simulator */
    CamSim::Simulator simulator;
    simulator.setCameraTransformation(camTransf);
    simulator.setScene(scene);
    simulator.setProjection(projection);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    /* Simulate */
    simulator.simulate(0);

    /* Export */
    if (output.rgb && parser.value("export-rgb").toInt()) {
        QString fileName = QString("rgb.") + parser.value("file-format-float");
        QFile::remove(fileName);
        CamSim::Exporter::exportData(fileName, simulator.getRGB());
    }
    if (output.srgb && parser.value("export-srgb").toInt()) {
        QFile::remove("srgb.ppm");
        CamSim::Exporter::exportData("srgb.ppm", simulator.getSRGB());
    }
    if (output.eyeSpacePositions && parser.value("export-eye-space-positions").toInt()) {
        QString fileName = QString("eye-space-positions.") + parser.value("file-format-float");
        QFile::remove(fileName);
        CamSim::Exporter::exportData(fileName, simulator.getEyeSpacePositions());
    }
    if (output.eyeSpaceNormals && parser.value("export-eye-space-normals").toInt()) {
        QString fileName = QString("eye-space-normals.") + parser.value("file-format-float");
        QFile::remove(fileName);
        CamSim::Exporter::exportData(fileName, simulator.getEyeSpaceNormals());
    }
    if (pipeline.reflectiveShadowMaps && parser.value("export-rsm").toInt()) {
        const QString cubeSideNames[6] = { "posx", "negx", "posy", "negy", "posz", "negz" };
        int centerLight = scene.lights.size() / 2;
        QString fileName;
        for (int i = 0; i < 6; i++) {
            fileName = QString("rsm-") + cubeSideNames[i] + "-pos." + parser.value("file-format-float");
            QFile::remove(fileName);
            CamSim::Exporter::exportData(fileName, simulator.getReflectiveShadowMapPositions(centerLight, i));
            fileName = QString("rsm-") + cubeSideNames[i] + "-nrm." + parser.value("file-format-float");
            QFile::remove(fileName);
            CamSim::Exporter::exportData(fileName, simulator.getReflectiveShadowMapNormals(centerLight, i));
            fileName = QString("rsm-") + cubeSideNames[i] + "-rad." + parser.value("file-format-float");
            QFile::remove(fileName);
            CamSim::Exporter::exportData(fileName, simulator.getReflectiveShadowMapRadiances(centerLight, i));
            fileName = QString("rsm-") + cubeSideNames[i] + "-dif." + parser.value("file-format-float");
            QFile::remove(fileName);
            CamSim::Exporter::exportData(fileName, simulator.getReflectiveShadowMapBRDFDiffuseParameters(centerLight, i));
            fileName = QString("rsm-") + cubeSideNames[i] + "-spc." + parser.value("file-format-float");
            QFile::remove(fileName);
            CamSim::Exporter::exportData(fileName, simulator.getReflectiveShadowMapBRDFSpecularParameters(centerLight, i));
        }
    }
    if (output.indices && parser.value("export-material-indices").toInt()) {
        QString fileName(QString("materials.") + parser.value("file-format-float"));
        QFile::remove(fileName);
        CamSim::Exporter::exportData(fileName, simulator.getIndices(), { 3 });
    }

    return 0;
}

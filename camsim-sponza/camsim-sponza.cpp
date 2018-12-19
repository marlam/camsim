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

/* This example uses CamSim::Importer to import the Crytek Sponza model
 * available here: http://casual-effects.com/data/
 *
 * You need to build libcamsim with the ASSIMP library for this to work.
 * Also, on Debian and Ubuntu systems, please install the package
 * qt5-image-formats-plugins so that Qt can import textures in TGA format.
 */

int main(int argc, char* argv[])
{
    if (argc < 2) {
        qCritical("Usage: %s </path/to/crytek-sponza/sponza.obj>", argv[0]);
        return 1;
    }

    /* Initialize Qt and an OpenGL context */
    QGuiApplication app(argc, argv);
    CamSim::Context context;

    /* Create our scene. The model file contains no light sources, so we add our own. */
    CamSim::Scene scene;
    CamSim::Light light;
    light.type = CamSim::PointLight;
    //light.position = QVector3D(-3.5, 0.0, 0.0);
    light.color = 2.0f * QVector3D(1.0f, 1.0f, 1.0f);
    light.attenuationConstant = 1.0f;
    light.attenuationLinear = 0.0f;
    light.attenuationQuadratic = 0.0f;
    scene.addLight(light);

    /* Create an importer and import the model. You need a separate importer instance
     * for each file you want to import. */
    CamSim::Importer importer;
    if (!importer.import(argv[1])) {
        return 1;
    }

    /* Add the imported model as one object to our scene. We apply a transformation
     * matrix to bring the model to sane scale and position. */
    QMatrix4x4 M;
    M.translate(0.0f, -1.7f, 0.0f);
    //M.translate(-3.5f, -5.7f, 0.0f);
    M.rotate(90.0f, 0.0f, 1.0f, 0.0f);
    M.scale(0.01f, 0.01f, 0.01f);
    importer.setTransformationMatrix(M);
    importer.addLightsToScene(scene); // does nothing since no lights were imported
    importer.addObjectToScene(scene);
    qInfo("%s: imported into OpenGL scene", argv[1]);

    /* Set simulation parameters. Here we only create an RGB image. */    
    CamSim::Projection projection = CamSim::Projection::fromOpeningAngle(800, 600, 70.0f);
    CamSim::Pipeline pipeline;
#if 0
    projection.setDistortion(-0.1f, 0.0f, 0.0f, 0.0f);
    pipeline.preprocLensDistortion = true;
    pipeline.postprocLensDistortion = false;
#endif
#if 0
    pipeline.shadowMaps = true;
    pipeline.shadowMapFiltering = true;
    pipeline.reflectiveShadowMaps = 1;
#endif
    pipeline.transparency = true; // required to render the plants correctly
    //pipeline.ambientLight = true; // it works without, but looks ugly...
    pipeline.gaussianWhiteNoise = false; // enable for a bit of noise...
    pipeline.gaussianWhiteNoiseMean = 0.0f;
    pipeline.gaussianWhiteNoiseStddev = 0.02f;
    CamSim::Output output;
    output.rgb = true;
    output.srgb = true;

    /* Initialize the simulator */
    CamSim::Simulator simulator;
    simulator.setScene(scene);
    simulator.setProjection(projection);
    simulator.setPipeline(pipeline);
    simulator.setOutput(output);

    /* Simulate our RGB image and export it */
    simulator.simulate(0);
    CamSim::Exporter::exportData("rgb.png", simulator.getSRGB(), { 0, 1, 2 }, 9);

    return 0;
}

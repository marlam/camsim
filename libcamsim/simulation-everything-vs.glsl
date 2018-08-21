/*
 * Copyright (C) 2016, 2017, 2018
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

#version 450

uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;
uniform mat3 normal_matrix;

uniform mat4 last_modelview_matrix;
uniform mat4 last_modelview_projection_matrix;
uniform mat4 next_modelview_matrix;
uniform mat4 next_modelview_projection_matrix;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

smooth out vec3 vpos;        // position in eye space
smooth out vec3 vnormal;     // normal vector in eye space
smooth out vec2 vtexcoord;
smooth out vec3 vlastpos;
smooth out vec4 vlastprojpos;
smooth out vec3 vnextpos;
smooth out vec4 vnextprojpos;

#if $PREPROC_LENS_DISTORTION$
uniform float k1, k2, p1, p2;
uniform float fx, fy, cx, cy;
uniform int width, height;
uniform vec2 undistortedCubeCorner;
uniform float lensDistMargin;
smooth out float discardTriangle;
vec2 distort(vec4 projectedPos)
{
    vec2 ndc = projectedPos.xy / projectedPos.w;
    vec2 viewPortCoord;
    viewPortCoord.x = ndc.x * 0.5 + 0.5;
    viewPortCoord.y = 0.5 - ndc.y * 0.5;
    vec2 pixelCoord = vec2(viewPortCoord.x * width, viewPortCoord.y * height);
    float x = (pixelCoord.x - cx) / fx;
    float y = (pixelCoord.y - cy) / fy;
    float r2 = x * x + y * y;
    float r4 = r2 * r2;
    float dx = x * (1.0 + k1 * r2 + k2 * r4) + (2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x));
    float dy = y * (1.0 + k1 * r2 + k2 * r4) + (p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y);
    x = dx * fx + cx;
    y = dy * fy + cy;
    viewPortCoord = vec2(x / float(width), y / float(height));
    ndc.x = viewPortCoord.x * 2.0 - 1.0;
    ndc.y = 1.0 - viewPortCoord.y * 2.0;
    return vec2(ndc * projectedPos.w);
}
#endif

void main(void)
{
    vpos = (modelview_matrix * position).xyz;
    vnormal = normal_matrix * normal;
    vtexcoord = texcoord;
    vlastpos = (last_modelview_matrix * position).xyz;
    vlastprojpos = last_modelview_projection_matrix * position;
    vnextpos = (next_modelview_matrix * position).xyz;
    vnextprojpos = next_modelview_projection_matrix * position;

    gl_Position = modelview_projection_matrix * position;
#if $PREPROC_LENS_DISTORTION$
    vec3 ndc = gl_Position.xyz / gl_Position.w;
    discardTriangle = 0.0;
    if (any(greaterThan(abs(ndc), vec3(undistortedCubeCorner + lensDistMargin, 1.0)))
            && any(greaterThan(undistortedCubeCorner, vec2(1.0)))) { //only discard triangles when barrel distortion
        discardTriangle = 1.0;
    }
    vec2 distortedCoords = distort(gl_Position);
    gl_Position.xy = distortedCoords;
#endif
}

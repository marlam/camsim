/*
 * Copyright (C) 2017, 2018
 * Computer Graphics Group, University of Siegen
 * Written by Hendrik Sommerhoff <hendrik.sommerhoff@student.uni-siegen.de>
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

uniform sampler2D tex;
uniform float k1, k2, p1, p2;   //distortion coefficients
uniform float fx, fy, cx, cy;   //intrinsic camera parameters

layout(origin_upper_left) in vec4 gl_FragCoord;

layout(location = 0) out vec4 fcolor;

vec2 undistortPixel(float u, float v);

void main(void)
{
    float u = gl_FragCoord.x;
    float v = gl_FragCoord.y;

    vec2 texCoords = undistortPixel(u,v);
    texCoords.x = texCoords.x / float(textureSize(tex, 0).x);
    texCoords.y = 1.0 - (texCoords.y / float(textureSize(tex, 0).y));

    fcolor = texture(tex, texCoords).rgba;
}

vec2 undistortPixel(float u, float v)
{
    float x = (u - cx) / fx;
    float y = (v - cy) / fy;
    float r2 = x * x + y * y;
    float r4 = r2 * r2;

    float invFactor = 1.0 / (4.0 * k1 * r2 + 6.0 * k2 * r4 + 8.0 * p1 * y + 8.0 * p2 * x + 1.0);
    float dx = x * (k1 * r2 + k2 * r4) + 2.0 * p1 * x * y + p2 * (r2 + 2.0 * x * x);
    float dy = y * (k1 * r2 + k2 * r4) + p1 * (r2 + 2.0 * y * y) + 2.0 * p2 * x * y;
    x -= invFactor * dx;
    y -= invFactor * dy;

    return vec2(x * fx + cx, y * fy + cy);
}

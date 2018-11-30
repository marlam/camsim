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

#version 450

const float pi = 3.14159265358979323846;

uniform sampler2D pmd_result_tex;
uniform float w, h;     // width and height of image
uniform float fx, fy;   // focal lengths
uniform float cx, cy;   // center pixel

smooth in vec2 vtexcoord;

layout(location = 0) out vec3 fcoords;

void main(void)
{
    float px = vtexcoord.x * w - 0.5;         // pixel coordinate x
    float py = (1.0 - vtexcoord.y) * h - 0.5; // pixel coordinate y
    float range = texture(pmd_result_tex, vtexcoord).r;
    vec3 uvw = vec3((px - cx) / fx, (py - cy) / fy, 1.0);
    vec3 xyz = normalize(uvw) * range;
    fcoords = xyz;
}

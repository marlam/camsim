/*
 * Copyright (C) 2012, 2013, 2014, 2015, 2016, 2017, 2018
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

uniform sampler2D oversampled0;
#if $TWO_INPUTS$
uniform sampler2D oversampled1;
#endif
uniform float weights[$WEIGHTS_WIDTH$ * $WEIGHTS_HEIGHT$];

smooth in vec2 vtexcoord;

layout(location = 0) out vec3 output0;
#if $TWO_INPUTS$
layout(location = 1) out vec3 output1;
#endif

void main(void)
{
    vec3 weightedSum0 = vec3(0.0, 0.0, 0.0);
#if $TWO_INPUTS$
    vec3 weightedSum1 = vec3(0.0, 0.0, 0.0);
#endif
    
    const int oversampled_width = textureSize(oversampled0, 0).x;
    const int oversampled_height = textureSize(oversampled0, 0).y;

    for (int y = 0; y < $WEIGHTS_HEIGHT$; y++) {
        float vy = vtexcoord.y + (y - $WEIGHTS_HEIGHT$ / 2) / float(oversampled_height);
        for (int x = 0; x < $WEIGHTS_WIDTH$; x++) {
            float vx = vtexcoord.x + (x - $WEIGHTS_WIDTH$ / 2) / float(oversampled_width);

            float weight = weights[y * $WEIGHTS_WIDTH$ + x];

            vec3 value0 = texture(oversampled0, vec2(vx, vy)).rgb;
            weightedSum0 += weight * value0;
#if $TWO_INPUTS$
            vec3 value1 = texture(oversampled1, vec2(vx, vy)).rgb;
            weightedSum1 += weight * value1;
#endif
        }
    }
    output0 = weightedSum0;
#if $TWO_INPUTS$
    output1 = weightedSum1;
#endif
}

/*
 * Copyright (C) 2012, 2013, 2014, 2016, 2017, 2018
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

const float pi = 3.14159265358979323846;

const int light_type_pointlight = 0;
const int light_type_spotlight = 1;
const int light_type_dirlight = 2;

const int material_type_phong = 0;
const int material_type_microfacets = 1;
const int material_type_measured = 2;

// Render parameters
uniform int viewport_width;
uniform int viewport_height;
uniform float far_plane;                // used only with shadow maps
uniform bool thin_lens_vignetting;
uniform float frac_apdiam_foclen;       // thin lens vignetting: aperture diameter / focal length
uniform int temporal_samples;
uniform mat4 projection_matrix;
uniform mat4 custom_matrix; // includes inverted view matrix; see simulator.cpp
uniform mat3 custom_normal_matrix;
#if $SHADOW_MAPS$
uniform mat3 inverted_view_matrix;
#endif

// Light sources
uniform int light_type[$LIGHT_SOURCES$];
uniform vec3 light_position[$LIGHT_SOURCES$];
uniform vec3 light_direction[$LIGHT_SOURCES$];
uniform vec3 light_up[$LIGHT_SOURCES$];
uniform float light_inner_cone_angle[$LIGHT_SOURCES$];
uniform float light_outer_cone_angle[$LIGHT_SOURCES$];
uniform vec3 light_attenuation[$LIGHT_SOURCES$];
uniform vec3 light_color[$LIGHT_SOURCES$];
uniform float light_intensity[$LIGHT_SOURCES$]; // light source intensity in milliwatt/steradian
#if $POWER_FACTOR_MAPS$
uniform bool light_have_power_factor_tex[$LIGHT_SOURCES$];
uniform sampler2D light_power_factor_tex[$LIGHT_SOURCES$];
uniform float light_power_factor_left[$LIGHT_SOURCES$];
uniform float light_power_factor_right[$LIGHT_SOURCES$];
uniform float light_power_factor_bottom[$LIGHT_SOURCES$];
uniform float light_power_factor_top[$LIGHT_SOURCES$];
#endif
#if $SHADOW_MAPS$
uniform bool light_have_shadowmap[$LIGHT_SOURCES$];
uniform samplerCube light_shadowmap[$LIGHT_SOURCES$];
uniform float light_depth_bias[$LIGHT_SOURCES$];
#endif
#if $REFLECTIVE_SHADOW_MAPS$
uniform bool light_have_reflective_shadowmap[$LIGHT_SOURCES$];
uniform samplerCubeArray light_reflective_shadowmap[$LIGHT_SOURCES$];
uniform int light_reflective_shadowmap_hemisphere_samples_root[$LIGHT_SOURCES$];
#endif

// Object and shape indices
uniform int object_index;
uniform int shape_index;

// Material
uniform int material_index;
uniform int material_type;
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform vec3 material_emissive;
uniform float material_shininess;
uniform float material_opacity;
uniform bool material_have_ambient_tex;
uniform sampler2D material_ambient_tex;
uniform bool material_have_diffuse_tex;
uniform sampler2D material_diffuse_tex;
uniform bool material_have_specular_tex;
uniform sampler2D material_specular_tex;
uniform bool material_have_emissive_tex;
uniform sampler2D material_emissive_tex;
uniform bool material_have_shininess_tex;
uniform sampler2D material_shininess_tex;
uniform bool material_have_lightness_tex;
uniform sampler2D material_lightness_tex;
uniform bool material_have_opacity_tex;
uniform sampler2D material_opacity_tex;
uniform float material_bumpscaling;
uniform bool material_have_bump_tex;
uniform sampler2D material_bump_tex;
uniform bool material_have_normal_tex;
uniform sampler2D material_normal_tex;

// Last depth buffer (for visibility-at-last-frame)
uniform sampler2D last_depth_buf;

// Spatial oversampling
uniform float pixel_area_factor;        // fraction of pixel_area relevant for this subpixel

// for PMD:
uniform float exposure_time;            // exposure time in microseconds
uniform float pixel_area;               // area of a sensor pixel in micrometer² (assumed to be 1 for RGB)
uniform float contrast;                 // achievable demodulation contrast, in [0,1]
uniform float frac_modfreq_c;           // modulation_frequency / speed of light
uniform float tau;                      // Phase i: tau=i*pi/2

smooth in vec3 vpos;
smooth in vec3 vnormal;
smooth in vec2 vtexcoord;
smooth in vec3 vlastpos;
smooth in vec4 vlastprojpos;
smooth in vec3 vnextpos;
smooth in vec4 vnextprojpos;
#if $PREPROC_LENS_DISTORTION$
smooth in float discardTriangle;
#endif

#if $OUTPUT_RGB$
layout(location = $OUTPUT_RGB_LOCATION$) out vec3 output_rgb;
#endif
#if $OUTPUT_PMD$
layout(location = $OUTPUT_PMD_LOCATION$) out vec2 output_pmd;
#endif
#if $OUTPUT_EYE_SPACE_POSITIONS$
layout(location = $OUTPUT_EYE_SPACE_POSITIONS_LOCATION$) out vec3 output_eye_space_positions;
#endif
#if $OUTPUT_CUSTOM_SPACE_POSITIONS$
layout(location = $OUTPUT_CUSTOM_SPACE_POSITIONS_LOCATION$) out vec3 output_custom_space_positions;
#endif
#if $OUTPUT_EYE_SPACE_NORMALS$
layout(location = $OUTPUT_EYE_SPACE_NORMALS_LOCATION$) out vec3 output_eye_space_normals;
#endif
#if $OUTPUT_CUSTOM_SPACE_NORMALS$
layout(location = $OUTPUT_CUSTOM_SPACE_NORMALS_LOCATION$) out vec3 output_custom_space_normals;
#endif
#if $OUTPUT_DEPTH_AND_RANGE$
layout(location = $OUTPUT_DEPTH_AND_RANGE_LOCATION$) out vec2 output_depth_and_range;
#endif
#if $OUTPUT_INDICES$
layout(location = $OUTPUT_INDICES_LOCATION$) out uvec4 output_indices;
#endif
#if $OUTPUT_FORWARDFLOW3D$
layout(location = $OUTPUT_FORWARDFLOW3D_LOCATION$) out vec3 output_forwardflow3d;
#endif
#if $OUTPUT_FORWARDFLOW2D$
layout(location = $OUTPUT_FORWARDFLOW2D_LOCATION$) out vec2 output_forwardflow2d;
#endif
#if $OUTPUT_BACKWARDFLOW3D$
layout(location = $OUTPUT_BACKWARDFLOW3D_LOCATION$) out vec3 output_backwardflow3d;
#endif
#if $OUTPUT_BACKWARDFLOW2D$
layout(location = $OUTPUT_BACKWARDFLOW2D_LOCATION$) out vec2 output_backwardflow2d;
#endif
#if $OUTPUT_RADIANCES$
layout(location = $OUTPUT_RADIANCES_LOCATION$) out vec4 output_radiances;
#endif
#if $OUTPUT_BRDF_DIFF_PARAMS$
layout(location = $OUTPUT_BRDF_DIFF_PARAMS_LOCATION$) out vec3 output_brdf_diff_params;
#endif
#if $OUTPUT_BRDF_SPEC_PARAMS$
layout(location = $OUTPUT_BRDF_SPEC_PARAMS_LOCATION$) out vec4 output_brdf_spec_params;
#endif


#if $NORMALMAPPING$
// This computation of a cotangent frame per fragment is taken from
// http://www.thetenthplanet.de/archives/1180
mat3 cotangent_frame(vec3 N, vec3 p, vec2 uv)
{
    // get edge vectors of the pixel triangle
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    // solve the linear system
    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    // construct a scale-invariant frame
    float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
    return mat3(T * invmax, B * invmax, N);
}
#endif

vec3 brdf_modphong(vec3 l, vec3 n, vec3 v, vec3 kd, vec3 ks, float s)
{
    // Modified Phong
    vec3 r = reflect(-l, n);
    vec3 diffuse = kd / pi;
    vec3 specular = ks * (s + 2.0) / (2.0 * pi) * pow(max(dot(v, r), 0.0), max(s, 0.001));
    return diffuse + specular;
}

float average(vec3 v)
{
    return (v.r + v.g + v.b) / 3.0;
}

#if $SHADOW_MAPS$
# if $SHADOW_MAP_FILTERING$
const vec3 shadowmap_sampling_disk[20] = vec3[]
(
   vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
   vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
   vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
   vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);
# endif
#endif

#if $REFLECTIVE_SHADOW_MAPS$
// Use an equal-area mapping from unit square to unit sphere coordinates
// so that we can generate uniformly distributed points on the sphere
// from uniformly distributed points on the square.
// This uses Shirley's mapping, see Sec. "Applications" in "A Low Distortion
// Map Between Disk and Square".
vec3 to_spherecoord(float x, float y, int hemisphere)
{
    // uniform on square to uniform on disk:
    float r, phi;
    if (x * x > y * y) {
        r = x;
        phi = pi / 4.0 * y / x;
    } else {
        r = y;
        if (abs(y) > 0.0)
            phi = pi / 2.0 - pi / 4.0 * x / y;
        else
            phi = 0.0;
    }
    float u = r * cos(phi);
    float v = r * sin(phi);
    // uniform on disk to uniform on hemisphere:
    float sphere_x = u * sqrt(2 - r * r);
    float sphere_y = v * sqrt(2 - r * r);
    float sphere_z = 1 - r * r;
    if (hemisphere == 1)
        sphere_z = -sphere_z;
    return vec3(sphere_x, sphere_y, sphere_z);
}
#endif

void main(void)
{
#if $PREPROC_LENS_DISTORTION$
    if (discardTriangle > 0.0) {
        discard;
    }
#endif
    // Basic color
    vec3 ambient_color = material_ambient;
    vec3 diffuse_color = material_diffuse;
    vec3 specular_color = material_specular;
    vec3 emissive_color = material_emissive;
    float shininess = material_shininess;
    float opacity = material_opacity;
    float lightness = 1.0;
    if (material_have_lightness_tex)
        lightness = texture(material_lightness_tex, vtexcoord).r;
    // Texture color
    if (material_have_ambient_tex) {
        vec4 t = texture(material_ambient_tex, vtexcoord);
        ambient_color *= t.rgb;
        if (!material_have_diffuse_tex && !material_have_opacity_tex)
            opacity *= t.a;
    }
    if (material_have_diffuse_tex) {
        vec4 t = texture(material_diffuse_tex, vtexcoord);
        diffuse_color *= t.rgb;
        if (!material_have_opacity_tex)
            opacity *= t.a;
    }
    if (material_have_specular_tex)
        specular_color *= texture(material_specular_tex, vtexcoord).rgb;
    if (material_have_emissive_tex)
        emissive_color = texture(material_emissive_tex, vtexcoord).rgb;
    if (material_have_shininess_tex)
        shininess *= texture(material_shininess_tex, vtexcoord).r;
#if $TRANSPARENCY$
    if (material_have_opacity_tex)
        opacity *= texture(material_opacity_tex, vtexcoord).r;
    if (opacity < 0.5)
        discard;
#endif

    // Lighting vectors
    vec3 normal = normalize(vnormal);
#if $NORMALMAPPING$
    if (material_have_bump_tex || material_have_normal_tex) {
        // Matrix to transform from eye space to tangent space
        mat3 TBN = cotangent_frame(normal, vpos, vtexcoord);
        if (material_have_normal_tex) {
            // Get normal from normal map
            normal = texture(material_normal_tex, vtexcoord).rgb;
            normal = normalize(2.0 * normal - 1.0);
        } else {
            // Get normal from bump map
            float height_t = textureOffset(material_bump_tex, vtexcoord, ivec2(0, +1)).r;
            float height_b = textureOffset(material_bump_tex, vtexcoord, ivec2(0, -1)).r;
            float height_l = textureOffset(material_bump_tex, vtexcoord, ivec2(-1, 0)).r;
            float height_r = textureOffset(material_bump_tex, vtexcoord, ivec2(+1, 0)).r;
            vec3 tx = normalize(vec3(2.0, 0.0, material_bumpscaling * (height_r - height_l)));
            vec3 ty = normalize(vec3(0.0, 2.0, material_bumpscaling * (height_t - height_b)));
            normal = normalize(cross(tx, ty));
        }
        normal = TBN * normal;
    }
#endif
    if (!gl_FrontFacing)
        normal = -normal;
    vec3 view = normalize(-vpos);

    // Lighting
    vec3 surface = normalize(vpos);
    vec3 total_color = vec3(0.0, 0.0, 0.0);
    float total_energy_a = 0.0;
    float total_energy_b = 0.0;
    vec3 light_to_surface = vec3(0.0, 0.0, 0.0);
    float radiance_to_surface = 0.0;
    for (int i = 0; i < $LIGHT_SOURCES$; i++) {
        // light vector (from surface to light)
        vec3 light = light_position[i] - vpos;
        if (light_type[i] == light_type_dirlight)
            light = -light_direction[i];
        float dist_light_surface = length(light);
        light = light / dist_light_surface; // == normalize(light)
        // shadow factor
        float factor_shadow = 1.0;
#if $SHADOW_MAPS$
        if (light_have_shadowmap[i]) {
            vec3 shadow_lookup_dir_base = inverted_view_matrix * (-light);
# if $SHADOW_MAP_FILTERING$
            float shadowmap_sampling_disk_radius = (1.0 + (length(vpos) / far_plane)) / 250.0;
            float shadow_sum = 0.0;
            for (int shadow_sample = 0; shadow_sample < 20; shadow_sample++) {
                vec3 shadow_lookup_dir = shadow_lookup_dir_base
                    + shadowmap_sampling_disk[shadow_sample] * shadowmap_sampling_disk_radius;
                float shadow_closest_depth = texture(light_shadowmap[i], shadow_lookup_dir).r * far_plane;
                if (dist_light_surface - light_depth_bias[i] > shadow_closest_depth)
                    shadow_sum += 1.0;
            }
            factor_shadow = 1.0 - shadow_sum / 20.0;
# else
            float shadow_closest_depth = texture(light_shadowmap[i], shadow_lookup_dir_base).r * far_plane;
            if (dist_light_surface - light_depth_bias[i] > shadow_closest_depth)
                factor_shadow = 0.0;
# endif
        }
#endif
        // attenuation factor
        float factor_attenuation = 1.0;
        if (light_type[i] != light_type_dirlight) {
            factor_attenuation = 1.0 / (light_attenuation[i].x
                    + light_attenuation[i].y * dist_light_surface
                    + light_attenuation[i].z * dist_light_surface * dist_light_surface);
        }
        // spot light factor
        float factor_spotlight = 1.0;
        if (light_type[i] == light_type_spotlight) {
            float angle = acos(dot(normalize(light_direction[i]), -light));
            if (angle > light_outer_cone_angle[i]) {
                factor_spotlight = 0.0;
            } else if (angle > light_inner_cone_angle[i]) {
                factor_spotlight = cos(0.5 * pi * (angle - light_inner_cone_angle[i])
                        / (light_outer_cone_angle[i] - light_inner_cone_angle[i]));
            }
        }
        // power factor
        float factor_power = 1.0;
#if $POWER_FACTOR_MAPS$
        if (light_have_power_factor_tex[i]) {
            vec3 col2 = -normalize(light_direction[i]);
            vec3 col0 = cross(-col2, normalize(light_up[i]));
            vec3 col1 = cross(col0, -col2);
            vec3 p = mat3(col0, col1, col2) * vpos;
            float hangle = 0.0;
            vec3 p_xz = vec3(p.x, 0.0, p.z);
            if (dot(p_xz, p_xz) > 0.001) {
                hangle = acos(clamp(dot(normalize(p_xz), vec3(0.0, 0.0, -1.0)), 0.0, 1.0));
                if (p.x < 0.0)
                    hangle = -hangle;
            }
            float vangle = 0.0;
            vec3 p_yz = normalize(vec3(0.0, p.y, p.z));
            if (dot(p_yz, p_yz) > 0.001) {
                vangle = acos(clamp(dot(normalize(p_yz), vec3(0.0, 0.0, -1.0)), 0.0, 1.0));
                if (p.y < 0.0)
                    vangle = -vangle;
            }
            float cx = (hangle - light_power_factor_left[i]) / (light_power_factor_right[i] - light_power_factor_left[i]);
            float cy = (vangle - light_power_factor_bottom[i]) / (light_power_factor_top[i] - light_power_factor_bottom[i]);
            if (cx >= 0.0 && cx <= 1.0 && cy >= 0.0 && cy <= 1.0)
                factor_power = texture(light_power_factor_tex[i], vec2(cx, cy)).r;
            else
                factor_power = 0.0;
        }
#endif
        // combined factors
        float factor = factor_shadow * factor_attenuation * factor_spotlight * factor_power;
        // light irradiating surface from light source
        light_to_surface = factor * light_color[i];
        radiance_to_surface = factor * light_intensity[i]; // [mW/sr/m²]
        // light irradiating sensor from surface
        vec3 brdf_factors = vec3(0.0, 0.0, 0.0);
        if (material_type == material_type_phong) {
            brdf_factors = brdf_modphong(light, normal, view, diffuse_color, specular_color, shininess);
        } // TODO: other material types
        brdf_factors *= lightness; // to support legacy lightness textures; should be 1 usually
        float cos_theta_surface = max(dot(light, normal), 0.0);
        float thin_lens_factor = 1.0;
        if (thin_lens_vignetting) {
            float cos_theta_sensor = clamp(dot(surface, vec3(0.0, 0.0, -1.0)), 0.0, 1.0);
            thin_lens_factor = (pi / 4.0) * (frac_apdiam_foclen * frac_apdiam_foclen)
                * (cos_theta_sensor * cos_theta_sensor * cos_theta_sensor * cos_theta_sensor);
        }
        total_color += thin_lens_factor * brdf_factors * cos_theta_surface * light_to_surface * pixel_area_factor;
        float brdf_factor = average(brdf_factors);
        float radiance_to_sensor = brdf_factor * cos_theta_surface * radiance_to_surface; // [mW/sr/m²]
        float irradiance_sensor = thin_lens_factor * radiance_to_sensor;
        float power_sensor = irradiance_sensor * pixel_area * pixel_area_factor;
            // [(mW/m²) * (1e-6m)²] = [1e-3 * 1e-6 * 1e-6 W] = [1e-15 W] = [Femtowatt]
        const float duty_cycle = 0.5; // duty cycle of signal
        float energy = power_sensor * duty_cycle * (exposure_time / float(temporal_samples)); // [1e-15 W * 1e-6 s] = [1e-21 J] = [Zeptojoule]
        float phase_shift = 2.0 * pi * (dist_light_surface + length(vpos)) * frac_modfreq_c;
        float energy_a = energy / 2.0 * (1.0 + contrast * cos(tau + phase_shift));
        float energy_b = energy / 2.0 * (1.0 - contrast * cos(tau + phase_shift));
        total_energy_a += energy_a;
        total_energy_b += energy_b;
#if $REFLECTIVE_SHADOW_MAPS$
        if (light_have_reflective_shadowmap[i]) {
            // We generate vpl_count VPLs that are uniformly distributed on a sphere.
            const int vpl_count = 2 * light_reflective_shadowmap_hemisphere_samples_root[i]
                * light_reflective_shadowmap_hemisphere_samples_root[i];
            int eligible_vpl_count = 0;
            vec3 rsm_color = vec3(0.0);
            float rsm_energy_a = 0.0;
            float rsm_energy_b = 0.0;
            for (int rsmh = 0; rsmh < 2; rsmh++) { // RSM hemisphere
                for (int rsmy = 0; rsmy < light_reflective_shadowmap_hemisphere_samples_root[i]; rsmy++) {
                    float rsm_square_y = (float(rsmy) + 0.5)
                        / float(light_reflective_shadowmap_hemisphere_samples_root[i])
                        * 2.0 - 1.0;
                    for (int rsmx = 0; rsmx < light_reflective_shadowmap_hemisphere_samples_root[i]; rsmx++) {
                        float rsm_square_x = (float(rsmx) + 0.5)
                            / float(light_reflective_shadowmap_hemisphere_samples_root[i])
                            * 2.0 - 1.0;

                        // Cube-RSM lookup vector, uniformly distributed on sphere:
                        vec3 rsmtc = to_spherecoord(rsm_square_x, rsm_square_y, rsmh);

                        // Check if this VPL is eligible. Eligible VPLs are those that are within
                        // the horizon of our surface point.
                        vec3 vpl_pos = texture(light_reflective_shadowmap[i], vec4(rsmtc, 0)).rgb; // already in eye space!
                        vec3 vpl_view = normalize(vpos - vpl_pos);
                        if (dot(normal, -vpl_view) <= 0.0)
                            continue;
                        // Check if this VPL can actually contribute something.
                        // First check if our surface point is in its horizon,
                        // then check if it has non-zero radiance.
                        vec3 vpl_normal = texture(light_reflective_shadowmap[i], vec4(rsmtc, 1)).rgb; // already in eye space!
                        if (dot(vpl_normal, vpl_view) <= 0.0)
                            continue;
                        eligible_vpl_count++;
                        vec4 vpl_radiances = texture(light_reflective_shadowmap[i], vec4(rsmtc, 2));
                        if (all(lessThanEqual(vpl_radiances, vec4(0.0))))
                            continue;

                        // Get RSM information
                        vec3 vpl_diff_color = texture(light_reflective_shadowmap[i], vec4(rsmtc, 3)).rgb;
                        vec4 vpl_spec_params = texture(light_reflective_shadowmap[i], vec4(rsmtc, 4));
                        vec3 vpl_spec_color = vpl_spec_params.rgb;
                        float vpl_shininess = vpl_spec_params.a;

                        // Compute radiance for this VPL
                        vec3 vpl_light = normalize(light_position[i] - vpl_pos);
                        vec3 tmp_brdf_factors_q = brdf_modphong(vpl_light, vpl_normal, vpl_view,
                                vpl_diff_color, vpl_spec_color, vpl_shininess);
                        vec4 brdf_factors_q = vec4(tmp_brdf_factors_q, average(tmp_brdf_factors_q));
                        float cos_theta_q_to_l = max(dot(vpl_light, vpl_normal), 0.0);
                        vec4 radiances_q_to_p = brdf_factors_q * vpl_radiances * cos_theta_q_to_l;
                        vec3 tmp_brdf_factors_p = brdf_modphong(-vpl_view, normal, view,
                                diffuse_color, specular_color, shininess);
                        vec4 brdf_factors_p = vec4(tmp_brdf_factors_p, average(tmp_brdf_factors_p));
                        float cos_theta_p_to_q = max(dot(-vpl_view, normal), 0.0);
                        vec4 radiances_p_to_sensor = brdf_factors_p * radiances_q_to_p * cos_theta_p_to_q;

                        // Apply to RGB:
                        rsm_color += thin_lens_factor * radiances_p_to_sensor.rgb * pixel_area_factor;

                        // Apply to PMD:
                        float vpl_irradiance_sensor = thin_lens_factor * radiances_p_to_sensor.a;
                        float vpl_power_sensor = vpl_irradiance_sensor * pixel_area * pixel_area_factor;
                        float vpl_energy = vpl_power_sensor * duty_cycle * (exposure_time / float(temporal_samples));
                        float vpl_phase_shift = 2.0 * pi * (
                                length(light_position[i] - vpl_pos)
                                + length(vpl_pos - vpos)
                                + length(vpos)) * frac_modfreq_c;
                        rsm_energy_a += vpl_energy / 2.0 * (1.0 + contrast * cos(tau + vpl_phase_shift));
                        rsm_energy_b += vpl_energy / 2.0 * (1.0 - contrast * cos(tau + vpl_phase_shift));
                    }
                }
            }

            // Apply indirect illumination to RGB:
            vec3 l00 = total_color;
            vec3 l11 = rsm_color / eligible_vpl_count;
            total_color = l00 + l11;

            // Apply indirect illumination to PMD:
            total_energy_a += rsm_energy_a / eligible_vpl_count;
            total_energy_b += rsm_energy_b / eligible_vpl_count;
        }
#endif
    }

#if $OUTPUT_SHADOW_MAP_DEPTH$
    gl_FragDepth = length(vpos) / far_plane; // linear, interpolatable, comparable
#endif
#if $OUTPUT_RGB$
    output_rgb = (lightness * (emissive_color + ambient_color) + total_color) / float(temporal_samples);
#endif
#if $OUTPUT_PMD$
    output_pmd = vec2(total_energy_a, total_energy_b);
#endif
#if $OUTPUT_EYE_SPACE_POSITIONS$
    output_eye_space_positions = vpos;
#endif
#if $OUTPUT_CUSTOM_SPACE_POSITIONS$
    output_custom_space_positions = (custom_matrix * vec4(vpos, 1.0)).xyz;
#endif
#if $OUTPUT_EYE_SPACE_NORMALS$
    output_eye_space_normals = normal;
#endif
#if $OUTPUT_CUSTOM_SPACE_NORMALS$
    output_custom_space_normals = custom_normal_matrix * normal;
#endif
#if $OUTPUT_DEPTH_AND_RANGE$
    output_depth_and_range = vec2(-vpos.z, length(vpos));
#endif
#if $OUTPUT_INDICES$
    output_indices = uvec4(object_index + 1, shape_index + 1, gl_PrimitiveID + 1, material_index + 1);
#endif
#if $OUTPUT_FORWARDFLOW3D$
    output_forwardflow3d = vnextpos - vpos;
#endif
#if $OUTPUT_FORWARDFLOW2D$
    vec3 nextwinpos = vec3(
            viewport_width * (vnextprojpos.x / vnextprojpos.w + 1.0) / 2.0,
            viewport_height * (vnextprojpos.y / vnextprojpos.w + 1.0) / 2.0,
            (vnextprojpos.z / vnextprojpos.w + 1.0) / 2.0);
    output_forwardflow2d = nextwinpos.xy - gl_FragCoord.xy;
#endif
#if $OUTPUT_BACKWARDFLOW3D$
    output_backwardflow3d = vlastpos - vpos;
#endif
#if $OUTPUT_BACKWARDFLOW2D$
    vec3 lastwinpos = vec3(
            viewport_width * (vlastprojpos.x / vlastprojpos.w + 1.0) / 2.0,
            viewport_height * (vlastprojpos.y / vlastprojpos.w + 1.0) / 2.0,
            (vlastprojpos.z / vlastprojpos.w + 1.0) / 2.0);
    output_backwardflow2d = lastwinpos.xy - gl_FragCoord.xy;
#endif
    /* The following outputs are only for reflective shadow maps, so we know
     * that we have exactly one light source. */
#if $OUTPUT_RADIANCES$
    output_radiances = vec4(light_to_surface, radiance_to_surface);
#endif
#if $OUTPUT_BRDF_DIFF_PARAMS$
    output_brdf_diff_params = diffuse_color;
#endif
#if $OUTPUT_BRDF_SPEC_PARAMS$
    output_brdf_spec_params = vec4(specular_color, shininess);
#endif
}

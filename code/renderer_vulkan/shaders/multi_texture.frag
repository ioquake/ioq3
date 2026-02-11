#version 450

/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2017-2025 Artem Kharytoniuk
Copyright (C) 2019-2024 Sui Jingfeng and the vkQuake3 contributors

This file is part of the Vulkan renderer of ioquake3, ported from vkQuake3
(https://github.com/suijingfeng/vkQuake3), whose Vulkan code is a C
conversion of the Vulkan backend of Quake-III-Arena-Kenny-Edition
(https://github.com/kennyalive/Quake-III-Arena-Kenny-Edition).

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/


layout(set = 0, binding = 0) uniform sampler2D texture0;
layout(set = 1, binding = 0) uniform sampler2D texture1;

layout(location = 0) in vec4 frag_color;
layout(location = 1) in vec2 frag_tex_coord0;
layout(location = 2) in vec2 frag_tex_coord1;
layout(location = 3) in float frag_clip_dist;

layout(location = 0) out vec4 out_color;

layout (constant_id = 0) const int alpha_test_func = 0;
layout (constant_id = 1) const int color_op = 0;
layout (constant_id = 2) const int clip_plane = 0;

void main() {
    if (clip_plane != 0 && frag_clip_dist < 0.0) discard;

    vec4 color_a = frag_color * texture(texture0, frag_tex_coord0);
    vec4 color_b = texture(texture1, frag_tex_coord1);

    if (color_op != 0)
        out_color = vec4(color_a.rgb + color_b.rgb, color_a.a * color_b.a);
    else {
        out_color = color_a * color_b;
    }

    if (alpha_test_func == 1) {
        if (out_color.a == 0.0f) discard;
    } else if (alpha_test_func == 2) {
        if (out_color.a >= 0.5f) discard;
    } else if (alpha_test_func == 3) {
        if (out_color.a < 0.5f) discard;
    }
}

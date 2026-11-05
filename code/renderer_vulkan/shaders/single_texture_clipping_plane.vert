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


// 128 bytes
layout(push_constant) uniform Transform {
    mat4x4 clip_space_xform;
    // a single-precision floating-point matrix with 3 columns and 4 rows
    mat3x4 eye_space_xform;
    vec4 clipping_plane; // in eye space
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 0) out vec4 frag_color;
layout(location = 1) out vec2 frag_tex_coord;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[1];
};

void main() {
    vec4 p = vec4(in_position, 1.0);

    gl_Position = clip_space_xform * p;
    gl_ClipDistance[0] = dot(clipping_plane, vec4( p * eye_space_xform, 1.0));

    frag_color = in_color;
    frag_tex_coord = in_tex_coord;
}

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

/*
 * ==================================================================================
 *
 *       Filename:  R_PrintMat.h
 *
 *    Description:  print matrix values for debug
 *         Author:  Sui Jingfeng (), 18949883232@163.com
 * ==================================================================================
 */

#ifndef R_PRINT_MAT_H_
#define R_PRINT_MAT_H_

// data print helper
void printMat1x3f(const char* name, const float src[3]);
void printMat1x4f(const char* name, const float src[4]);
void printMat3x3f(const char* name, const float src[3][3]);
void printMat4x4f(const char* name, const float src[16]);


#endif

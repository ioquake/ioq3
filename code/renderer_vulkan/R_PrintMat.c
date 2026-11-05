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

#include <stdio.h>
#include <stdlib.h>
#include "R_PrintMat.h"
#include "ref_import.h"


/*
 * ===============================================================================
 *       Filename:  R_PrintMat.h
 *
 *    Description:  print matrix values or buffer in easier way for debug
 *         Author:  SuiJingfeng, 18949883232@163.com
 * ===============================================================================
 */

void printMat1x3f(const char* name, const float src[3])
{
    ri.Printf(PRINT_ALL, "\n float %s[3] = {%f, %f, %f};\n", 
            name, src[0], src[1], src[2]);
}

void printMat1x4f(const char* name, const float src[4])
{
    ri.Printf(PRINT_ALL, "\n float %s[4] = {%f, %f, %f, %f};\n",
            name, src[0], src[1], src[2], src[3]);
}

void printMat3x3f(const char* name, const float src[3][3])
{
    ri.Printf(PRINT_ALL, 
        "\n float %s[3][3] = {\n%f, %f, %f, \n%f, %f, %f, \n%f, %f, %f };\n", name, 
        src[0][0], src[0][1], src[0][2],
        src[1][0], src[1][1], src[1][2],
        src[2][0], src[2][1], src[2][2]);
}

void printMat4x4f(const char* name, const float src[16])
{
    ri.Printf(PRINT_ALL, "\n float %s[16] = {\n %f, %f, %f, %f,\n %f, %f, %f, %f, \n %f, %f, %f, %f, \n %f, %f, %f, %f};\n", name, 
        src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7], 
        src[8], src[9], src[10], src[11], src[12], src[13], src[14], src[15] );
}

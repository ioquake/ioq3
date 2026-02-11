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

#ifndef R_IMAGE_PROCESS_H_
#define R_IMAGE_PROCESS_H_

void GetScaledDimension(const unsigned int width, const unsigned int height, unsigned int * const outW, unsigned int * const outH, int isPicMip);

void R_SetColorMappings( void );
void R_LightScaleTexture (unsigned char* dst, unsigned char* in, unsigned int nBytes);
void ResampleTexture(unsigned char *pOut, const unsigned int inwidth, const unsigned int inheight,
                        const unsigned char *pIn, const unsigned int outwidth, const unsigned int outheight);
void R_BlendOverTexture(unsigned char* data, const uint32_t pixelCount, const uint32_t l);
//void R_GammaCorrect(unsigned char* buffer, const unsigned int Size);
void R_MipMap(const unsigned char* in, uint32_t width, uint32_t height, unsigned char* out);
void R_MipMap2(const unsigned char* in, uint32_t inWidth, uint32_t inHeight, unsigned char* out);

#endif

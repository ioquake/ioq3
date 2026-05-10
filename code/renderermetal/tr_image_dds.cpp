/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
              2015 James Canete
              2025 Metal Renderer Port

This file is part of Quake III Arena source code.

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

// DDS texture loading for the Metal renderer.
// Supports BC1-BC7 (DXT1/3/5, ATI1/ATI2) and plain RGBA8 DDS files.
// Translated from code/renderergl2/tr_image_dds.c, replacing GL format
// enums with MTL::PixelFormat equivalents.

#include "tr_texture.h"

extern "C" {
	#include "../renderercommon/tr_common.h"
}

// -------------------------------------------------------------------------
// DDS on-disk structures
// -------------------------------------------------------------------------

typedef unsigned int ui32_t;

typedef struct {
	ui32_t headerSize;
	ui32_t flags;
	ui32_t height;
	ui32_t width;
	ui32_t pitchOrFirstMipSize;
	ui32_t volumeDepth;
	ui32_t numMips;
	ui32_t reserved1[11];
	ui32_t always_0x00000020;
	ui32_t pixelFormatFlags;
	ui32_t fourCC;
	ui32_t rgbBitCount;
	ui32_t rBitMask;
	ui32_t gBitMask;
	ui32_t bBitMask;
	ui32_t aBitMask;
	ui32_t caps;
	ui32_t caps2;
	ui32_t caps3;
	ui32_t caps4;
	ui32_t reserved2;
} ddsHeader_t;

// DDS flags
#define _DDSFLAGS_REQUIRED     0x001007
#define _DDSFLAGS_PITCH        0x8
#define _DDSFLAGS_MIPMAPCOUNT  0x20000
#define _DDSFLAGS_FIRSTMIPSIZE 0x80000
#define _DDSFLAGS_VOLUMEDEPTH  0x800000

// pixelFormatFlags
#define DDSPF_ALPHAPIXELS 0x1
#define DDSPF_ALPHA       0x2
#define DDSPF_FOURCC      0x4
#define DDSPF_RGB         0x40
#define DDSPF_YUV         0x200
#define DDSPF_LUMINANCE   0x20000

// caps
#define DDSCAPS_COMPLEX  0x8
#define DDSCAPS_MIPMAP   0x400000
#define DDSCAPS_REQUIRED 0x1000

// caps2
#define DDSCAPS2_CUBEMAP 0xFE00
#define DDSCAPS2_VOLUME  0x200000

typedef struct {
	ui32_t dxgiFormat;
	ui32_t dimensions;
	ui32_t miscFlags;
	ui32_t arraySize;
	ui32_t miscFlags2;
} ddsHeaderDxt10_t;

// DXGI_FORMAT values used for format mapping
enum {
	DXGI_FORMAT_UNKNOWN              = 0,
	DXGI_FORMAT_R8G8B8A8_UNORM      = 28,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
	DXGI_FORMAT_R8G8B8A8_SNORM      = 31,
	DXGI_FORMAT_BC1_TYPELESS        = 70,
	DXGI_FORMAT_BC1_UNORM           = 71,
	DXGI_FORMAT_BC1_UNORM_SRGB      = 72,
	DXGI_FORMAT_BC2_TYPELESS        = 73,
	DXGI_FORMAT_BC2_UNORM           = 74,
	DXGI_FORMAT_BC2_UNORM_SRGB      = 75,
	DXGI_FORMAT_BC3_TYPELESS        = 76,
	DXGI_FORMAT_BC3_UNORM           = 77,
	DXGI_FORMAT_BC3_UNORM_SRGB      = 78,
	DXGI_FORMAT_BC4_TYPELESS        = 79,
	DXGI_FORMAT_BC4_UNORM           = 80,
	DXGI_FORMAT_BC4_SNORM           = 81,
	DXGI_FORMAT_BC5_TYPELESS        = 82,
	DXGI_FORMAT_BC5_UNORM           = 83,
	DXGI_FORMAT_BC5_SNORM           = 84,
	DXGI_FORMAT_BC6H_TYPELESS       = 94,
	DXGI_FORMAT_BC6H_UF16           = 95,
	DXGI_FORMAT_BC6H_SF16           = 96,
	DXGI_FORMAT_BC7_TYPELESS        = 97,
	DXGI_FORMAT_BC7_UNORM           = 98,
	DXGI_FORMAT_BC7_UNORM_SRGB      = 99,
};

#define EncodeFourCC(x) ((((ui32_t)((x)[0]))      ) | \
                         (((ui32_t)((x)[1])) << 8 ) | \
                         (((ui32_t)((x)[2])) << 16) | \
                         (((ui32_t)((x)[3])) << 24))

// -------------------------------------------------------------------------
// Public loader
// -------------------------------------------------------------------------

void R_LoadDDS_Metal(const char* filename,
                     byte**           pic,
                     int*             width,
                     int*             height,
                     MTL::PixelFormat* pixelFormat,
                     int*             numMips)
{
	union { byte* b; void* v; } buffer;
	int len;
	ddsHeader_t*      ddsHeader    = nullptr;
	ddsHeaderDxt10_t* ddsHeaderDxt10 = nullptr;
	byte*             data;

	if (!pixelFormat) {
		ri.Printf(PRINT_ERROR, "R_LoadDDS_Metal() called without pixelFormat!\n");
		return;
	}

	if (width)       *width       = 0;
	if (height)      *height      = 0;
	if (pixelFormat) *pixelFormat = MTL::PixelFormatRGBA8Unorm;
	if (numMips)     *numMips     = 1;
	*pic = nullptr;

	len = ri.FS_ReadFile((char*)filename, &buffer.v);
	if (!buffer.b || len < 0) {
		return;
	}

	if (len < 4 + (int)sizeof(*ddsHeader)) {
		ri.Printf(PRINT_ALL, "File %s is too small to be a DDS file.\n", filename);
		ri.FS_FreeFile(buffer.v);
		return;
	}

	if (*((ui32_t*)(buffer.b)) != EncodeFourCC("DDS ")) {
		ri.Printf(PRINT_ALL, "File %s is not a DDS file.\n", filename);
		ri.FS_FreeFile(buffer.v);
		return;
	}

	ddsHeader = (ddsHeader_t*)(buffer.b + 4);

	if ((ddsHeader->pixelFormatFlags & DDSPF_FOURCC) &&
	    ddsHeader->fourCC == EncodeFourCC("DX10"))
	{
		if (len < 4 + (int)sizeof(*ddsHeader) + (int)sizeof(*ddsHeaderDxt10)) {
			ri.Printf(PRINT_ALL, "File %s indicates a DX10 header it is too small to contain.\n", filename);
			ri.FS_FreeFile(buffer.v);
			return;
		}
		ddsHeaderDxt10 = (ddsHeaderDxt10_t*)(buffer.b + 4 + sizeof(ddsHeader_t));
		data = buffer.b + 4 + sizeof(*ddsHeader) + sizeof(*ddsHeaderDxt10);
		len -= 4 + (int)sizeof(*ddsHeader) + (int)sizeof(*ddsHeaderDxt10);
	}
	else
	{
		data = buffer.b + 4 + sizeof(*ddsHeader);
		len -= 4 + (int)sizeof(*ddsHeader);
	}

	if (width)  *width  = (int)ddsHeader->width;
	if (height) *height = (int)ddsHeader->height;

	if (numMips) {
		*numMips = (ddsHeader->flags & _DDSFLAGS_MIPMAPCOUNT) ?
		           (int)ddsHeader->numMips : 1;
	}

	// -----------------------------------------------------------------------
	// Map format to MTLPixelFormat
	// -----------------------------------------------------------------------
	if (ddsHeaderDxt10)
	{
		switch (ddsHeaderDxt10->dxgiFormat)
		{
			case DXGI_FORMAT_BC1_TYPELESS:
			case DXGI_FORMAT_BC1_UNORM:
				*pixelFormat = MTL::PixelFormatBC1_RGBA;
				break;
			case DXGI_FORMAT_BC1_UNORM_SRGB:
				*pixelFormat = MTL::PixelFormatBC1_RGBA_sRGB;
				break;
			case DXGI_FORMAT_BC2_TYPELESS:
			case DXGI_FORMAT_BC2_UNORM:
				*pixelFormat = MTL::PixelFormatBC2_RGBA;
				break;
			case DXGI_FORMAT_BC2_UNORM_SRGB:
				*pixelFormat = MTL::PixelFormatBC2_RGBA_sRGB;
				break;
			case DXGI_FORMAT_BC3_TYPELESS:
			case DXGI_FORMAT_BC3_UNORM:
				*pixelFormat = MTL::PixelFormatBC3_RGBA;
				break;
			case DXGI_FORMAT_BC3_UNORM_SRGB:
				*pixelFormat = MTL::PixelFormatBC3_RGBA_sRGB;
				break;
			case DXGI_FORMAT_BC4_TYPELESS:
			case DXGI_FORMAT_BC4_UNORM:
				*pixelFormat = MTL::PixelFormatBC4_RUnorm;
				break;
			case DXGI_FORMAT_BC4_SNORM:
				*pixelFormat = MTL::PixelFormatBC4_RSnorm;
				break;
			case DXGI_FORMAT_BC5_TYPELESS:
			case DXGI_FORMAT_BC5_UNORM:
				*pixelFormat = MTL::PixelFormatBC5_RGUnorm;
				break;
			case DXGI_FORMAT_BC5_SNORM:
				*pixelFormat = MTL::PixelFormatBC5_RGSnorm;
				break;
			case DXGI_FORMAT_BC6H_TYPELESS:
			case DXGI_FORMAT_BC6H_UF16:
				*pixelFormat = MTL::PixelFormatBC6H_RGBUfloat;
				break;
			case DXGI_FORMAT_BC6H_SF16:
				*pixelFormat = MTL::PixelFormatBC6H_RGBFloat;
				break;
			case DXGI_FORMAT_BC7_TYPELESS:
			case DXGI_FORMAT_BC7_UNORM:
				*pixelFormat = MTL::PixelFormatBC7_RGBAUnorm;
				break;
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				*pixelFormat = MTL::PixelFormatBC7_RGBAUnorm_sRGB;
				break;
			case DXGI_FORMAT_R8G8B8A8_UNORM:
			case DXGI_FORMAT_R8G8B8A8_SNORM:
				*pixelFormat = MTL::PixelFormatRGBA8Unorm;
				break;
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				*pixelFormat = MTL::PixelFormatRGBA8Unorm_sRGB;
				break;
			default:
				ri.Printf(PRINT_ALL, "DDS file %s has unsupported DXGI format %u.\n",
				          filename, ddsHeaderDxt10->dxgiFormat);
				ri.FS_FreeFile(buffer.v);
				return;
		}
	}
	else if (ddsHeader->pixelFormatFlags & DDSPF_FOURCC)
	{
		ui32_t fcc = ddsHeader->fourCC;
		if      (fcc == EncodeFourCC("DXT1")) *pixelFormat = MTL::PixelFormatBC1_RGBA;
		else if (fcc == EncodeFourCC("DXT2")) *pixelFormat = MTL::PixelFormatBC2_RGBA;
		else if (fcc == EncodeFourCC("DXT3")) *pixelFormat = MTL::PixelFormatBC2_RGBA;
		else if (fcc == EncodeFourCC("DXT4")) *pixelFormat = MTL::PixelFormatBC3_RGBA;
		else if (fcc == EncodeFourCC("DXT5")) *pixelFormat = MTL::PixelFormatBC3_RGBA;
		else if (fcc == EncodeFourCC("ATI1")) *pixelFormat = MTL::PixelFormatBC4_RUnorm;
		else if (fcc == EncodeFourCC("BC4U")) *pixelFormat = MTL::PixelFormatBC4_RUnorm;
		else if (fcc == EncodeFourCC("BC4S")) *pixelFormat = MTL::PixelFormatBC4_RSnorm;
		else if (fcc == EncodeFourCC("ATI2")) *pixelFormat = MTL::PixelFormatBC5_RGUnorm;
		else if (fcc == EncodeFourCC("BC5U")) *pixelFormat = MTL::PixelFormatBC5_RGUnorm;
		else if (fcc == EncodeFourCC("BC5S")) *pixelFormat = MTL::PixelFormatBC5_RGSnorm;
		else {
			ri.Printf(PRINT_ALL, "DDS file %s has unsupported FourCC.\n", filename);
			ri.FS_FreeFile(buffer.v);
			return;
		}
	}
	else if (ddsHeader->pixelFormatFlags == (DDSPF_RGB | DDSPF_ALPHAPIXELS)
	         && ddsHeader->rgbBitCount == 32
	         && ddsHeader->rBitMask == 0x000000ff
	         && ddsHeader->gBitMask == 0x0000ff00
	         && ddsHeader->bBitMask == 0x00ff0000
	         && ddsHeader->aBitMask == 0xff000000)
	{
		*pixelFormat = MTL::PixelFormatRGBA8Unorm;
	}
	else
	{
		ri.Printf(PRINT_ALL, "DDS file %s has an unsupported uncompressed format.\n", filename);
		ri.FS_FreeFile(buffer.v);
		return;
	}

	*pic = (byte*)ri.Malloc(len);
	Com_Memcpy(*pic, data, len);
	ri.FS_FreeFile(buffer.v);
}

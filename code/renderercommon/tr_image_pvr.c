/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

#include "tr_common.h"

typedef struct gbix {
	unsigned int magic;
	unsigned int len;
} gbix_t;

typedef struct pvr {
	unsigned int magic;
	unsigned int len_file;
	unsigned int type;
	unsigned short width;
	unsigned short height;
} pvr_t;

typedef unsigned int (*pvr_pixel_func_t)(unsigned short color);
typedef byte *(*pvr_image_func_t)(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func);

enum {
	PVR_PIXEL_TYPE_ARGB1555 = 0,
	PVR_PIXEL_TYPE_RGB565 = 1,
	PVR_PIXEL_TYPE_ARGB4444 = 2
};

enum {
	PVR_IMAGE_TYPE_TWIDDLED = 1,
	PVR_IMAGE_TYPE_TWIDDLED_MM = 2,
	PVR_IMAGE_TYPE_VQ = 3,
	PVR_IMAGE_TYPE_VQ_MM = 4,
	PVR_IMAGE_TYPE_RECTANGULAR = 9,
	PVR_IMAGE_TYPE_RECTANGULAR_MM = 10
};

static int log2approx(int x)
{
	switch (x)
	{
		case 8: return 3;
		case 16: return 4;
		case 32: return 5;
		case 64: return 6;
		case 128: return 7;
		case 256: return 8;
		case 512: return 9;
		case 1024: return 10;
		default: return -1;
	}
}

static int pvr_detwiddle(int x, int y, int w, int h)
{
	int wmax, hmax;
	int i, idx = 0;

	wmax = log2approx(w);
	hmax = log2approx(h);

	if (wmax < 0 || hmax < 0)
		return -1;

	for (i = 0; i < 10; i++)
	{
		if (i < wmax && i < hmax)
		{
			idx |= ((y >> i) & 1) << (i * 2 + 0);
			idx |= ((x >> i) & 1) << (i * 2 + 1);
		}
		else if (i < wmax)
		{
			idx |= ((x >> i) & 1) << (i + hmax);
		}
		else if (i < hmax)
		{
			idx |= ((y >> i) & 1) << (i + wmax);
		}
		else
		{
			break;
		}
	}

	return idx;
}

static unsigned int argb1555_to_rgba8888(unsigned short color)
{
	unsigned char r, g, b, a;
	r = ((color >> 10) & 31) << 4;
	g = ((color >> 5) & 31) << 4;
	b = ((color >> 0) & 31) << 4;
	a = ((color >> 15) & 1) * 255;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static unsigned int rgb565_to_rgba8888(unsigned short color)
{
	unsigned char r, g, b, a;
	r = ((color >> 11) & 31) << 3;
	g = ((color >> 5) & 63) << 2;
	b = ((color >> 0) & 31) << 3;
	a = 255;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static unsigned int argb4444_to_rgba8888(unsigned short color)
{
	unsigned char r, g, b, a;
	r = ((color >> 8) & 15) << 4;
	g = ((color >> 4) & 15) << 4;
	b = ((color >> 0) & 15) << 4;
	a = ((color >> 12) & 15) | 0xF;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static int mm_offset(int w)
{
	switch (w)
	{
		case 1: return 0x00006;
		case 2: return 0x00008;
		case 4: return 0x00010;
		case 8: return 0x00030;
		case 16: return 0x000B0;
		case 32: return 0x002B0;
		case 64: return 0x00AB0;
		case 128: return 0x02AB0;
		case 256: return 0x0AAB0;
		case 512: return 0x2AAB0;
		case 1024: return 0xAAAB0;
		default: return -1;
	}
}

static int mm_offset_vq(int w)
{
	switch (w)
	{
		case 1: return 0x00000;
		case 2: return 0x00001;
		case 4: return 0x00002;
		case 8: return 0x00006;
		case 16: return 0x00016;
		case 32: return 0x00056;
		case 64: return 0x00156;
		case 128: return 0x00556;
		case 256: return 0x01556;
		case 512: return 0x05556;
		case 1024: return 0x15556;
		default: return -1;
	}
}

static byte *decode_mm(pvr_t *pvr, pvr_image_func_t image_func, pvr_pixel_func_t pixel_func, qboolean vq, qboolean detwiddle)
{
	int offset = vq ? mm_offset_vq(pvr->width) : mm_offset(pvr->width);
	return image_func(pvr, offset, detwiddle, pixel_func);
}

static byte *decode(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func)
{
	int x, y;
	unsigned int *rgba32;
	byte *ret;

	ret = (byte *)ri.Malloc(pvr->width * pvr->height * sizeof(unsigned int));
	rgba32 = (unsigned int *)ret;

	for (y = 0; y < pvr->height; y++)
	{
		for (x = 0; x < pvr->width; x++)
		{
			unsigned short color;
			int ofs;
			if (detwiddle)
				ofs = offset + pvr_detwiddle(x, y, pvr->width, pvr->height) * 2;
			else
				ofs = offset + (y * pvr->width + x) * 2;

			if (ofs < 0)
			{
				ri.Free(ret);
				return NULL;
			}

			color = *(unsigned short *)(((byte *)(pvr + 1)) + ofs);

			rgba32[y * pvr->width + x] = pixel_func(color);
		}
	}

	return ret;
}

static byte *decode_vq(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func)
{
	int x, y;
	unsigned int *rgba32;
	unsigned short *codebook;
	unsigned char *indices;
	byte *ret;

	ret = (byte *)ri.Malloc(pvr->width * pvr->height * (pixel_func ? sizeof(unsigned int) : sizeof(unsigned short)));
	rgba32 = (unsigned int *)ret;

	codebook = (unsigned short *)(pvr + 1);
	indices = ((unsigned char *)(codebook + 1024)) + offset;

	for (y = 0; y < pvr->height / 2; y++)
	{
		for (x = 0; x < pvr->width / 2; x++)
		{
			unsigned short *colors;
			int a, b, c, d;
			int idx;
			if (detwiddle)
				idx = pvr_detwiddle(x, y, pvr->width, pvr->height);
			else
				idx = y * (pvr->width / 2) + x;

			if (idx < 0)
			{
				ri.Free(ret);
				return NULL;
			}

			colors = &codebook[indices[idx] * 4];

			a = ((y * 2) + 0) * pvr->width + ((x * 2) + 0);
			b = ((y * 2) + 1) * pvr->width + ((x * 2) + 0);
			c = ((y * 2) + 0) * pvr->width + ((x * 2) + 1);
			d = ((y * 2) + 1) * pvr->width + ((x * 2) + 1);

			rgba32[a] = pixel_func(colors[0]);
			rgba32[b] = pixel_func(colors[1]);
			rgba32[c] = pixel_func(colors[2]);
			rgba32[d] = pixel_func(colors[3]);
		}
	}

	return ret;
}

void R_LoadPVR(const char *name, byte **pic, int *width, int *height)
{
	unsigned int length;
	void *buffer;
	byte *ptr;
	pvr_t *pvr;
	int pixel_type, image_type;
	pvr_pixel_func_t pixel_func;
	byte *ret = NULL;

	*pic = NULL;
	if (width)
		*width = 0;
	if (height)
		*height = 0;

	// load the file
	length = ri.FS_ReadFile((char *)name, &buffer);
	if (!buffer || length < 0)
		return;

	ptr = (byte *)buffer;

	// skip global index
	if (memcmp(ptr, "GBIX", 4) == 0)
	{
		gbix_t *gbix = (gbix_t *)ptr;
		ptr += sizeof(gbix_t) + LittleLong(gbix->len);
	}

	// check magic identifier
	if (memcmp(ptr, "PVRT", 4) != 0)
		ri.Error(ERR_DROP, "LoadPVR: magic identifier does not match expected (%s)", name);

	// fix up header
	pvr = (pvr_t *)ptr;
	pvr->len_file = LittleLong(pvr->len_file);
	pvr->type = LittleLong(pvr->type);
	pvr->width = LittleShort(pvr->width);
	pvr->height = LittleShort(pvr->height);

	// break out type values
	pixel_type = pvr->type & 0xFF;
	image_type = (pvr->type & 0xFF00) >> 8;

	// get pixel function
	switch (pixel_type)
	{
		case PVR_PIXEL_TYPE_ARGB1555:
		{
			pixel_func = argb1555_to_rgba8888;
			break;
		}
		case PVR_PIXEL_TYPE_RGB565:
		{
			pixel_func = rgb565_to_rgba8888;
			break;
		}
		case PVR_PIXEL_TYPE_ARGB4444:
		{
			pixel_func = argb4444_to_rgba8888;
			break;
		}
		default:
		{
			ri.Error(ERR_DROP, "LoadPVR: unsupported pixel type 0x%02x (%s)", pixel_type, name);
			break;
		}
	}

	// decompress image
	switch (image_type)
	{
		case PVR_IMAGE_TYPE_TWIDDLED:
		{
			ret = decode(pvr, 0, qtrue, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_TWIDDLED_MM:
		{
			ret = decode_mm(pvr, decode, pixel_func, qfalse, qtrue);
			break;
		}
		case PVR_IMAGE_TYPE_VQ:
		{
			ret = decode_vq(pvr, 0, qtrue, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_VQ_MM:
		{
			ret = decode_mm(pvr, decode_vq, pixel_func, qtrue, qtrue);
			break;
		}
		case PVR_IMAGE_TYPE_RECTANGULAR:
		{
			ret = decode(pvr, 0, qfalse, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_RECTANGULAR_MM:
		{
			ret = decode_mm(pvr, decode, pixel_func, qfalse, qfalse);
			break;
		}
		default:
		{
			ri.Error(ERR_DROP, "LoadPVR: unsupported image type 0x%02x (%s)", image_type, name);
			break;
		}
	}

	// clean up
	ri.FS_FreeFile(buffer);

	// something failed
	if (!ret)
		return;

	// return stuff
	*pic = ret;
	if (width)
		*width = pvr->width;
	if (height)
		*height = pvr->height;
}

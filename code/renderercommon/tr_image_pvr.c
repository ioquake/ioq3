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

typedef struct pvr {
	unsigned char magic[4];
	unsigned int len_file;
	unsigned int type;
	unsigned short width;
	unsigned short height;
	unsigned short codebook[1024];
	unsigned char indices[1];
} pvr_t;

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

static int from_xy(int x, int y, int w, int h)
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

static unsigned int rgb565_to_rgba24(unsigned short color)
{
	unsigned char r, g, b, a;
	r = ((color >> 11) & 31) << 3;
	g = ((color >> 5) & 63) << 2;
	b = ((color >> 0) & 31) << 3;
	a = 255;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

void R_LoadPVR(const char *name, byte **pic, int *width, int *height)
{
	unsigned int length;
	void *buffer;
	pvr_t *pvr;
	unsigned int *rgba;
	int x, y;

	*pic = NULL;
	if (width)
		*width = 0;
	if (height)
		*height = 0;

	// load the file
	length = ri.FS_ReadFile((char *)name, &buffer);
	if (!buffer || length < 0)
		return;

	// check magic identifier
	pvr = (pvr_t *)buffer;
	if (memcmp(pvr->magic, "PVRT", sizeof(pvr->magic)) != 0)
		ri.Error(ERR_DROP, "LoadPVR: magic identifier does not match expected (%s)", name);

	// fix up header
	pvr->len_file = LittleLong(pvr->len_file);
	pvr->type = LittleLong(pvr->type);
	pvr->width = LittleShort(pvr->width);
	pvr->height = LittleShort(pvr->height);

	// allocate rgba buffer
	rgba = (unsigned int *)ri.Malloc(pvr->width * pvr->height * 4);

	// decompress image
	for (y = 0; y < pvr->height / 2; y++)
	{
		for (x = 0; x < pvr->width / 2; x++)
		{
			unsigned short *colors;
			int a, b, c, d;
			int idx = from_xy(x, y, pvr->width, pvr->height);

			if (idx < 0)
				ri.Error(ERR_DROP, "LoadPVR: invalid data passed to decompressor (%s)", name);

			colors = &pvr->codebook[pvr->indices[idx] * 4];

			a = ((y * 2) + 0) * pvr->width + ((x * 2) + 0);
			b = ((y * 2) + 1) * pvr->width + ((x * 2) + 0);
			c = ((y * 2) + 0) * pvr->width + ((x * 2) + 1);
			d = ((y * 2) + 1) * pvr->width + ((x * 2) + 1);

			rgba[a] = rgb565_to_rgba24(colors[0]);
			rgba[b] = rgb565_to_rgba24(colors[1]);
			rgba[c] = rgb565_to_rgba24(colors[2]);
			rgba[d] = rgb565_to_rgba24(colors[3]);
		}
	}

	// clean up
	ri.FS_FreeFile(buffer);

	// return stuff
	*pic = (byte *)rgba;
	if (width)
		*width = pvr->width;
	if (height)
		*height = pvr->height;
}

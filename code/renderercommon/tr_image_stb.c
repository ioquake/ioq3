/*
===========================================================================
Copyright (C) 2017 James Canete

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

// malloc/realloc_sized/free for stb libs
// FIXME: Actually implement realloc in client.
static void* R_LocalMalloc(size_t size)
{
	return ri.Malloc(size);
}

static void* R_LocalReallocSized(void *ptr, size_t old_size, size_t new_size)
{
	void *mem = ri.Malloc(new_size);

	if (ptr)
	{
		memcpy(mem, ptr, old_size);
		ri.Free(ptr);
	}

	return mem;
}

static void R_LocalFree(void *ptr)
{
	ri.Free(ptr);
}

#define STBI_MALLOC R_LocalMalloc
#define STBI_REALLOC_SIZED R_LocalReallocSized
#define STBI_FREE R_LocalFree
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
//#define STB_IMAGE_STATIC
#include "stb_image.h"

#define STBIW_MALLOC R_LocalMalloc
#define STBIW_REALLOC_SIZED R_LocalReallocSized
#define STBIW_FREE R_LocalFree
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
//#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"

#define IMG_BYTE 0
#define IMG_FLOAT 1

void R_LoadSTB( const char *name, byte **pic, int *width, int *height, int comp, int type)
{
	byte *raw;
	int x, y, n, len;

	if (width)
		*width = 0;
	if (height)
		*height = 0;
	*pic = NULL;

	len = ri.FS_ReadFile(name, (void **)&raw);
	if (!raw || len < 0)
		return;

	if (type == IMG_BYTE)
		*pic = stbi_load_from_memory(raw, len, &x, &y, &n, comp);
	else
		*pic = (byte *)stbi_loadf_from_memory(raw, len, &x, &y, &n, comp);

	ri.FS_FreeFile (raw);

	if (!pic)
		ri.Printf(PRINT_DEVELOPER, "R_LoadSTB(%s) failed: %s\n", name, stbi_failure_reason());

	if (width)
		*width = x;
	if (height)
		*height = y;
}

void R_LoadBMP( const char *name, byte **pic, int *width, int *height )
{
	R_LoadSTB(name, pic, width, height, 4, IMG_BYTE);
}

void R_LoadJPG( const char *name, byte **pic, int *width, int *height )
{
	R_LoadSTB(name, pic, width, height, 4, IMG_BYTE);
}

void R_LoadPNG( const char *name, byte **pic, int *width, int *height )
{
	R_LoadSTB(name, pic, width, height, 4, IMG_BYTE);
}

void R_LoadTGA( const char *name, byte **pic, int *width, int *height )
{
	R_LoadSTB(name, pic, width, height, 4, IMG_BYTE);
}

void R_LoadHDR( const char *name, byte **pic, int *width, int *height )
{
	R_LoadSTB(name, pic, width, height, 3, IMG_FLOAT);
}

static void RE_AppendToBuffer(void* context, void* data, int size)
{
	byte **out = (byte **)context;
	memcpy(*out, data, size);
	*out += size;
}

size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
    int image_width, int image_height, byte *image_buffer, int padding)
{
	byte *out, *newImage;
	int y;

	// flip image vertically
	out = newImage = ri.Malloc(image_height * image_width * 3);
	for (y = image_height - 1; y >= 0; y--)
	{
		byte *in = image_buffer + (image_width * 3 + padding) * y;
		memcpy(out, in, image_width * 3);
		out += image_width * 3;
	}

	out = buffer;
	if (!stbi_write_jpg_to_func(RE_AppendToBuffer, (void *)&out, image_width, image_height, 3, newImage, quality))
		out = buffer;

	ri.Free(newImage);

	return out - buffer;
}


void RE_SaveJPG(char * filename, int quality, int image_width, int image_height, byte *image_buffer, int padding)
{
	byte *out;
	size_t bufSize;

	bufSize = image_width * image_height * 3;
	out = ri.Hunk_AllocateTempMemory(bufSize);

	bufSize = RE_SaveJPGToBuffer(out, bufSize, quality, image_width, image_height, image_buffer, padding);
	ri.FS_WriteFile(filename, out, bufSize);

	ri.Hunk_FreeTempMemory(out);
}

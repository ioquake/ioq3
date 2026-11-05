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
#include <assert.h>

int main(int argc, char** argv) {
    if(argc != 3)
	return 0;

    char* fn = argv[1];
    FILE* f = fopen(fn, "rb");
    printf("unsigned char %s[] = {\n", argv[2]);
    unsigned long n = 0;

    while(!feof(f)) {
        unsigned char c;
        if(fread(&c, 1, 1, f) == 0) break;
        printf("0x%.2X, ", (int)c);
        ++n;
        if(n % 10 == 0) printf("\n");
    }

    fclose(f);
    printf("};\n");

    printf("int %s_size = %ld;\n", argv[2], n);
    return 0;
}

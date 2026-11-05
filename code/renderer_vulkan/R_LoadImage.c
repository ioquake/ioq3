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

#include "tr_local.h"
#include "ref_import.h"
#include "image_loader.h"

// Description:  Loads any of the supported image types into
// a cannonical 32 bit format.


typedef struct
{
    char *ext;
    void (*ImageLoader)( const char *, unsigned char **, int *, int * );
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
const static imageExtToLoaderMap_t imageLoaders[7] =
{
    { "tga",  R_LoadTGA },
    { "jpg",  R_LoadJPG },
    { "jpeg", R_LoadJPG },
    { "png",  R_LoadPNG },
    { "pcx",  R_LoadPCX },
    { "bmp",  R_LoadBMP },
    { "pvr",  R_LoadPVR }
};

const static int numImageLoaders = 7;


void R_LoadImage(const char *name, unsigned char **pic, uint32_t *width, uint32_t *height )
{
	int orgLoader = -1;
	int i;
	char localName[ 128 ] = {0};
	int iwidth = 0, iheight = 0;

    const char* pSrc = name;
    char* pDst = localName;
    char* pExt = NULL;

    *pic = NULL;
	*width = 0;
	*height = 0;


    char c;

    // copy name to localName
    while( (c = *pDst++ = *pSrc++) )
    {

        if(c == '.')
            pExt = pDst;
//        else if(c == '/')
//            slash = pDst-1;
    }
    
	if( pExt != NULL )
	{
		// Look for the correct loader and use it
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !Q_stricmp( pExt, imageLoaders[ i ].ext ) )
			{
                orgLoader = i;

				// Load
				imageLoaders[ i ].ImageLoader( localName, pic, &iwidth, &iheight );
				if( *pic != NULL )
                {
                    // Something loaded
                    *width = (uint32_t)iwidth;
                    *height = (uint32_t)iheight;
				    return;
                }
			}
		}
  
		// Loader failed, most likely because the file isn't there;
        // Try and find a suitable match using all the image formats supported

        for( i = 0; i < numImageLoaders; i++ )
	    {
            if (i == orgLoader)
                continue;

            strcpy(pExt, imageLoaders[ i ].ext);

            // Load
            imageLoaders[ i ].ImageLoader( localName, pic, &iwidth, &iheight );

            if( *pic != NULL )
            {
                // Something loaded
                *width = (uint32_t)iwidth;
                *height = (uint32_t)iheight;
                return;
            }
        }

        ri.Printf( PRINT_DEVELOPER, "%s not present\n", localName );

    }
    else
    {
          
        // Try and find a suitable match using all the image formats supported
        *(pDst-1) = '.';

        for( i = 0; i < numImageLoaders; i++ )
	    {
            strcpy(pDst, imageLoaders[ i ].ext);
            // Load
            imageLoaders[ i ].ImageLoader( localName, pic, &iwidth, &iheight );

            if( *pic != NULL )
            {
                ri.Printf( PRINT_DEVELOPER, "%s without a extension, using %s instead. \n",
                        name, localName);
                *width = (uint32_t)iwidth;
                *height = (uint32_t)iheight;
                return;
            }
        }
    }

    ri.Printf( PRINT_DEVELOPER, "%s not present.\n", name);

    // try again without the extension
    
}

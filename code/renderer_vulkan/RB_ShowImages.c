#include "tr_backend.h"
#include "vk_shade_geometry.h"
#include "vk_pipelines.h"
#include "glConfig.h"

/*
===============
Draw all the images to the screen, on top of whatever was there.
This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
// TODO: move glConfig retated stuff to glConfig.c,


void RB_ShowImages(image_t ** const pImg, unsigned int N)
{

    backEnd.projection2D = qtrue;

	const float black[4] = {0, 0, 0, 1};
	vk_clearColorAttachments(black);
   
    int width;
    int height;

    R_GetWinResolution(&width, &height);

    const float w = width / 20;
	const float h = height / 15;


    tess.numIndexes = 6;
    tess.numVertexes = 4;

    uint32_t i;
	for (i = 0 ; i < N; ++i)
    {
		//image_t* image = tr.images[i];
		float x = i % 20 * w;
		float y = i / 20 * h;

		tess.indexes[0] = 0;
		tess.indexes[1] = 1;
		tess.indexes[2] = 2;
		tess.indexes[3] = 0;
		tess.indexes[4] = 2;
		tess.indexes[5] = 3;

		tess.xyz[0][0] = x;
		tess.xyz[0][1] = y;

		tess.xyz[1][0] = x + w;
		tess.xyz[1][1] = y;

		tess.xyz[2][0] = x + w;
		tess.xyz[2][1] = y + h;
		
        tess.xyz[3][0] = x;
		tess.xyz[3][1] = y + h;

		tess.svars.texcoords[0][0][0] = 0;
		tess.svars.texcoords[0][0][1] = 0;
		tess.svars.texcoords[0][1][0] = 1;
		tess.svars.texcoords[0][1][1] = 0;
		tess.svars.texcoords[0][2][0] = 1;
		tess.svars.texcoords[0][2][1] = 1;
		tess.svars.texcoords[0][3][0] = 0;
		tess.svars.texcoords[0][3][1] = 1;

		memset( tess.svars.colors, 255, tess.numVertexes * 4 );
        
        updateCurDescriptor( pImg[i]->descriptor_set, 0);

        vk_UploadXYZI(tess.xyz, 4, tess.indexes, 6);

        // updateMVP(backEnd.viewParms.isPortal, backEnd.projection2D, getptr_modelview_matrix());
        updateMVP( 0 , 1, getptr_modelview_matrix());

        vk_shade_geometry(g_stdPipelines.images_debug_pipeline, VK_FALSE, DEPTH_RANGE_NORMAL, VK_TRUE);
	}
	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

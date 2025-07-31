#include "ref_import.h"
#include "tr_globals.h"


extern void RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
extern void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader );


/*
=============
FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const unsigned char *data, int client, qboolean dirty)
{
	int			i, j;

	if ( !tr.registered ) {
		return;
	}
	
    // make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ )
    {
        ;
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ )
    {
        ;
	}
    
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows) {
		ri.Error (ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows);
	}

    RE_UploadCinematic(w, h, cols, rows, data, client, dirty);

    tr.cinematicShader->stages[0]->bundle[0].image[0] = tr.scratchImage[client];
    RE_StretchPic(x, y, w, h,  0.5f / cols, 0.5f / rows,  1.0f - 0.5f / cols, 1.0f - 0.5 / rows, tr.cinematicShader->index);
}

#include "tr_fog.h"
#include <stdlib.h>
#include <math.h>

#define	FOG_TABLE_SIZE		256

static float FogTable[FOG_TABLE_SIZE];


void R_InitFogTable( void )
{
	float exp = 0.5;

    unsigned int i;

	for ( i = 0 ; i < FOG_TABLE_SIZE ; i++ )
    {
		FogTable[i] = pow ( (float)i/(FOG_TABLE_SIZE-1), exp );
	}
}


/*
================
Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float R_FogFactor( float s, float t )
{
	s -= 1.0/512;
	if ( s < 0 ) {
		return 0;
	}
	if ( t < 1.0/32 ) {
		return 0;
	}
	if ( t < 31.0/32 ) {
		s *= (t - 1.0f/32.0f) / (30.0f/32.0f);
	}

	// we need to leave a lot of clamp range
	s *= 8;

	if ( s > 1.0 ) {
		s = 1.0;
	}

	float d = FogTable[ (int)(s * (FOG_TABLE_SIZE-1)) ];

	return d;
}

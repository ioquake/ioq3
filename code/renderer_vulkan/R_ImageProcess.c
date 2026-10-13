#include "tr_cvar.h"
#include "ref_import.h"
#include "vk_image.h"
#include "R_ImageProcess.h"


static unsigned char s_intensitytable[256];
static unsigned char s_gammatable[256];

/*
void R_GammaCorrect(unsigned char* buffer, const unsigned int Size)
{
	unsigned int i;

	for( i = 0; i < Size; i++ )
    {
		buffer[i] = s_gammatable[buffer[i]];
	}
}
*/

void R_SetColorMappings( void )
{
	int		i, j;
	int		inf;
	int		shift = 0;

    for (i = 0; i < 255; i++)
    {
        s_intensitytable[i] = s_gammatable[i] = i;
    }

	float g = r_gamma->value;

	for ( i = 0; i < 256; i++ ) {
		if ( g == 1 ) {
			inf = i;
		} else {
			inf = 255 * pow ( i/255.0f, 1.0f / g ) + 0.5f;
		}
		inf <<= shift;
		if (inf < 0) {
			inf = 0;
		}
        else if (inf > 255) {
			inf = 255;
		}
		s_gammatable[i] = inf;
	}


	if ( r_intensity->value <= 1 ) {
		ri.Cvar_Set( "r_intensity", "1" );
	}

	for (i=0 ; i<256 ; i++)
    {
		j = i * r_intensity->value;
		if (j > 255) {
			j = 255;
		}
		s_intensitytable[i] = j;
	}

}


/*
================
Scale up the pixel values in a texture to increase the lighting range
================
*/
void R_LightScaleTexture (unsigned char* dst, unsigned char* in, unsigned int nBytes)
{
    unsigned int i;

    if ( 0 )
    {
        for (i=0; i<nBytes; i+=4)
        {
            unsigned int n1 = i + 1;
            unsigned int n2 = i + 2;
            unsigned int n3 = i + 3;

            dst[i] = s_gammatable[in[i]];
            dst[n1] = s_gammatable[in[n1]];
            dst[n2] = s_gammatable[in[n2]];
            dst[n3] = in[n3];
        }
    }
    else
    {
        for (i=0; i<nBytes; i+=4)
        {
            unsigned int n1 = i + 1;
            unsigned int n2 = i + 2;
            unsigned int n3 = i + 3;

            dst[i] = s_gammatable[s_intensitytable[in[i]]];
            dst[n1] = s_gammatable[s_intensitytable[in[n1]]];
            dst[n2] = s_gammatable[s_intensitytable[in[n2]]];
            dst[n3] = in[n3];
        }
    }
}

//////////////////////////////////////////////////////////////////////
//
//      MIP maps
//
//////////////////////////////////////////////////////////////////////

// In computer graphics, mipmaps (also MIP maps) or pyramids are pre-calculated,
// optimized sequences of images, each of which is a progressively lower resolution
// representation of the same image. The height and width of each image, or level, 
// in the mipmap is a power of two smaller than the previous level. 
// Mipmaps do not have to be square. They are intended to increase rendering speed
// and reduce aliasing artifacts.
// A high-resolution mipmap image is used for high-density samples, such as for 
// objects close to the camera. Lower-resolution images are used as the object
// appears farther away.
// This is a more efficient way of downfiltering (minifying) a texture than
// sampling all texels in the original texture that would contribute to a 
// screen pixel; it is faster to take a constant number of samples from the
// appropriately downfiltered textures. Mipmaps are widely used in 3D computer games. 

// The letters "MIP" in the name are an acronym of the Latin phrase multum in parvo, 
// meaning "much in little".Since mipmaps, by definition, are pre-allocated, 
// additional storage space is required to take advantage of them. 
// Mipmap textures are used in 3D scenes to decrease the time required to 
// render a scene. They also improve the scene's realism.

// mip-mapping of 1/3 more memory per texture.

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void R_BlendOverTexture(unsigned char* data, const uint32_t pixelCount, uint32_t l)
{
	uint32_t i;

    static const unsigned char mipBlendColors[16][4] =
    {
        {0,0,0,0},
        {255,0,0,128},
        {0,255,0,128},
        {0,0,255,128},
        {255,0,0,128},
        {0,255,0,128},
        {0,0,255,128},
        {255,0,0,128},
        {0,255,0,128},
        {0,0,255,128},
        {255,0,0,128},
        {0,255,0,128},
        {0,0,255,128},
        {255,0,0,128},
        {0,255,0,128},
        {0,0,255,128},
    };

    const unsigned int alpha = mipBlendColors[l][3];
	const unsigned int inverseAlpha = 255 - alpha;
    
	const unsigned int bR = mipBlendColors[l][0] * alpha;
	const unsigned int bG = mipBlendColors[l][1] * alpha;
	const unsigned int bB = mipBlendColors[l][2] * alpha;

	for ( i = 0; i < pixelCount; i++, data+=4 )
    {
		data[0] = ( data[0] * inverseAlpha + bR ) >> 9;
		data[1] = ( data[1] * inverseAlpha + bG ) >> 9;
		data[2] = ( data[2] * inverseAlpha + bB ) >> 9;
	}
}


/*
================
R_MipMap

Operates in place, quartering the size of the texture, no error checking
================
*/
void R_MipMap(const unsigned char* in, uint32_t width, uint32_t height, unsigned char* out)
{

	if ( (width == 1) && (height == 1) )
    {
        out[0] = in[0];
		return;
    }
    uint32_t i;

    const unsigned int row = width * 4;
	width >>= 1;
	height >>= 1;

	if ( (width == 0) || (height == 0) )
    {
		width += height;	// get largest
		for (i=0; i<width; i++, out+=4, in+=8 )
        {
			out[0] = ( in[0] + in[4] )>>1;
			out[1] = ( in[1] + in[5] )>>1;
			out[2] = ( in[2] + in[6] )>>1;
			out[3] = ( in[3] + in[7] )>>1;
		}
	}
    else
    {   
        for (i=0; i<height; i++, in+=row)
        {
            uint32_t j;
            for (j=0; j<width; j++, out+=4, in+=8)
            {
                out[0] = (in[0] + in[4] + in[row+0] + in[row+4])>>2;
                out[1] = (in[1] + in[5] + in[row+1] + in[row+5])>>2;
                out[2] = (in[2] + in[6] + in[row+2] + in[row+6])>>2;
                out[3] = (in[3] + in[7] + in[row+3] + in[row+7])>>2;
            }
        }
    }
}


/*
================
R_MipMap2

Operates in place, quartering the size of the texture
Proper linear filter, no error checking
================
*/
void R_MipMap2(const unsigned char* in, uint32_t inWidth, uint32_t inHeight, unsigned char* out)
{

	int	i, j;

	if ( (inWidth == 1) && (inHeight == 1) )
    {
        out[0] = in[0];
		return;
    }
	//ri.Printf (PRINT_ALL, "\n---R_MipMap2---\n");
    // Not run time funs, can be used for best view effects

	unsigned int outWidth = inWidth >> 1;
	unsigned int outHeight = inHeight >> 1;
    unsigned int nBytes = outWidth * outHeight * 4;

    unsigned char * temp = (unsigned char *)malloc( nBytes );

	const unsigned int inWidthMask = inWidth - 1;
	const unsigned int inHeightMask = inHeight - 1;

	for ( i = 0 ; i < outHeight ; i++ )
    {
		for ( j = 0 ; j < outWidth ; j++ )
        {
            unsigned int outIndex = i * outWidth + j;
            unsigned int k;
			for ( k = 0 ; k < 4 ; k++ )
            {
                unsigned int r0 = ((i*2-1) & inHeightMask) * inWidth;
                unsigned int r1 = ((i*2 ) & inHeightMask) * inWidth;
                unsigned int r2 = ((i*2+1) & inHeightMask) * inWidth;
                unsigned int r3 = ((i*2+2) & inHeightMask) * inWidth;

                unsigned int c0 = ((j*2-1) & inWidthMask);
                unsigned int c1 = ((j*2 ) & inWidthMask);
                unsigned int c2 = ((j*2+1) & inWidthMask);
                unsigned int c3 = ((j*2+2) & inWidthMask);


				unsigned int total = 
					1 * in[(r0 + c0) * 4 + k] +
					2 * in[(r0 + c1) * 4 + k] +
					2 * in[(r0 + c2) * 4 + k] +
					1 * in[(r0 + c3) * 4 + k] +

					2 * in[(r1 + c0) * 4 + k] +
					4 * in[(r1 + c1) * 4 + k] +
					4 * in[(r1 + c2) * 4 + k] +
					2 * in[(r1 + c3) * 4 + k] +

					2 * in[(r2 + c0) * 4 + k] +
					4 * in[(r2 + c1) * 4 + k] +
					4 * in[(r2 + c2) * 4 + k] +
					2 * in[(r2 + c3) * 4 + k] +

					1 * in[(r3 + c0) * 4 + k] +
					2 * in[(r3 + c1) * 4 + k] +
					2 * in[(r3 + c2) * 4 + k] +
					1 * in[(r3 + c3) * 4 + k] ;
				
                temp[4*outIndex + k] = total / 36;
			}
		}
	}

	memcpy( out, temp, nBytes );
	free( temp );
}

/*
================

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function before or after.
================
*/

void ResampleTexture(unsigned char * pOut, const unsigned int inwidth, const unsigned int inheight,
                               const unsigned char *pIn, const unsigned int outwidth, const unsigned int outheight)
{
	unsigned int i, j;
	unsigned int p1[2048], p2[2048];

    // printf("inwidth: %d \t outwidth: %d \n", inwidth, outwidth);

    unsigned int fracstep = (inwidth << 16)/outwidth;

    unsigned int frac1 = fracstep>>2;
    unsigned int frac2 = 3*(fracstep>>2);

    for(i=0; i<outwidth; i++)
    {
        p1[i] = 4*(frac1>>16);
        frac1 += fracstep;

        p2[i] = 4*(frac2>>16);
        frac2 += fracstep;
    }
 
   
	for (i=0; i<outheight; i++)
	{
		const unsigned char* inrow1 = pIn + 4 * inwidth * (unsigned int)((i+0.25)*inheight/outheight);
		const unsigned char* inrow2 = pIn + 4 * inwidth * (unsigned int)((i+0.75)*inheight/outheight);
        //const unsigned char* inrow1 = pIn + inwidth * (unsigned int)((4*i+1)*inheight/outheight);
		//const unsigned char* inrow2 = pIn + inwidth * (unsigned int)((4*i+3)*inheight/outheight);


        // printf("inrow1: %d \t inrow2: %d \n", 
        //        4 * (unsigned int)((i+0.25)*inheight/outheight),
        //        4 * (unsigned int)((i+0.75)*inheight/outheight)
        //        );

        for (j=0; j<outwidth; j++)
        {
			const unsigned char* pix1 = inrow1 + p1[j];
			const unsigned char* pix2 = inrow1 + p2[j];
			const unsigned char* pix3 = inrow2 + p1[j];
			const unsigned char* pix4 = inrow2 + p2[j];
            
            unsigned char* pCurPix = pOut + j*4;

			pCurPix[0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			pCurPix[1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			pCurPix[2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			pCurPix[3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}

        pOut += outwidth*4;
	}
}


void GetScaledDimension(const unsigned int width, const unsigned int height, unsigned int * const outW, unsigned int * const outH, int isPicMip)
{
    const unsigned int max_texture_size = 2048;
    
    unsigned int scaled_width, scaled_height;

    for(scaled_width = max_texture_size; scaled_width > width; scaled_width>>=1)
        ;
    
    for (scaled_height = max_texture_size; scaled_height > height; scaled_height>>=1)
        ;

    // perform optional picmip operation
    if ( isPicMip )
    {
        scaled_width >>= r_picmip->integer;
        scaled_height >>= r_picmip->integer;
    }

	// clamp to minimum size
    if (scaled_width == 0) {
        scaled_width = 1;
    }
    if (scaled_height == 0) {
        scaled_height = 1;
    }

    *outW = scaled_width;
    *outH = scaled_height;
}


///////////////////////////////////////////////////////////////////////////////////////////////
// DEBUG HELPER FUNCTIONAS ...
///////////////////////////////////////////////////////////////////////////////////////////////

void imsave(char *fileName, unsigned char* buffer2, unsigned int width, unsigned int height);
void fsWriteFile( const char *qpath, const void *buffer, int size );


void fsWriteFile( const char *qpath, const void *buffer, int size )
{
	
    unsigned char* buf = (unsigned char *)buffer;

    FILE * f = fopen( qpath, "wb" );
	if ( !f )
    {
		fprintf(stderr, "Failed to open %s\n", qpath );
		return;
	}

    int remaining = size;
	int tries = 0;
    int block = 0; 
    int written = 0;

    while (remaining)
    {
		block = remaining;
		written = fwrite (buf, 1, block, f);
		if (written == 0)
        {
			if (!tries)
            {
				tries = 1;
			}
            else
            {
				fprintf(stderr, "FS_Write: 0 bytes written\n" );
				return;
			}
		}

		if (written == -1)
        {
			fprintf(stderr, "FS_Write: -1 bytes written\n" );
			return;
		}

		remaining -= written;
		buf += written;
	}

    //FS_Write( buffer, size, f );
    
    fclose(f);
}


void imsave(char *fileName, unsigned char* buffer2, unsigned int width, unsigned int height)
{

    const unsigned int cnPixels = width * height;

	unsigned char* buffer = (unsigned char*) malloc( cnPixels * 3 + 18);

    memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

    unsigned char* buffer_ptr = buffer + 18;
    unsigned char* buffer2_ptr = buffer2;
    
    unsigned int i;
    for (i = 0; i < cnPixels; i++)
    {
        buffer_ptr[0] = buffer2_ptr[0];
        buffer_ptr[1] = buffer2_ptr[1];
        buffer_ptr[2] = buffer2_ptr[2];
        buffer_ptr += 3;
        buffer2_ptr += 4;
    }


	// swap rgb to bgr
	const unsigned int c = 18 + width * height * 3;
	for (i=18; i<c; i+=3)
    {
		unsigned char temp = buffer[i];
		buffer[i] = buffer[i+2];
		buffer[i+2] = temp;
	}

 	
    fsWriteFile( fileName, buffer, c );

	free( buffer );

    ri.Printf( PRINT_ALL, "imsave: %s\n", fileName );
}


void DEBUG_resample(const char *name, unsigned char* data, unsigned char* pBuffer,
       unsigned int width, unsigned int height, unsigned int scaled_width, unsigned int scaled_height)
{
    const char *slash = strrchr(name, '/');

    char tmpName[128] = {0};
    char tmpName2[128] = "resampled_";

    strcpy(tmpName, slash+1);
    strcat(tmpName2, tmpName);

    imsave(tmpName , data, width, height);            
    imsave(tmpName2, pBuffer, scaled_width, scaled_height);

    ri.Printf( PRINT_ALL, "tmpName: %s\n", tmpName);
    ri.Printf( PRINT_ALL, "tmpName2: %s\n",  tmpName2);

    ri.Printf( PRINT_ALL, "DEBUG_resample, inwidth: %d, inheight: %d, outwidth: %d, outheight: %d\n",
                width, height, scaled_width, scaled_height );
}

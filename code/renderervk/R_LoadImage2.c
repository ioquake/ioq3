#include "tr_local.h"
#include "ref_import.h"

static void* q3_stbi_malloc(size_t size) {
    return ri.Malloc((int)size);
}
static void q3_stbi_free(void* p) {
    ri.Free(p);
}
static void* q3_stbi_realloc(void* p, size_t old_size, size_t new_size) {
    if (p == NULL)
        return q3_stbi_malloc(new_size);

    void* p_new;
    if (old_size < new_size) {
        p_new = q3_stbi_malloc(new_size);
        memcpy(p_new, p, old_size);
        q3_stbi_free(p);
    } else {
        p_new = p;
    }
    return p_new;
}
#define STBI_MALLOC q3_stbi_malloc
#define STBI_FREE q3_stbi_free
#define STBI_REALLOC_SIZED q3_stbi_realloc
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



static void LoadTGA( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height);
static void LoadBMP( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height);
static void LoadJPG( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height);
static void LoadPCX32 ( const char* filename, unsigned char** pic, uint32_t* width, uint32_t* height);



/*
    Loads any of the supported image types into a cannonical 32 bit format.
*/
void R_LoadImage2(const char *name, unsigned char **pic, uint32_t* width, uint32_t* height)
{

	const int len = (int)strlen(name);
	if (len<5)
    {
        ri.Printf( PRINT_WARNING, "R_LoadImage2: try to loading %s ? \n", name );
		return;
	}

    // point to '.', .jped are assume not exist
    const char* const pPnt = (char*)name + len - 4;
    
    if(pPnt[0] == '.')
    {   // have a extension
        if( ( (pPnt[1] == 't') && (pPnt[2] == 'g') && (pPnt[3] == 'a') ) ||
            ( (pPnt[1] == 'T') && (pPnt[2] == 'G') && (pPnt[3] == 'A') ) )
        {
            LoadTGA( name, pic, width, height );

            if (NULL == *pic)
            {
             // try jpg in place of tga
                char altname[128] = {0};
                strncpy( altname, name, 128 );                      
                char* pt = altname + len - 4;

                pt[1] = 'j';
                pt[2] = 'p';
                pt[3] = 'g';
                
                LoadJPG( altname, pic, width, height );
		    }
        }
        else if ( ( (pPnt[1] == 'j') && (pPnt[2] == 'p') && (pPnt[3] == 'g') ) ||
                ( (pPnt[1] == 'J') && (pPnt[2] == 'P') && (pPnt[3] == 'G') )  )
        {
            LoadJPG( name, pic, width, height );
        }
        else if ( ( (pPnt[1] == 'b') && (pPnt[2] == 'm') && (pPnt[3] == 'p') ) ||
                ( (pPnt[1] == 'B') && (pPnt[2] == 'M') && (pPnt[3] == 'P') )  )

        {
            LoadBMP( name, pic, width, height );
        }
        else if ( ( (pPnt[1] == 'p') && (pPnt[2] == 'c') && (pPnt[3] == 'x') ) ||
                ( (pPnt[1] == 'P') && (pPnt[2] == 'C') && (pPnt[3] == 'X') )  )
        {
            LoadPCX32( name, pic, width, height );
        }
    }
    else
    {   // without a extension
        // Try and find a suitable match using all the image formats supported
        char altname[128] = {0};
        strcpy( altname, name );          
        char* pt = altname + len;
        pt[0] = '.';
        pt[1] = 't';
        pt[2] = 'g';
        pt[3] = 'a';
        pt[4] = '\0';

        LoadTGA( altname, pic, width, height );
        
        if (NULL == *pic)
        {
            // try jpg in place of tga
            pt[0] = '.';
            pt[1] = 'j';
            pt[2] = 'p';
            pt[3] = 'g';
            pt[4] = '\0';

            LoadJPG( altname, pic, width, height );
        }
        
        if( *pic == NULL )
        {
            ri.Printf( PRINT_WARNING, "%s not present.\n", name);
        }
        // else
        //    ri.Printf( PRINT_ALL, "%s without a extension, using %s instead. \n", name, altname);

    }
}




// ====================================== //
// ====================================== //

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;



static void LoadTGA( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height)
{
	int	columns, rows, numPixels;
	unsigned char* pixbuf;
	int	row, column;

	char* buffer;
	TargaHeader	targa_header;

	*pic = NULL;

	//
	// load the file
	//
	ri.FS_ReadFile(name, (void**)&buffer);
	if (!buffer) {
		return;
	}

	char* buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;
	
	targa_header.colormap_index = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.width = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.height = LittleShort ( *(short *)buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if (targa_header.image_type!=2 
		&& targa_header.image_type!=10
		&& targa_header.image_type != 3 ) 
	{
		ri.Error (ERR_DROP, "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n");
	}

	if ( targa_header.colormap_type != 0 )
	{
		ri.Error( ERR_DROP, "LoadTGA: colormaps not supported\n" );
	}

	if ( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
	{
		ri.Error (ERR_DROP, "LoadTGA: Only 32 or 24 bit images supported (no colormaps)\n");
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if (width)
		*width = columns;
	if (height)
		*height = rows;

	unsigned char* targa_rgba = (unsigned char*) ri.Malloc (numPixels*4);
	*pic = targa_rgba;

	if (targa_header.id_length != 0)
		buf_p += targa_header.id_length;  // skip TARGA image comment
	
	if ( targa_header.image_type==2 || targa_header.image_type == 3 )
	{ 
		// Uncompressed RGB or gray scale image
		for(row=rows-1; row>=0; row--) 
		{
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; column++) 
			{
				unsigned char red,green,blue,alphabyte;
				switch (targa_header.pixel_size) 
				{
					
				case 8:
					blue = *buf_p++;
					green = blue;
					red = blue;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;

				case 24:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = 255;
					break;
				case 32:
					blue = *buf_p++;
					green = *buf_p++;
					red = *buf_p++;
					alphabyte = *buf_p++;
					*pixbuf++ = red;
					*pixbuf++ = green;
					*pixbuf++ = blue;
					*pixbuf++ = alphabyte;
					break;
				default:
					ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
					break;
				}
			}
		}
	}
	else if (targa_header.image_type==10) {   // Runlength encoded RGB images
		unsigned char red,green,blue,alphabyte,packetHeader,packetSize,j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for(row=rows-1; row>=0; row--) {
			pixbuf = targa_rgba + row*columns*4;
			for(column=0; column<columns; ) {
				packetHeader= *buf_p++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80) {        // run-length packet
					switch (targa_header.pixel_size) {
						case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = 255;
								break;
						case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								break;
						default:
							ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
							break;
					}
	
					for(j=0;j<packetSize;j++) {
						*pixbuf++=red;
						*pixbuf++=green;
						*pixbuf++=blue;
						*pixbuf++=alphabyte;
						column++;
						if (column==columns) { // run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}
					}
				}
				else {                            // non run-length packet
					for(j=0;j<packetSize;j++) {
						switch (targa_header.pixel_size) {
							case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = 255;
									break;
							case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									*pixbuf++ = red;
									*pixbuf++ = green;
									*pixbuf++ = blue;
									*pixbuf++ = alphabyte;
									break;
							default:
								ri.Error( ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'\n", targa_header.pixel_size, name );
								break;
						}
						column++;
						if (column==columns) { // pixel packet run spans across rows
							column=0;
							if (row>0)
								row--;
							else
								goto breakOut;
							pixbuf = targa_rgba + row*columns*4;
						}						
					}
				}
			}
			breakOut:;
		}
	}

#if 0 
  // TTimo: this is the chunk of code to ensure a behavior that meets TGA specs 
  // bk0101024 - fix from Leonardo
  // bit 5 set => top-down
  if (targa_header.attributes & 0x20) {
    unsigned char *flip = (unsigned char*)malloc (columns*4);
    unsigned char *src, *dst;

    for (row = 0; row < rows/2; row++) {
      src = targa_rgba + row * 4 * columns;
      dst = targa_rgba + (rows - row - 1) * 4 * columns;

      memcpy (flip, src, columns*4);
      memcpy (src, dst, columns*4);
      memcpy (dst, flip, columns*4);
    }
    free (flip);
  }
#endif
  // instead we just print a warning
  if (targa_header.attributes & 0x20) {
    ri.Printf( PRINT_WARNING, "WARNING: '%s' TGA file header declares top-down image, ignoring\n", name);
  }

  ri.FS_FreeFile (buffer);
}


static void LoadJPG( const char* name, unsigned char** pic, uint32_t* width, uint32_t* height)
{
    char* fbuffer;
    int len = ri.FS_ReadFile(name, (void**)&fbuffer);
    if (!fbuffer) {
        return;
    }
  
    int components;
    *pic = stbi_load_from_memory((unsigned char*)fbuffer, len, (int*)width, (int*)height, &components, STBI_rgb_alpha);
    if (*pic == NULL) {
        ri.FS_FreeFile(fbuffer);
        return;
    }

    // clear all the alphas to 255
    {
        unsigned int i;
        unsigned char* buf = *pic;

        unsigned int nBytes = 4 * (*width) * (*height);
        for (i = 3; i < nBytes; i += 4)
        {
            buf[i] = 255;
        }
    }
    ri.FS_FreeFile(fbuffer);
}



/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct
{
	char id[2];
	unsigned long fileSize;
	unsigned long reserved0;
	unsigned long bitmapDataOffset;
	unsigned long bitmapHeaderSize;
	unsigned long width;
	unsigned long height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned long compression;
	unsigned long bitmapDataSize;
	unsigned long hRes;
	unsigned long vRes;
	unsigned long colors;
	unsigned long importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;




static void LoadBMP( const char *name, unsigned char **pic, uint32_t *width, uint32_t *height )
{
	int		columns, rows, numPixels;
	unsigned char	*pixbuf;
	int		row, column;
	char* buf_p;
	char* buffer;
	
    int		length;
	BMPHeader_t bmpHeader;
	unsigned char *bmpRGBA;

	*pic = NULL;

	//
	// load the file
	//
	length = ri.FS_ReadFile(name, (void**)&buffer);
	if (!buffer) {
		return;
	}

	buf_p = buffer;

	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;

	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );

	if ( bmpHeader.bitsPerPixel == 8 )
		buf_p += 1024;

	if ( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) 
	{
		ri.Error( ERR_DROP, "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.fileSize != length )
	{
		ri.Error( ERR_DROP, "LoadBMP: header size does not match file size (%ld vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if ( bmpHeader.compression != 0 )
	{
		ri.Error( ERR_DROP, "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if ( bmpHeader.bitsPerPixel < 8 )
	{
		ri.Error( ERR_DROP, "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}

	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if ( rows < 0 )
		rows = -rows;
	numPixels = columns * rows;

	if ( width ) 
		*width = columns;
	if ( height )
		*height = rows;

	bmpRGBA = (unsigned char*) ri.Malloc( numPixels * 4 );
	*pic = bmpRGBA;


	for ( row = rows-1; row >= 0; row-- )
	{
		pixbuf = bmpRGBA + row*columns*4;

		for ( column = 0; column < columns; column++ )
		{
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;

			switch ( bmpHeader.bitsPerPixel )
			{
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = * ( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;

			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				ri.Error( ERR_DROP, "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
				break;
			}
		}
	}

	ri.FS_FreeFile( buffer );

}


typedef struct {
    char	manufacturer;
    char	version;
    char	encoding;
    char	bits_per_pixel;
    unsigned short	xmin,ymin,xmax,ymax;
    unsigned short	hres,vres;
    unsigned char	palette[48];
    char	reserved;
    char	color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;
    char	filler[58];
    char	data;			// unbounded
} pcx_t;

static void LoadPCX ( const char *filename, unsigned char **pic, unsigned char **palette, uint32_t *width, uint32_t *height)
{
	char* raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	unsigned char	*out, *pix;
	int		xmax, ymax;

	*pic = NULL;
	*palette = NULL;

	//
	// load the file
	//
	len = ri.FS_ReadFile(filename, (void**)&raw);
	if (!raw) {
		return;
	}

	//
	// parse the PCX file
	//
	pcx = (pcx_t *)raw;
	raw = &pcx->data;

  	xmax = LittleShort(pcx->xmax);
    ymax = LittleShort(pcx->ymax);

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| xmax >= 1024
		|| ymax >= 1024)
	{
		ri.Printf (PRINT_ALL, "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax+1, ymax+1, pcx->xmax, pcx->ymax);
		return;
	}

	out = (unsigned char*) ri.Malloc ( (ymax+1) * (xmax+1) );

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = (unsigned char*) ri.Malloc(768);
		memcpy (*palette, (unsigned char *)pcx + len - 768, 768);
	}

	if (width)
		*width = xmax+1;
	if (height)
		*height = ymax+1;
// FIXME: use bytes_per_line here?

	for (y=0 ; y<=ymax ; y++, pix += xmax+1)
	{
		for (x=0 ; x<=xmax ; )
		{
			dataByte = *raw++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
				pix[x++] = dataByte;
		}

	}

	if ( raw - (char *)pcx > len)
	{
		ri.Printf (PRINT_DEVELOPER, "PCX file %s was malformed", filename);
		ri.Free (*pic);
		*pic = NULL;
	}

	ri.FS_FreeFile (pcx);
}


static void LoadPCX32 ( const char *filename, unsigned char **pic, uint32_t *width, uint32_t *height)
{
	unsigned char	*palette;
	unsigned char	*pic8;
	int		i, c, p;
	unsigned char	*pic32;

	LoadPCX (filename, &pic8, &palette, width, height);
	if (!pic8) {
		*pic = NULL;
		return;
	}

	c = (*width) * (*height);
	pic32 = *pic = (unsigned char*) ri.Malloc(4 * c );
	for (i = 0 ; i < c ; i++) {
		p = pic8[i];
		pic32[0] = palette[p*3];
		pic32[1] = palette[p*3 + 1];
		pic32[2] = palette[p*3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}

	ri.Free (pic8);
	ri.Free (palette);
}

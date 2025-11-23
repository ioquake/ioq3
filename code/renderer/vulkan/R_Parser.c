#include "tr_local.h"
#include "ref_import.h"

/*
============================================================================

                         PARSING

split those parsing functions from q_shared.c
I want the render part standalone, dont fuck up with game part.

============================================================================
*/


static char	r_parsename[512];
static int	r_lines;
static int	r_tokenline;

void R_BeginParseSession(const char* name)
{
	r_lines = 1;
	r_tokenline = 0;
	snprintf(r_parsename, sizeof(r_parsename), "%s", name);
}

int R_GetCurrentParseLine( void )
{
	if ( r_tokenline )
	{
		return r_tokenline;
	}

	return r_lines;
}



int R_Compress( char *data_p )
{
	qboolean newline = qfalse;
    qboolean whitespace = qfalse;

	char* in = data_p;
    char* out = data_p;

	if (in)
    {
        int c;
		while ((c = *in) != 0)
        {
			// skip double slash comments
			if ( c == '/' && in[1] == '/' )
            {
				while (*in && *in != '\n') {
					in++;
				}
			// skip /* */ comments
			}
            else if ( c == '/' && in[1] == '*' ) {
				while ( *in && ( *in != '*' || in[1] != '/' ) ) 
					in++;
				if ( *in ) 
					in += 2;
				// record when we hit a newline
			}
            else if ( c == '\n' || c == '\r' ) {
				newline = qtrue;
				in++;
				// record when we hit whitespace
			}
            else if ( (c == ' ') || (c == '\t') )
            {
				whitespace = qtrue;
				in++;
				// an actual token
			}
            else
            {
				// if we have a pending newline, emit it (and it counts as whitespace)
				if (newline)
                {
					*out++ = '\n';
					newline = qfalse;
					whitespace = qfalse;
				}
                if (whitespace)
                {
					*out++ = ' ';
					whitespace = qfalse;
				}

				// copy quoted strings unmolested
				if (c == '"') {
					*out++ = c;
					in++;
					while (1) {
						c = *in;
						if (c && c != '"') {
							*out++ = c;
							in++;
						} else {
							break;
						}
					}
					if (c == '"') {
						*out++ = c;
						in++;
					}
				} else {
					*out = c;
					out++;
					in++;
				}
			}
		}

		*out = 0;
	}
	return out - data_p;
}


/*
==============
Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is a newline.
==============
*/
char* R_ParseExt(char** data_p, qboolean allowLineBreaks)
{

    unsigned int len = 0;
	char *data = *data_p;

    unsigned char c;
    static char r_token[512] = {0}; 
    r_token[0] = 0;
	r_tokenline = 0;

	// make sure incoming data is valid
	if( !data )
	{
		*data_p = NULL;
		return r_token;
	}


	while( 1 )
	{
		// skip whitespace
		//data = SkipWhitespace( data, &hasNewLines );

	    while( (c = *data) <= ' ')
        {
		    if( c == '\n' )
            {
			    r_lines++;
		        if( allowLineBreaks == qfalse )
		        {
			        *data_p = data;
			        return r_token;
		        }
		    }
            else if( c == 0 )
            {
			    *data_p = NULL;
			    return r_token;
		    }

		    data++;
	    }

		// skip double slash comments
		if(data[0] == '/')
        {    
            if(data[1] == '/')
		    {
			    data += 2;
			    while (*data && (*data != '\n'))
                {
				    data++;
			    }
		    }
		    else if( data[1] == '*' ) 
		    {   // skip /* */ comments
			    data += 2;
                // Assuming /* and */ occurs in pairs.
			    while( (data[0] != '*') || (data[1] != '/') ) 
			    {
				    if ( data[0] == '\n' )
				    {
					    r_lines++;
				    }
				    data++;
			    }
				data += 2;
		    }
		    else
                break;
        }
        else
            break;
	}

	// token starts on this line
	r_tokenline = r_lines;

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\"' || !c)
			{
				r_token[len] = 0;
				*data_p = data;
				return r_token;
			}
            else if ( c == '\n' )
			{
				r_lines++;
			}

			if (len < MAX_TOKEN_CHARS - 1)
			{
				r_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS - 1)
		{
			r_token[len++] = c;
		}

		c = *(++data);
	} while(c > ' ');

	r_token[len] = 0;

	*data_p = data;
	return r_token;
}

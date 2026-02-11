#ifndef REF_IMPORT_H_
#define REF_IMPORT_H_

#include "../qcommon/q_shared.h"
#include "../renderercommon/tr_public.h"

extern refimport_t ri;

char* R_SkipPath (char *pathname);
void R_StripExtension( const char *in, char *out, int destsize );
// const char* R_GetExtension( const char *name );

#endif

/*
===========================================================================
Copyright (C) 2006 Tony J. White (tjw@tjw.org)

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

#ifndef __CL_HTTP_H__
#define __CL_HTTP_H__

#include "../qcommon/q_shared.h"

qboolean CL_HTTP_Init( void );
qboolean CL_HTTP_Available( void );
void CL_HTTP_Shutdown( void );
void CL_HTTP_BeginDownload( const char *localName, const char *remoteURL );
void CL_HTTP_PerformDownload( void );

#endif	// __CL_HTTP_H__

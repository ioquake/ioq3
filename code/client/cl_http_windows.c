/*
===========================================================================
Copyright (C) 2025 Tim Angus (tim@ngus.net)

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

#ifdef USE_HTTP

#include "client.h"

#include <windows.h>
#include <wininet.h>

static HINTERNET hInternet = NULL;
static HINTERNET hUrl = NULL;

static void DropIf(qboolean condition, const char *fmt, ...)
{
    char buffer[1024];

    if (!condition)
        return;

    va_list argptr;
    va_start(argptr, fmt);
    Q_vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    va_end(argptr);

    Com_Error(ERR_DROP, "Download Error: %s URL: %s", buffer, clc.downloadURL);
}

/*
=================
CL_HTTP_Init
=================
*/
qboolean CL_HTTP_Init()
{
    OSVERSIONINFO osvi = {sizeof(OSVERSIONINFO)};
    const char *windowsVersion = GetVersionEx(&osvi) ? va("Windows %lu.%lu (build %lu)",
                                                          osvi.dwMajorVersion,
                                                          osvi.dwMinorVersion,
                                                          osvi.dwBuildNumber)
                                                     : "Windows";

    hInternet = InternetOpenA(
        va("%s %s", Q3_VERSION, windowsVersion),
        INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);

    return hInternet != NULL;
}

/*
=================
CL_HTTP_Available
=================
*/
qboolean CL_HTTP_Available()
{
    return hInternet != NULL;
}

/*
=================
CL_HTTP_Shutdown
=================
*/
void CL_HTTP_Shutdown(void)
{
    if (hInternet)
    {
        InternetCloseHandle(hInternet);
        hInternet = NULL;
    }
}

/*
=================
CL_HTTP_BeginDownload
=================
*/
void CL_HTTP_BeginDownload(const char *remoteURL)
{
    DWORD httpCode = 0;
    DWORD contentLength = 0;
    DWORD len = sizeof(httpCode);
    DWORD zero = 0;
    BOOL success;

    hUrl = InternetOpenUrlA(hInternet, remoteURL,
                            va("Referer: ioQ3://%s\r\n", NET_AdrToString(clc.serverAddress)), -1,
                            INTERNET_FLAG_HYPERLINK |
                                INTERNET_FLAG_NO_CACHE_WRITE |
                                INTERNET_FLAG_NO_COOKIES |
                                INTERNET_FLAG_NO_UI |
                                INTERNET_FLAG_RESYNCHRONIZE |
                                INTERNET_FLAG_RELOAD,
                            0);

    DropIf(hUrl == NULL, "InternetOpenUrlA failed %lu", GetLastError());

    success = HttpQueryInfo(hUrl, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &httpCode, &len, &zero);
    DropIf(!success, "Get HTTP_QUERY_STATUS_CODE failed %lu", GetLastError());

    DropIf(httpCode >= 400, "HTTP code %lu", httpCode);
    DropIf(httpCode != 200, "Unhandled HTTP code %lu", httpCode);

    success = HttpQueryInfo(hUrl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentLength, &len, &zero);
    DropIf(!success, "Get HTTP_QUERY_CONTENT_LENGTH failed %lu", GetLastError());

    clc.downloadSize = (int)contentLength;
    Cvar_SetValue("cl_downloadSize", clc.downloadSize);
}

/*
=================
CL_HTTP_PerformDownload
=================
*/
qboolean CL_HTTP_PerformDownload(void)
{
    static BYTE readBuffer[256 * 1024];
    DWORD bytesRead = 0;
    BOOL success;

    DropIf(hUrl == NULL, "hUrl is NULL");

    success = InternetReadFile(hUrl, readBuffer, sizeof(readBuffer), &bytesRead);
    DropIf(!success, "InternetReadFile failed %lu", GetLastError());

    if (bytesRead > 0)
    {
        clc.downloadCount += bytesRead;
        Cvar_SetValue("cl_downloadCount", clc.downloadCount);

        int bytesWritten = FS_Write(readBuffer, bytesRead, clc.download);
        DropIf(bytesWritten != bytesRead, "bytesWritten != bytesRead");

        return qfalse;
    }

    InternetCloseHandle(hUrl);
    return qtrue;
}

#endif /* USE_HTTP */

/*
===========================================================================
Copyright (C) 2006 Tony J. White (tjw@tjw.org)
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

#ifdef USE_MONGOOSE

#include "client.h"
#include "mongoose.h"

#define TIMEOUT_SECONDS 10
#define MAX_REDIRECTS 5

#define CDATA_INDEX_HEADERS_PARSED 0
#define CDATA_INDEX_TIME_SINCE_DATA 1

typedef struct
{
    struct mg_mgr manager;
    struct mg_connection *connection;
    enum
    {
        PHASE_STARTED,
        PHASE_FINISHED,
        PHASE_ERROR,
    } phase;
    int remainingRedirects;
    char errorMessage[256];
} mongoose_state_t;

static mongoose_state_t *state(void)
{
    assert(clc.httpState != NULL);
    return (mongoose_state_t *)clc.httpState;
}

/*
=================
CL_Mongoose_ReportErrorVA
=================
*/
__attribute__((format(printf, 1, 0))) static void CL_Mongoose_ReportErrorVA(const char *fmt, va_list argptr)
{
    state()->phase = PHASE_ERROR;
    Q_vsnprintf(state()->errorMessage, sizeof(state()->errorMessage), fmt, argptr);
}

/*
=================
CL_Mongoose_ReportErrorIf
=================
*/
__attribute__((format(printf, 2, 3))) static qboolean CL_Mongoose_ReportErrorIf(qboolean condition, const char *fmt, ...)
{
    if (!condition)
        return qfalse;

    va_list argptr;
    va_start(argptr, fmt);
    CL_Mongoose_ReportErrorVA(fmt, argptr);
    va_end(argptr);

    return qtrue;
}

/*
=================
CL_Mongoose_ReportError
=================
*/
__attribute__((format(printf, 1, 2))) static void CL_Mongoose_ReportError(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    CL_Mongoose_ReportErrorVA(fmt, argptr);
    va_end(argptr);
}

/*
=================
CL_Mongoose_WriteFile
=================
*/
static qboolean CL_Mongoose_WriteFile(const void *buffer, int length)
{
    if (length <= 0)
        return qtrue;

    int written = FS_Write(buffer, length, clc.download);
    if (CL_Mongoose_ReportErrorIf(written != length, "File write error %d != %d", written, length))
        return qfalse;

    clc.downloadCount += length;
    Cvar_SetValue("cl_downloadCount", clc.downloadCount);

    return qtrue;
}

/*
=================
CL_Mongoose_SetTimeSinceData
=================
*/
static void CL_Mongoose_SetTimeSinceData(struct mg_connection *c, int32_t time)
{
    memcpy(c->data + CDATA_INDEX_TIME_SINCE_DATA, &time, sizeof(int32_t));
}

/*
=================
CL_Mongoose_GetTimeSinceData
=================
*/
static int32_t CL_Mongoose_GetTimeSinceData(struct mg_connection *c)
{
    int32_t time;
    memcpy(&time, c->data + CDATA_INDEX_TIME_SINCE_DATA, sizeof(int32_t));
    return time;
}

/*
=================
CL_Mongoose_EventHandler
=================
*/
static void CL_Mongoose_EventHandler(struct mg_connection *c, int ev, void *ev_data)
{
    switch (ev)
    {
    case MG_EV_OPEN:
        CL_Mongoose_SetTimeSinceData(c, cls.realtime);
        break;

    case MG_EV_POLL:
    {
        int timeSinceData = cls.realtime - CL_Mongoose_GetTimeSinceData(c);
        CL_Mongoose_ReportErrorIf(timeSinceData >= (TIMEOUT_SECONDS * 1000), "Timeout");
        break;
    }

    case MG_EV_CONNECT:
    {
        struct mg_str host = mg_url_host(clc.downloadURL);

        if (mg_url_is_ssl(clc.downloadURL))
        {
            struct mg_tls_opts opts = {.name = host};
            mg_tls_init(c, &opts);
        }

        mg_printf(c,
                  "GET %s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "User-Agent: %s\r\n"
                  "Accept: */*\r\n"
                  "Connection: close\r\n"
                  "\r\n",
                  mg_url_uri(clc.downloadURL), (int)host.len, host.buf, Q3_VERSION);
        break;
    }

    case MG_EV_READ:
        CL_Mongoose_SetTimeSinceData(c, cls.realtime);

        if (c->data[CDATA_INDEX_HEADERS_PARSED] == qfalse)
        {
            struct mg_http_message hm;
            int n = mg_http_parse((char *)c->recv.buf, c->recv.len, &hm);
            if (CL_Mongoose_ReportErrorIf(n <= 0, "Bad HTTP response"))
            {
                break;
            }

            int status = mg_http_status(&hm);

            // Handle redirects
            if (status >= 300 && status < 400)
            {
                if (CL_Mongoose_ReportErrorIf(state()->remainingRedirects <= 0, "Too many redirects"))
                {
                    break;
                }

                struct mg_str *location = mg_http_get_header(&hm, "Location");
                if (CL_Mongoose_ReportErrorIf(location == NULL, "Redirect has no destination"))
                {
                    break;
                }

                if (mg_match(*location, mg_str("#://#"), NULL))
                {
                    // Fully qualified URL
                    Com_sprintf(clc.downloadURL, sizeof(clc.downloadURL),
                                "%.*s", (int)location->len, location->buf);
                }
                else if (location->buf[0] == '/')
                {
                    // Path relative to base URL
                    const char *p = strstr(clc.downloadURL, "://");
                    const char *host = p + 3;
                    const char *uri = strchr(host, '/');

                    size_t baseLength = uri != NULL ? (uri - clc.downloadURL) : strlen(clc.downloadURL);

                    Com_sprintf(clc.downloadURL + baseLength, sizeof(clc.downloadURL) - baseLength, "%.*s",
                                (int)location->len, location->buf);
                }
                else
                {
                    // Something else
                    CL_Mongoose_ReportError("Malformed/unsupported redirect destination");
                }

                c->is_draining = 1;
                state()->remainingRedirects--;

                break;
            }

            if (CL_Mongoose_ReportErrorIf(status >= 400, "HTTP error %d", status))
            {
                break;
            }

            struct mg_str *content_length = mg_http_get_header(&hm, "Content-Length");
            if (content_length)
            {
                clc.downloadSize = atoi(content_length->buf);
                Cvar_SetValue("cl_downloadSize", clc.downloadSize);
            }

            if (CL_Mongoose_WriteFile(c->recv.buf + n, c->recv.len - n))
            {
                c->recv.len = 0;
                c->data[CDATA_INDEX_HEADERS_PARSED] = qtrue;
            }
        }
        else if (CL_Mongoose_WriteFile(c->recv.buf, c->recv.len))
        {
            c->recv.len = 0;
        }

        break;

    case MG_EV_ERROR:
        CL_Mongoose_ReportError("%s", (char *)ev_data);
        break;

    case MG_EV_CLOSE:
        if (state()->phase != PHASE_ERROR)
            state()->phase = PHASE_FINISHED;
        break;
    }
}

/*
=================
CL_HTTP_Init
=================
*/
qboolean CL_HTTP_Init(void)
{
    if (clc.httpState != NULL)
        return qtrue;

    clc.httpState = (mongoose_state_t *)Z_Malloc(sizeof(mongoose_state_t));
    mg_mgr_init(&state()->manager);

    return qtrue;
}

/*
=================
CL_Mongoose_Cleanup
=================
*/
static void CL_Mongoose_Cleanup(void)
{
    if (state()->connection)
    {
        state()->connection->is_closing = 1;
        state()->connection = NULL;
    }

    if (clc.download)
    {
        FS_FCloseFile(clc.download);
        clc.download = 0;
        FS_Remove(clc.downloadTempName);
    }
}

/*
=================
CL_HTTP_Shutdown
=================
*/
void CL_HTTP_Shutdown(void)
{
    if (clc.httpState == NULL)
        return;

    CL_Mongoose_Cleanup();
    mg_mgr_free(&state()->manager);

    Z_Free(clc.httpState);
    clc.httpState = NULL;
}

/*
=================
CL_HTTP_BeginDownload
=================
*/
void CL_HTTP_BeginDownload(const char *localName, const char *remoteURL)
{
    Com_Printf("URL: %s\n", remoteURL);
    Com_DPrintf("***** CL_HTTP_BeginDownload *****\n"
                "Localname: %s\n"
                "RemoteURL: %s\n"
                "****************************\n",
                localName, remoteURL);

    Q_strncpyz(clc.downloadURL, remoteURL, sizeof(clc.downloadURL));
    Q_strncpyz(clc.downloadName, localName, sizeof(clc.downloadName));
    Com_sprintf(clc.downloadTempName, sizeof(clc.downloadTempName),
                "%s.tmp", localName);

    Cvar_Set("cl_downloadName", localName);
    Cvar_Set("cl_downloadSize", "0");
    Cvar_Set("cl_downloadCount", "0");
    Cvar_SetValue("cl_downloadTime", cls.realtime);

    // mg_url_port returning 0 effectively means the URL is invalid
    if (mg_url_port(clc.downloadURL) == 0)
    {
        Com_Error(ERR_DROP, "CL_HTTP_BeginDownload: Malformed URL %s", clc.downloadURL);
        return;
    }

    clc.downloadBlock = 0; // Starting new file
    clc.downloadCount = 0;
    state()->phase = PHASE_STARTED;
    state()->remainingRedirects = MAX_REDIRECTS;

    clc.download = FS_SV_FOpenFileWrite(clc.downloadTempName);
    if (!clc.download)
    {
        CL_Mongoose_Cleanup();
        Com_Error(ERR_DROP, "CL_HTTP_BeginDownload: failed to open %s for writing",
                  clc.downloadTempName);
        return;
    }

    state()->connection = mg_connect(&state()->manager, remoteURL, CL_Mongoose_EventHandler, NULL);
    if (!state()->connection)
    {
        CL_Mongoose_Cleanup();
        Com_Error(ERR_DROP, "CL_HTTP_BeginDownload: failed to connect to %s", remoteURL);
        return;
    }

    CL_Mongoose_SetTimeSinceData(state()->connection, cls.realtime);

    if (!(clc.sv_allowDownload & DLF_NO_DISCONNECT) && !clc.disconnectedForHttpDownload)
    {
        CL_AddReliableCommand("disconnect", qtrue);
        CL_WritePacket();
        CL_WritePacket();
        CL_WritePacket();
        clc.disconnectedForHttpDownload = qtrue;
    }
}

/*
=================
CL_HTTP_PerformDownload

Called once per frame
=================
*/
void CL_HTTP_PerformDownload(void)
{
    if (state()->connection == NULL)
    {
        return;
    }

    // For some reason, TLS connections are only processed in 16Kb chunks, even
    // when MG_IO_SIZE is set to something larger, the net result of which is
    // that the download speed is quite low. We can workaround this by just
    // running the poll function multiple times though, hence the loop. The
    // number of loops was arrived at empirically; it may need to be reduced to
    // be performant on slower hardware.
    for (int i = 0; i < 25; i++)
    {
        // 2nd parameter 0 parameter means don't block
        mg_mgr_poll(&state()->manager, 0);
    }

    if (state()->phase == PHASE_ERROR)
    {
        CL_Mongoose_Cleanup();
        Com_Error(ERR_DROP, "Download Error: %s URL: %s",
                  state()->errorMessage, clc.downloadURL);
        return;
    }

    if (state()->phase == PHASE_FINISHED)
    {
        if (clc.downloadCount > 0)
        {
            FS_FCloseFile(clc.download);
            clc.download = 0;
            state()->connection = NULL;

            FS_SV_Rename(clc.downloadTempName, clc.downloadName, qfalse);
            clc.downloadRestart = qtrue;
            CL_NextDownload();
        }
        else
        {
            // We've finished, but haven't downloaded anything, probably a redirect, try again
            state()->connection = mg_connect(&state()->manager, clc.downloadURL, CL_Mongoose_EventHandler, NULL);
            state()->phase = PHASE_STARTED;
            CL_Mongoose_SetTimeSinceData(state()->connection, cls.realtime);
        }
    }
}

#endif /* USE_MONGOOSE */
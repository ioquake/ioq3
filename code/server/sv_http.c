/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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

/**
 * Basic usage
 * ===========
 *
 * /status endpoint is printing information about the current running game
 * /pk3    is the endpoints for downloading currently referenced pk3 files (configure sv_dlURL to
 *         point to http://yourserver:8080/pk3 and activate sv_allowDownload)
 * /maps   prints a list of maps that the server knows and shows a screenshot for it (only jpeg
 *         and png - no tga)
 *
 * Encrpytion/Authentication
 * =========================
 *
 * There is no encryption support in this implementation. Consider the use of a reverse
 * proxy if you really need this. For those who need this, there is a token validation built-in.
 * But for obvious reasons it doesn't make much sense to use a token validation in combination
 * with a non-encrypted connection. This could get useful if someone would implement a voting
 * system or rcon api or ui - for the currently used endpoints that are read-only it might not
 * make much sense ;)
 *
 * New Cvars
 * =========
 *
 * sv_httpServerPort: the default port is 8080
 * sv_httpServerIP: listen on all interfaces by default: 0.0.0.0
 * sv_httpServerBearer: not activated by default - some services might need the token for authentication
 */

#ifdef USE_HTTP_SERVER

#include "server.h"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2spi.h>
#define network_return int

#else

#define SOCKET int
#include <sys/select.h>
#define network_return ssize_t
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
#include <signal.h>
#define closesocket close
#define INVALID_SOCKET (-1)

#endif

cvar_t *sv_httpServerPort;
cvar_t *sv_httpServerIP;
cvar_t *sv_httpServerBearer;

static SOCKET httpSocket = INVALID_SOCKET;
static fd_set httpReadFDSet;
static fd_set httpWriteFDSet;
static const size_t httpMaxRequestBytes = 1 * 1024 * 1024;
static const char *httpImageExtensions[] = {"jpg", "jpeg", "png", NULL};

qboolean Sys_StringToSockaddr(const char *s, struct sockaddr *sadr, int sadr_len, sa_family_t family);

typedef enum {
	HTTP_STATUS_Unknown = 0,
	HTTP_STATUS_Ok = 200,
	HTTP_STATUS_Created = 201,
	HTTP_STATUS_Accepted = 202,
	HTTP_STATUS_BadRequest = 400,
	HTTP_STATUS_Unauthorized = 401,
	HTTP_STATUS_Forbidden = 403,
	HTTP_STATUS_NotFound = 404,
	HTTP_STATUS_RequestUriTooLong = 414,
	HTTP_STATUS_TooManyRequests = 429,
	HTTP_STATUS_InternalServerError = 500,
	HTTP_STATUS_NotImplemented = 501,
	HTTP_STATUS_BadGateway = 502,
	HTTP_STATUS_ServiceUnavailable = 503,
	HTTP_STATUS_GatewayTimeout = 504,
	HTTP_STATUS_HttpVersionNotSupported = 505
} httpStatus_t;

typedef enum { HTTP_METHOD_GET, HTTP_METHOD_POST, HTTP_METHOD_NOT_SUPPORTED } httpMethod_t;

typedef struct httpKeyValue_s {
	const char *key;
	qboolean freeValue;
	char *value;
} httpKeyValue_t;

#define HTTP_MAX_HEADERS 32
#define HTTP_MAX_QUERY_PARAMS 16

typedef struct httpProtocol_s {
	httpKeyValue_t headers[HTTP_MAX_HEADERS];
	uint8_t *buf;
	size_t bufSize;
	qboolean _valid;
	const char *protocolVersion;
	const char *content;
	int contentLength;
} httpProtocol_t;

typedef struct httpResponse_s {
	httpProtocol_t proto;
	httpStatus_t status;
	const char *statusText;
	// the memory is managed by the server and freed after the response was sent.
	const char *body;
	size_t bodySize;
	// if the route handler sets this to false, the memory is not freed. Can be useful for static content
	// like error pages.
	qboolean freeBody;
	qboolean file;
} httpResponse_t;

typedef struct httpRequest_s {
	httpProtocol_t proto;
	httpKeyValue_t query[HTTP_MAX_QUERY_PARAMS];
	httpMethod_t method;
	const char *path;
} httpRequest_t;

typedef struct httpClient_s {
	SOCKET socket;

	uint8_t *request;
	size_t requestLength;

	char *response;
	size_t responseLength;
	size_t alreadySent;
} httpClient_t;

typedef qboolean (*httpCallback_t)(const httpRequest_t *request, httpResponse_t *response);

typedef struct httpEndpoint_s {
	const char *path;
	httpCallback_t callback;
	qboolean checkBearerToken;
} httpEndpoint_t;

#define HTTP_MAX_CLIENTS 32
static httpClient_t httpClients[HTTP_MAX_CLIENTS];

#define HTTP_MAX_ENDPOINTS 32
static httpEndpoint_t httpEndpoints[HTTP_MAX_ENDPOINTS];

static const char *HTTP_HEADER_CONTENT_TYPE = "Content-Type";
static const char *HTTP_HEADER_CACHE_CONTROL = "Cache-Control";
static const char *HTTP_HEADER_CONTENT_DISPOSITION = "Content-Disposition";
static const char *HTTP_HEADER_CONTENT_LENGTH = "Content-length";

static const char *HTTP_MIMETYPE_TEXT_PLAIN = "text/plain";
static const char *HTTP_MIMETYPE_TEXT_HTML = "text/html";
static const char *HTTP_MIMETYPE_IMAGE_JPEG = "image/jpeg";
static const char *HTTP_MIMETYPE_IMAGE_PNG = "image/png";
static const char *HTTP_MIMETYPE_APPLICATION_OCTET_STREAM = "application/octet-stream";

static const char *HTTP_ToStatusString(httpStatus_t status) {
	if (status == HTTP_STATUS_InternalServerError) {
		return "Internal Server Error";
	} else if (status == HTTP_STATUS_Ok) {
		return "OK";
	} else if (status == HTTP_STATUS_NotFound) {
		return "Not Found";
	} else if (status == HTTP_STATUS_NotImplemented) {
		return "Not Implemented";
	}
	return "Unknown";
}

/**
 * If you have an endpoint that is secured by the bearer token, you can also have a fallback for the same
 * path without the need for the token. But first you have to register the one with the token validation,
 * then the one without validation.
 */
static qboolean HTTP_Register(httpMethod_t method, const char *path, httpCallback_t callback, qboolean checkBearerToken) {
	int i;

	for (i = 0; i < HTTP_MAX_ENDPOINTS; ++i) {
		if (httpEndpoints[i].path == NULL) {
			Com_Printf("Registered endpoint '%s'\n", path);
			httpEndpoints[i].path = path;
			httpEndpoints[i].callback = callback;
			httpEndpoints[i].checkBearerToken = checkBearerToken;
			return qtrue;
		}
	}
	Com_Printf("Failed to register endpoint '%s'\n", path);

	return qfalse;
}

/**
 * Returns the first header entry for the given key
 */
static const httpKeyValue_t *HTTP_GetHeader(const httpProtocol_t *proto, const char *key) {
	int i;

	for (i = 0; i < ARRAY_LEN(proto->headers); ++i) {
		if (proto->headers[i].key == NULL) {
			break;
		}
		if (!Q_stricmp(proto->headers[i].key, key)) {
			return &proto->headers[i];
		}
	}

	return NULL;
}

static char *getBeforeToken(char **buffer, const char *token, size_t bufferSize) {
	char *begin;
	int length;

	if (bufferSize <= 0) {
		return NULL;
	}
	begin = *buffer;
	length = (int)strlen(token);
	while (**buffer) {
		if (bufferSize <= 0) {
			return NULL;
		}
		if (strncmp(*buffer, token, length) == 0) {
			**buffer = '\0';
			*buffer += length;
			return begin;
		}
		++(*buffer);
		--bufferSize;
	}
	*buffer = begin;

	return NULL;
}

static qboolean HTTP_SetHeader(httpProtocol_t *proto, const char *key, const char *value, qboolean freeValue) {
	int i;

	for (i = 0; i < ARRAY_LEN(proto->headers); ++i) {
		if (proto->headers[i].key == NULL || !Q_stricmp(proto->headers[i].key, key)) {
			proto->headers[i].key = key;
			proto->headers[i].value = (char *)value;
			proto->headers[i].freeValue = freeValue;
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean HTTP_AddQueryParam(httpRequest_t *request, const char *key, const char *value) {
	int i;

	for (i = 0; i < ARRAY_LEN(request->query); ++i) {
		if (request->query[i].key == NULL) {
			request->query[i].key = key;
			request->query[i].value = (char *)value;
			request->query[i].freeValue = qfalse;
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean HTTP_BuildHeaderBuffer(char *headersBuffer, size_t len, const httpKeyValue_t *headers, int size) {
	char *headersP = headersBuffer;
	size_t headersSize = len;
	int i;

	for (i = 0; i < size; ++i) {
		const httpKeyValue_t *h = &headers[i];
		int written;
		if (h->key == NULL) {
			continue;
		}
		written = Com_sprintf(headersP, headersSize, "%s: %s\r\n", h->key, h->value);
		if (written >= headersSize) {
			return qfalse;
		}
		headersSize -= written;
		headersP += written;
	}
	return qtrue;
}

static void HTTP_ResponseSetText(httpResponse_t *response, const char *body) {
	response->body = body;
	response->bodySize = strlen(body);
	response->freeBody = qfalse;
	if (!HTTP_GetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE)) {
		HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_TEXT_PLAIN, qfalse);
	}
}

static qboolean HTTP_InsertClient(SOCKET clientSocket) {
	int i;

	for (i = 0; i < HTTP_MAX_CLIENTS; ++i) {
		if (httpClients[i].socket == INVALID_SOCKET) {
			httpClients[i].socket = clientSocket;
#ifdef O_NONBLOCK
			fcntl(clientSocket, F_SETFL, O_NONBLOCK);
#endif
#ifdef _WIN32
			{
				unsigned long mode = 1;
				ioctlsocket(clientSocket, FIONBIO, &mode);
			}
#endif
			return qtrue;
		}
	}

	return qfalse;
}

static void HTTP_RemoveClient(httpClient_t *client) {
	SOCKET clientSocket = client->socket;
	if (clientSocket == INVALID_SOCKET) {
		return;
	}
	FD_CLR(clientSocket, &httpReadFDSet);
	FD_CLR(clientSocket, &httpWriteFDSet);
	closesocket(clientSocket);
	free(client->request);
	free(client->response);

	memset(client, 0, sizeof(*client));
	client->socket = INVALID_SOCKET;
}

static qboolean HTTP_SendMessage(httpClient_t *client) {
	int remaining = (int)client->responseLength - (int)client->alreadySent;
	network_return sent;
	const char *p;

	if (remaining <= 0) {
		return qfalse;
	}
	p = client->response + client->alreadySent;
	sent = send(client->socket, p, remaining, 0);
	if (sent < 0) {
		Com_DPrintf("Failed to send to the client");
		return qfalse;
	}
	if (sent == 0) {
		return qtrue;
	}
	remaining -= sent;
	client->alreadySent += sent;
	return remaining > 0;
}

static qboolean HTTP_IsClientFinished(httpClient_t *client) {
	if (client->response == NULL) {
		return qfalse;
	}
	return client->responseLength == client->alreadySent;
}

static const char *HTTP_GetErrorPage(httpStatus_t status) {
	return HTTP_ToStatusString(status);
}

static void HTTP_ClientSetResponse(httpClient_t *client, char *responseBuf, size_t responseBufLength) {
	client->response = responseBuf;
	client->responseLength = responseBufLength;
	client->alreadySent = 0u;
}

static void HTTP_AssembleError(httpClient_t *client, httpStatus_t status) {
	char buf[512];
	int responseSize;
	char *responseBuf;
	const char *errorPage;

	Com_sprintf(buf, sizeof(buf),
				"HTTP/1.1 %i %s\r\n"
				"Connection: close\r\n"
				"Server: " Q3_VERSION "\r\n"
				"\r\n",
				(int)status, HTTP_ToStatusString(status));

	errorPage = HTTP_GetErrorPage(status);

	responseSize = (int)strlen(errorPage) + (int)strlen(buf);
	responseBuf = (char *)malloc(responseSize + 1);
	Com_sprintf(responseBuf, responseSize + 1, "%s%s", buf, errorPage);
	HTTP_ClientSetResponse(client, responseBuf, responseSize);
	FD_SET(client->socket, &httpWriteFDSet);
}

static void HTTP_AssembleResponse(httpClient_t *client, const httpResponse_t *response) {
	char headers[2048];
	size_t responseSize;
	int headerSize;
	char buf[4096];
	char *responseBuf;

	if (!HTTP_BuildHeaderBuffer(headers, sizeof(headers), response->proto.headers, HTTP_MAX_HEADERS)) {
		HTTP_AssembleError(client, HTTP_STATUS_InternalServerError);
		return;
	}

	headerSize = Com_sprintf(buf, sizeof(buf),
							 "HTTP/1.1 %i %s\r\n"
							 "Content-length: %u\r\n"
							 "%s"
							 "\r\n",
							 (int)response->status, HTTP_ToStatusString(response->status),
							 (unsigned int)response->bodySize, headers);
	if (headerSize >= sizeof(buf)) {
		HTTP_AssembleError(client, HTTP_STATUS_InternalServerError);
		return;
	}

	responseSize = response->bodySize + strlen(buf);
	responseBuf = (char *)malloc(responseSize);
	memcpy(responseBuf, buf, headerSize);
	memcpy(responseBuf + headerSize, response->body, response->bodySize);
	HTTP_ClientSetResponse(client, responseBuf, responseSize);
	FD_SET(client->socket, &httpWriteFDSet);
}

static size_t HTTP_ParseRemainingBufSize(const httpProtocol_t *proto, const char *bufPos) {
	size_t alreadyRead;
	size_t remaining;

	if (bufPos < (const char *)proto->buf) {
		return 0u;
	}
	if (bufPos >= (const char *)proto->buf + proto->bufSize) {
		return 0u;
	}
	alreadyRead = (size_t)((uint8_t *)bufPos - proto->buf);
	remaining = proto->bufSize - alreadyRead;
	return remaining;
}

static qboolean HTTP_ParseHeaders(httpProtocol_t *proto, char **bufPos) {
	char *hdrPos = getBeforeToken(bufPos, "\r\n\r\n", HTTP_ParseRemainingBufSize(proto, *bufPos));
	int hdrSize;
	if (hdrPos == NULL) {
		return qfalse;
	}

	// re-add one newline to simplify the code
	// for key/value parsing of the header
	hdrSize = (int)strlen(hdrPos);
	hdrPos[hdrSize + 0] = '\r';
	hdrPos[hdrSize + 1] = '\n';
	hdrPos[hdrSize + 2] = '\0';

	for (;;) {
		const char *var;
		const char *value;

		char *headerEntry = getBeforeToken(&hdrPos, "\r\n", HTTP_ParseRemainingBufSize(proto, hdrPos));
		if (headerEntry == NULL) {
			break;
		}
		var = getBeforeToken(&headerEntry, ": ", HTTP_ParseRemainingBufSize(proto, headerEntry));
		value = headerEntry;
		HTTP_SetHeader(proto, var, value, qfalse);
	}
	return qtrue;
}

static char *HTTP_GetHeaderLine(httpProtocol_t *proto, char **buffer) {
	const size_t remainingSize = HTTP_ParseRemainingBufSize(proto, *buffer);
	return getBeforeToken(buffer, "\r\n", remainingSize);
}

static qboolean HTTP_ParseRequest(httpRequest_t *request, const uint8_t *buf, size_t bufSize) {
	char *statusLine;
	char *bufPos;
	char *methodStr;
	char *queryString;
	size_t remainingSize;

	if (buf == NULL || bufSize == 0) {
		return qfalse;
	}
	bufPos = (char *)buf;

	statusLine = HTTP_GetHeaderLine(&request->proto, &bufPos);
	if (statusLine == NULL) {
		return qfalse;
	}
	remainingSize = HTTP_ParseRemainingBufSize(&request->proto, bufPos);
	methodStr = getBeforeToken(&statusLine, " ", remainingSize);
	if (methodStr == NULL) {
		return qfalse;
	}

	if (!strcmp(methodStr, "GET")) {
		request->method = HTTP_METHOD_GET;
	} else if (!strcmp(methodStr, "POST")) {
		request->method = HTTP_METHOD_POST;
	} else {
		request->method = HTTP_METHOD_NOT_SUPPORTED;
		return qfalse;
	}

	remainingSize = HTTP_ParseRemainingBufSize(&request->proto, statusLine);
	queryString = getBeforeToken(&statusLine, " ", remainingSize);
	if (queryString == NULL) {
		return qfalse;
	}
	request->proto.protocolVersion = statusLine;
	if (request->proto.protocolVersion == NULL) {
		return qfalse;
	}

	remainingSize = HTTP_ParseRemainingBufSize(&request->proto, queryString);
	request->path = getBeforeToken(&queryString, "?", remainingSize);
	if (request->path == NULL) {
		request->path = queryString;
	} else {
		char *queryStringPos = queryString;
		qboolean last = qfalse;
		for (;;) {
			char *key;
			char *paramValue;
			char *value;

			remainingSize = HTTP_ParseRemainingBufSize(&request->proto, queryStringPos);
			paramValue = getBeforeToken(&queryStringPos, "&", remainingSize);
			if (paramValue == NULL) {
				paramValue = queryStringPos;
				last = qtrue;
			}

			remainingSize = HTTP_ParseRemainingBufSize(&request->proto, paramValue);
			key = getBeforeToken(&paramValue, "=", remainingSize);
			value = paramValue;
			if (key == NULL) {
				static const char *EMPTY = "";
				key = paramValue;
				value = (char *)EMPTY;
			}
			HTTP_AddQueryParam(request, key, value);

			if (last) {
				break;
			}
		}
	}

	if (!HTTP_ParseHeaders(&request->proto, &bufPos)) {
		return qfalse;
	}

	request->proto.content = bufPos;
	request->proto.contentLength = HTTP_ParseRemainingBufSize(&request->proto, bufPos);

	if (request->method == HTTP_METHOD_GET) {
		request->proto._valid = request->proto.contentLength == 0;
	} else if (request->method == HTTP_METHOD_POST) {
		const httpKeyValue_t *header = HTTP_GetHeader(&request->proto, HTTP_HEADER_CONTENT_LENGTH);
		if (!header) {
			request->proto._valid = qfalse;
		} else {
			request->proto._valid = request->proto.contentLength == atoi(header->value);
		}
	}
	return request->proto._valid;
}

static qboolean HTTP_Route(httpClient_t *client, const httpRequest_t *request, httpResponse_t *response) {
	int i;
	qboolean unauthorized = qfalse;

	for (i = 0; i < HTTP_MAX_ENDPOINTS; ++i) {
		if (httpEndpoints[i].path == NULL) {
			break;
		}
		if (!Q_stricmpn(httpEndpoints[i].path, request->path, (int)strlen(httpEndpoints[i].path))) {
			if (httpEndpoints[i].checkBearerToken && sv_httpServerBearer->string && sv_httpServerBearer->string[0] != '\0') {
				unauthorized = qtrue;
				// other endpoint with same path might not need registration
				continue;
			}
			if (!httpEndpoints[i].callback(request, response)) {
				HTTP_AssembleError(client, HTTP_STATUS_InternalServerError);
			}
			return qtrue;
		}
	}
	if (unauthorized) {
		HTTP_AssembleError(client, HTTP_STATUS_Unauthorized);
		return qtrue;
	}
	Com_DPrintf("Could not find mapping for path '%s'\n", request->path);
	return qfalse;
}

static void HTTP_ProtoFree(httpProtocol_t *proto) {
	int i;

	for (i = 0; i < HTTP_MAX_HEADERS; ++i) {
		if (!proto->headers[i].freeValue) {
			continue;
		}
		free(proto->headers[i].value);
		proto->headers[i].value = NULL;
	}

	free(proto->buf);
	proto->buf = NULL;
}

static void HTTP_ResponseFree(httpResponse_t *response) {
	if (response->freeBody) {
		if (response->file) {
			FS_FreeFile((void*)response->body);
		} else {
			free((char *)response->body);
		}
		response->body = NULL;
	}
	HTTP_ProtoFree(&response->proto);
}

static void HTTP_RequestFree(httpRequest_t *request) {
	HTTP_ProtoFree(&request->proto);
}

void HTTP_Frame(void) {
	fd_set readFDsOut;
	fd_set writeFDsOut;
	struct timeval tv;
	int i;
	int ready;

	memcpy(&readFDsOut, &httpReadFDSet, sizeof(readFDsOut));
	memcpy(&writeFDsOut, &httpWriteFDSet, sizeof(writeFDsOut));

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	ready = select(FD_SETSIZE, &readFDsOut, &writeFDsOut, NULL, &tv);
	if (ready < 0) {
		return;
	}
	if (httpSocket != INVALID_SOCKET && FD_ISSET(httpSocket, &readFDsOut)) {
		const SOCKET clientSocket = accept(httpSocket, NULL, NULL);
		if (clientSocket != INVALID_SOCKET) {
			if (HTTP_InsertClient(clientSocket)) {
				FD_SET(clientSocket, &httpReadFDSet);
			} else {
				closesocket(clientSocket);
			}
		}
	}

	for (i = 0; i < HTTP_MAX_CLIENTS; ++i) {
		httpClient_t *client = &httpClients[i];
		const SOCKET clientSocket = client->socket;
		uint8_t recvBuf[2048];
		network_return len;
		httpRequest_t request;
		httpResponse_t response;
		uint8_t *mem;

		if (clientSocket == INVALID_SOCKET) {
			continue;
		}

		if (FD_ISSET(clientSocket, &writeFDsOut)) {
			if (!HTTP_SendMessage(client) || HTTP_IsClientFinished(client)) {
				HTTP_RemoveClient(client);
			}
			continue;
		}

		if (!FD_ISSET(clientSocket, &readFDsOut)) {
			continue;
		}

		len = recv(clientSocket, (char *)recvBuf, sizeof(recvBuf) - 1, 0);
		if (len < 0) {
			HTTP_RemoveClient(client);
			continue;
		}
		if (len == 0) {
			continue;
		}

		client->request = (uint8_t *)realloc(client->request, client->requestLength + len);
		memcpy(client->request + client->requestLength, recvBuf, len);
		client->requestLength += len;

		if (client->requestLength == 0) {
			continue;
		}

		// GET / HTTP/1.1\r\n\r\n
		if (client->requestLength < 18) {
			continue;
		}

		if (memcmp(client->request, "GET", 3) != 0 && memcmp(client->request, "POST", 4) != 0) {
			FD_CLR(clientSocket, &httpReadFDSet);
			FD_CLR(clientSocket, &readFDsOut);
			HTTP_AssembleError(client, HTTP_STATUS_NotImplemented);
			continue;
		}

		if (client->requestLength > httpMaxRequestBytes) {
			FD_CLR(clientSocket, &httpReadFDSet);
			FD_CLR(clientSocket, &readFDsOut);
			HTTP_AssembleError(client, HTTP_STATUS_InternalServerError);
			continue;
		}

		memset(&request, 0, sizeof(request));
		request.proto.contentLength = -1;
		mem = (uint8_t *)malloc(client->requestLength);
		memcpy(mem, client->request, client->requestLength);
		request.proto.buf = mem;
		request.proto.bufSize = client->requestLength;
		if (!HTTP_ParseRequest(&request, mem, client->requestLength)) {
			Com_DPrintf("Failed to parse request\n");
			HTTP_AssembleError(client, HTTP_STATUS_InternalServerError);
			HTTP_RequestFree(&request);
			continue;
		}

		FD_CLR(clientSocket, &httpReadFDSet);
		FD_CLR(clientSocket, &readFDsOut);

		memset(&response, 0, sizeof(response));
		response.proto.contentLength = -1;
		response.freeBody = qtrue;
		response.status = HTTP_STATUS_Ok;

		if (!HTTP_Route(client, &request, &response)) {
			HTTP_AssembleError(client, HTTP_STATUS_NotFound);
		} else if (client->responseLength == 0) {
			HTTP_AssembleResponse(client, &response);
		}
		HTTP_ResponseFree(&response);
		HTTP_RequestFree(&request);
	}
}

static qboolean HTTP_ResponseSetPk3File(httpResponse_t *response, const char *filepath) {
	long length;
	fileHandle_t fileHandle;

	length = FS_SV_FOpenFileRead(filepath, &fileHandle);
	if (length <= 0) {
		Com_Printf("Failed to load %s\n", filepath);
		return qfalse;
	}
	response->body = (char*)malloc(length);
	if (FS_Read((void*)response->body, (int)length, fileHandle) != (int)length) {
		Com_Printf("Failed to read %s\n", filepath);
		free((void*)response->body);
		response->body = NULL;
		return qfalse;
	}

	HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_APPLICATION_OCTET_STREAM, qfalse);
	response->file = qfalse; // use normal free
	response->freeBody = qtrue;
	response->bodySize = length;
	return qtrue;
}

static qboolean HTTP_ResponseSetFile(httpResponse_t *response, const char *filepath) {
	const char *extension = COM_GetExtension(filepath);
	long length;
	length = FS_ReadFile(filepath, (void**)&response->body);
	if (length <= 0) {
		Com_Printf("Failed to load %s\n", filepath);
		return qfalse;
	}
	if (!Q_stricmp(extension, "jpg") ||!Q_stricmp(extension, "jpeg")) {
		HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_IMAGE_JPEG, qfalse);
	} else if (!Q_stricmp(extension, "png")) {
		HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_IMAGE_PNG, qfalse);
	} else {
		HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_APPLICATION_OCTET_STREAM, qfalse);
	}
	HTTP_SetHeader(&response->proto, HTTP_HEADER_CACHE_CONTROL, "public, max-age=31536000", qfalse);
	response->file = qtrue;
	response->freeBody = qtrue;
	response->bodySize = length;
	return qtrue;
}

static void HTTP_BodyHeader(char *buf, int size) {
	char *style = NULL;

	Q_strcat(buf, size, "<html>\n<head>\n\t<style>\n");

	FS_ReadFile("http/style.css", (void**)&style);
	if (style) {
		Q_strcat(buf, size, style);
		FS_FreeFile(style);
	} else {
		Q_strcat(buf, size,
			"\tdiv#content {\n"
			"\t\tposition: relative;\n"
			"\t\twidth: 95%;\n"
			"\t\tmargin: 20px auto;\n"
			"\t\tbackground-repeat:repeat;\n"
			"\t\tpadding: 0px;\n"
			"\t\tfont-size: 1.0em;\n"
			"\t\tline-height: 1.6em;\n"
			"\t}\n"
			"\tdiv#footer {\n"
			"\t\twidth:100%;\n"
			"\t\tmargin:0px;\n"
			"\t\tpadding:20px;\n"
			"\t\tclear:both;\n"
			"\t\tbox-shadow: 0px 0px 15px #000;\n"
			"\t}\n"
			"\t.map {\n"
			"\t\tposition: relative;\n"
			"\t\twidth:340px;\n"
			"\t\tmargin:10px 10px;\n"
			"\t\tpadding:0px;\n"
			"\t\tfloat:left;\n"
			"\t\tborder:1px solid #063978;\n"
			"\t\tbox-shadow: 0px 0px 15px #000;\n"
			"\t\t-moz-box-shadow: #000 0px 0px 15px;\n"
			"\t\t-webkit-box-shadow: #000 0px 0px 15px;\n"
			"\t}\n"
			"\t.map h1 {\n"
			"\t\tfont-size:1.4em;\n"
			"\t\tfont-weight:bold;\n"
			"\t\tmargin:5px 5px 5px 5px;\n"
			"\t\tcolor:#010101;\n"
			"\t}\n"
			"\t.map p {\n"
			"\t\tmargin:0px 5px 0px 0px;\n"
			"\t\tpadding:0px;\n"
			"\t\tfloat:right;\n"
			"\t\tcolor:#f1f1f1;\n"
			"\t}\n"
			"\t.img {\n"
			"\t\tmargin:0px 5px 0px 5px;\n"
			"\t\tpadding:0px;\n"
			"\t\tborder:1px solid #f1f1f1;\n"
			"\t}\n");
	}
	Q_strcat(buf, size, "\t</style>\n"
		"\t<title>" Q3_VERSION "</title>\n</head>\n"
		"<body>\n<div id=\"content\">\n");
}

static void HTTP_BodyFooter(char *buf, int size) {
	Q_strcat(buf, size, "</div>\n</body>\n</html>\n");
}

static void HTTP_MapsAddMap(char *buf, int size, const char *map) {
	char imgLine[256];
	char mapName[MAX_QPATH];
	char imageName[MAX_QPATH] = "";
	const char **ext;
	COM_StripExtension(map, mapName, sizeof(mapName));
	for (ext = httpImageExtensions; *ext; ++ext) {
		fileHandle_t imgFileHandle;
		Com_sprintf(imageName, sizeof(imageName), "levelshots/%s.%s", mapName, *ext);
		FS_FOpenFileRead(imageName, &imgFileHandle, qtrue);
		if (imgFileHandle) {
			FS_FCloseFile(imgFileHandle);
			break;
		}
		imageName[0] = '\0';
	}
	if (imageName[0] == '\0') {
		Q_strncpyz(imageName, "levelshots/unknownmap", sizeof(imageName));
	}

	Com_sprintf(imgLine, sizeof(imgLine),
		"\t\t<h1>%s</h1>\n"
		"\t\t<div class=\"img\"><img width=\"320\" height=\"240\" alt=\"%s\" src=\"maps/%s\" /></div>\n",
		mapName, mapName, imageName);

	Q_strcat(buf, size, "\t<div class=\"map\">\n");
	Q_strcat(buf, size, imgLine);
	Q_strcat(buf, size, "\t</div>\n");
}

static qboolean HTTP_Maps_f(const httpRequest_t *request, httpResponse_t *response) {
	static char mapsOutputBuffer[512 * 1024];
	char maps[4096];
	const int numMaps = FS_GetFileList("maps", "bsp", maps, sizeof(maps));
	int i;
	const char *ptr = maps;

	if (strstr(request->path, "/maps/levelshots/")) {
		const char *levelshot = request->path + 6;
		const char *extension = COM_GetExtension(levelshot);
		qboolean validImage = qfalse;
		const char **ext;

		if (extension) {
			for (ext = httpImageExtensions; *ext; ++ext) {
				if (!Q_stricmp(*ext, extension)) {
					validImage = qtrue;
					break;
				}
			}
		}
		if (!validImage) {
			levelshot = "menu/art/unknownmap.jpg";
		}
		HTTP_ResponseSetFile(response, levelshot);
	} else {
		HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_TEXT_HTML, qfalse);
		if (mapsOutputBuffer[0] == '\0') {
			HTTP_BodyHeader(mapsOutputBuffer, sizeof(mapsOutputBuffer));
			for (i = 0; i < numMaps; ++i, ptr += (strlen(ptr) + 1)) {
				HTTP_MapsAddMap(mapsOutputBuffer, sizeof(mapsOutputBuffer), ptr);
			}
			HTTP_BodyFooter(mapsOutputBuffer, sizeof(mapsOutputBuffer));
		}

		HTTP_ResponseSetText(response, mapsOutputBuffer);
	}

	return qtrue;
}

static qboolean HTTP_Status_f(const httpRequest_t *request, httpResponse_t *response) {
	static char outputBuffer[16 * 1024] = "";
	client_t *cl;
	int i;

	if (!com_sv_running->integer) {
		HTTP_ResponseSetText(response, "Server is not running\n");
		return qtrue;
	}

	outputBuffer[0] = '\0';

	HTTP_BodyHeader(outputBuffer, sizeof(outputBuffer));

	HTTP_MapsAddMap(outputBuffer, sizeof(outputBuffer), sv_mapname->string);

	Q_strcat(outputBuffer, sizeof(outputBuffer), "<table><tr><th>cl</th><th>score</th><th>ping</th><th>name</th><th>address</th><th>rate</th><tr>");

	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
		playerState_t *ps;
		char buf[512];
		const char *s;

		if (!cl->state) {
			continue;
		}

		ps = SV_GameClientNum(i);
		s = NET_AdrToString(cl->netchan.remoteAddress);

		Com_sprintf(buf, sizeof(buf), "<tr><td>%i</td><td>%i</td><td>%i</td><td>%s</td><td>%s</td><td>%i</td></tr>",
			i, ps->persistant[PERS_SCORE], cl->ping, cl->name, s, cl->rate);

		Q_strcat(outputBuffer, sizeof(outputBuffer), buf);
	}

	Q_strcat(outputBuffer, sizeof(outputBuffer), "</table>");

	HTTP_BodyFooter(outputBuffer, sizeof(outputBuffer));

	HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_TEXT_HTML, qfalse);
	HTTP_ResponseSetText(response, outputBuffer);

	return qtrue;
}

static qboolean HTTP_PK3_f(const httpRequest_t *request, httpResponse_t *response) {
	const char *referencedPaks = Cvar_VariableString("sv_referencedPakNames");
	const char *baseGame = Cvar_VariableString("com_basegame");
	const int baseGameSize = (int)strlen(baseGame);
	const char *basename;
	char pk3Path[MAX_QPATH];
	char *dispositionBuf;
	int i;

	if (!referencedPaks || !*referencedPaks) {
		HTTP_ResponseSetText(response, "Server doesn't have any pk3 referenced\n");
		return qtrue;
	}

	COM_StripExtension(request->path + 5, pk3Path, sizeof(pk3Path));
	if (Q_strncmp(baseGame, pk3Path, baseGameSize) != 0) {
		HTTP_ResponseSetText(response, "Invalid pk3 file request\n");
		return qtrue;
	}

	basename = pk3Path + baseGameSize;

	if (*basename != '/') {
		HTTP_ResponseSetText(response, "Invalid request path\n");
		return qtrue;
	}
	++basename;

#ifndef STANDALONE
	if (FS_idPak(pk3Path, BASEGAME, NUM_ID_PAKS)
#ifdef MISSIONPACK
		|| FS_idPak(pk3Path, BASETA, NUM_TA_PAKS)
#endif
	) {
		HTTP_ResponseSetText(response, "Invalid request path\n");
		return qtrue;
	}
#endif

	Cmd_TokenizeString(referencedPaks);

	for (i = 0; i < Cmd_Argc(); i++) {
		if (!strcmp(pk3Path, Cmd_Argv(i))) {
			// attachment; filename=""
			const int bufSize = sizeof(pk3Path) + 24;
			dispositionBuf = (char *)malloc(bufSize);
			Com_sprintf(dispositionBuf, bufSize, "attachment; filename=\"%s.pk3\"", basename);
			HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_TYPE, HTTP_MIMETYPE_APPLICATION_OCTET_STREAM, qfalse);
			HTTP_SetHeader(&response->proto, HTTP_HEADER_CONTENT_DISPOSITION, dispositionBuf, qtrue);
			HTTP_ResponseSetPk3File(response, va("%s.pk3", pk3Path));
			return qtrue;
		}
	}

	HTTP_ResponseSetText(response, "Given pk3 is not referenced\n");

	return qtrue;
}

qboolean SV_HTTPServerInit(void) {
	int port = sv_httpServerPort->integer;
	const char *httpInterface = sv_httpServerIP->string;

	struct sockaddr_in sin;
	int t = 1;
	int i;

	if (port <= 0) {
		Com_Printf("Built-in http server is disabled\n");
		return qtrue;
	}

	memset(httpClients, 0, sizeof(httpClients));
	for (i = 0; i < HTTP_MAX_CLIENTS; ++i) {
		httpClients[i].socket = INVALID_SOCKET;
	}

	memset(httpEndpoints, 0, sizeof(httpEndpoints));

	httpSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (httpSocket == INVALID_SOCKET) {
		return qfalse;
	}
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;

	if (!httpInterface || !httpInterface[0]) {
		sin.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (!Sys_StringToSockaddr(httpInterface, (struct sockaddr *)&sin, sizeof(sin), AF_INET)) {
			closesocket(httpSocket);
			return qfalse;
		}
	}
	sin.sin_port = htons(port);

	FD_ZERO(&httpReadFDSet);
	FD_ZERO(&httpWriteFDSet);

#ifdef _WIN32
	if (setsockopt(httpSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&t, sizeof(t)) != 0) {
#else
	if (setsockopt(httpSocket, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) != 0) {
#endif
		closesocket(httpSocket);
		httpSocket = INVALID_SOCKET;
		return qfalse;
	}

	if (bind(httpSocket, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		closesocket(httpSocket);
		httpSocket = INVALID_SOCKET;
		return qfalse;
	}

	if (listen(httpSocket, 5) < 0) {
		closesocket(httpSocket);
		httpSocket = INVALID_SOCKET;
		return qfalse;
	}

#ifndef _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

#ifdef O_NONBLOCK
	fcntl(httpSocket, F_SETFL, O_NONBLOCK);
#endif
#ifdef _WIN32
	{
		unsigned long mode = 1;
		ioctlsocket(httpSocket, FIONBIO, &mode);
	}
#endif
	FD_SET(httpSocket, &httpReadFDSet);

	HTTP_Register(HTTP_METHOD_GET, "/maps", HTTP_Maps_f, qfalse);
	HTTP_Register(HTTP_METHOD_GET, "/pk3", HTTP_PK3_f, qfalse);
	HTTP_Register(HTTP_METHOD_GET, "/status", HTTP_Status_f, qfalse);

	Com_Printf("Built-in http server is listening on %s:%i\n", httpInterface ? httpInterface : "0.0.0.0", port);

	return qtrue;
}

void SV_HTTPServerShutdown(void) {
	int i;
	for (i = 0; i < HTTP_MAX_CLIENTS; ++i) {
		HTTP_RemoveClient(&httpClients[i]);
	}

	FD_ZERO(&httpReadFDSet);
	FD_ZERO(&httpWriteFDSet);
	closesocket(httpSocket);
	httpSocket = INVALID_SOCKET;
}

#endif

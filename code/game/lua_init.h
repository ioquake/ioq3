#ifndef _LUA_INIT_H_
#define _LUA_INIT_H_

#define luajit_c // ???
#include "..\luajit\lua.h"
#include "..\luajit\lauxlib.h"
#include "..\luajit\lualib.h"
#include "..\luajit\luajit.h"

extern lua_State *global_lua;

int LUA_callfunction(lua_State *L, char *functionname, char *params, ...);

int traceback(lua_State *L);
int docall(lua_State *L, int narg, int clear);
void l_message(const char *pname, const char *msg);
int report(lua_State *L, int status);
int dofile(lua_State *L, const char *name);
int dostring(lua_State *L, const char *s, const char *name);
int dolibrary(lua_State *L, const char *name);
int l_sin (lua_State *L);
int lua_Cvar_Set (lua_State *L);
int lua_Com_Printf(lua_State *L);
void LUA_init();

#endif
#include "lua_init.h"

#include <math.h>
lua_State *global_lua = NULL;


#include "../../code/client/client.h"

int LUA_callfunction(lua_State *L, char *functionname, char *params, ...)
{
	va_list args;
	int len, i, errors=0;
	int ret_start, rets_amount=0;
	int args_amount = 0;
	if ( ! L)
		return 0;

	lua_getglobal(global_lua, functionname);

	if (lua_isnil(L, -1)) {
		Com_Printf("[LUA][ENGINE][WARNING] function \"%s\" not found! params=\"%s\"\n", functionname, params);
		return 0;
	}

	len = strlen(params);
	va_start(args, params);
	for (i=0; i<len; i++) {
		// leave loop to handle the to-be-returned values
		if (params[i] == ',') {
			rets_amount = len - i - 1;
			break;
		}

		switch (params[i]) {
			case 'i': {
				int tmp_int = va_arg(args, int);
				lua_pushinteger(L, tmp_int);
				args_amount++;
				break;
			}
			case 's': {
				char *tmp_str = va_arg(args, char *);
				lua_pushstring(L, tmp_str);
				args_amount++;
				break;
			}
			default:
				errors++;
				Com_Printf("[WARNING] [PUSH] LUA_callfunction errors=%d params[%d]=%c not implemented!\n", errors, i, params[i]);
		}
	}
	
	lua_call(global_lua, args_amount, rets_amount);

	i++; // jump ,
	for (; i<len; i++) {
		switch (params[i]) {
			case 'i': {
				int *tmp_int = va_arg(args, int *);
				*tmp_int = lua_tointeger(L, len - i); // todo: test with 2 return args (only one return value is just -1)
				break;
			}
			default:
				errors++;
				Com_Printf("[WARNING] [POP] LUA_callfunction errors=%d params[%d]=%c not implemented!\n", errors, i, params[i]);
		}
	}
	va_end(args);
	lua_pop(global_lua, rets_amount);


	return errors == 0; // success if no errors
}

int traceback(lua_State *L)
{
  if (!lua_isstring(L, 1)) { /* Non-string error object? Try metamethod. */
    if (lua_isnoneornil(L, 1) ||
	!luaL_callmeta(L, 1, "__tostring") ||
	!lua_isstring(L, -1))
      return 1;  /* Return non-string error object. */
    lua_remove(L, 1);  /* Replace object by result of __tostring metamethod. */
  }
  luaL_traceback(L, L, lua_tostring(L, 1), 1);
  return 1;
}

int docall(lua_State *L, int narg, int clear)
{
  int status;
  int base = lua_gettop(L) - narg;  /* function index */
  lua_pushcfunction(L, traceback);  /* push traceback function */
  lua_insert(L, base);  /* put it under chunk and args */
#if !LJ_TARGET_CONSOLE && 0
  signal(SIGINT, laction);
#endif
  status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
#if !LJ_TARGET_CONSOLE && 0
  signal(SIGINT, SIG_DFL);
#endif
  lua_remove(L, base);  /* remove traceback function */
  /* force a complete garbage collection in case of errors */
  if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
  return status;
}

void l_message(const char *pname, const char *msg)
{
	if (pname) Com_Printf("%s: ", pname);
	Com_Printf("%s\n", msg);
}

int report(lua_State *L, int status)
{
  if (status && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message("", msg);
    lua_pop(L, 1);
  }
  return status;
}

int dofile(lua_State *L, const char *name)
{
  int status = luaL_loadfile(L, name) || docall(L, 0, 1);
  return report(L, status);
}

int dostring(lua_State *L, const char *s, const char *name)
{
  int status = luaL_loadbuffer(L, s, strlen(s), name) || docall(L, 0, 1);
  return report(L, status);
}

// http://www.lua.org/source/5.0/src_lib_lauxlib.c.html#errfile
void callalert (lua_State *L) {
	lua_getglobal(L, "_ALERT");
	if (lua_isfunction(L, -1)) {
		lua_insert(L, -2);
		lua_call(L, 1, 0);
	} else {  /* no _ALERT function; print it directly */
		Com_Printf("%s\n", lua_tostring(L, -2));
		lua_pop(L, 2);  /* remove error message and _ALERT */
	}
}
int errfile (lua_State *L, int fnameindex) {
  const char *filename = lua_tostring(L, fnameindex) + 1;
  lua_pushfstring(L, "cannot read %s: %s", filename, strerror(errno));
  lua_remove(L, fnameindex);
  return LUA_ERRFILE;
}
int loadfile_fs(lua_State *L, const char *name)
{
	char *code;
	int status;
	fileHandle_t file;
	luaL_Buffer lua_buffer;
	int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */

	FS_FOpenFileRead(name, &file, qtrue); // uniqueFILE = not in pak? but what if i put it in...
	if ( ! file) {
		//errfile(L, fnameindex); // unable to open file
		//callalert(L);
		lua_pushfstring(L, "Cannot open file: %s", name);
		/*
		lua_getglobal(L, "_ALERT");
		if (lua_isfunction(L, -1)) {

			lua_insert(L, -2);

			Com_Printf("Before!!!!\n");
			lua_call(L, 1, 0);
			Com_Printf("After!!!!\n");
		}
		*/
		lua_error(L); // i still wonder how to call our own trace function like in pcall
		//lua_pushnil(L);
		return 0;
	}
	
	luaL_buffinit(L, &lua_buffer);
	
	while (1) {
		char *lua_memory = luaL_prepbuffer(&lua_buffer);
		int len = FS_Read(lua_memory, LUAL_BUFFERSIZE, file);
		//Com_Printf("Len: %d\n", len);
		if (len <= 0) {
			FS_FCloseFile(file);
			break;
		}
		luaL_addsize(&lua_buffer, len);
	}
	luaL_pushresult(&lua_buffer); // close buffer and push as string
	code = lua_tostring(L, -1);
	lua_pop(L, 1); // clean up our string

	{
		int status = luaL_loadbuffer(L, code, strlen(code), name);
		report(L, status);
		return 1; // return the function object
		//return dostring(L, code, name);
	}
}

int dolibrary(lua_State *L, const char *name)
{
  lua_getglobal(L, "require");
  lua_pushstring(L, name);
  return report(L, docall(L, 1, 1));
}
int doloadfile_fs(lua_State *L, const char *name)
{
  lua_getglobal(L, "myloadfile");
  lua_pushstring(L, name);
  report(L, docall(L, 1, 0)); // 1=arg(the string) and 0=dont clean result up(the function)
  return report(L, docall(L, 0, 1));
}

int l_sin (lua_State *L) {
	double d = luaL_checknumber(L, 1);
	lua_pushnumber(L, sin(d * (M_PI/180)));
	return 1;  /* number of results */
}

int lua_Cvar_Set (lua_State *L) {	
	char *var_name = (char *)luaL_checkstring(L, 1);
	char *value = (char *)luaL_checkstring(L, 2);
	//Cvar_Set(var_name, value);
}
int lua_Com_Printf(lua_State *L) {	
	char *text = (char *)luaL_checkstring(L, 1);
	Com_Printf("%s", text);
	return 0;
}
// need to export it to read fs_game in include(), which would have circle dependency on include("lua\\codscript")
int lua_Cvar_VariableString(lua_State *L) {
	char *name = (char *)luaL_checkstring(L, 1);
	lua_pushstring(L, Cvar_VariableString(name));
	return 1;
}
int lua_myloadfile(lua_State *L) {
	char *name = (char *)luaL_checkstring(L, 1);
	//Com_Printf("Open file: %s\n", name);
	return loadfile_fs(L, name);;
}
void LUA_init()
{
	int ret;
	lua_State *L;

	global_lua = lua_open();
	L = global_lua; // just to remove the references in this function (makes search list through whole source way smaller)

	lua_pushcfunction(L, l_sin);
	lua_setglobal(L, "mysin");
	
	lua_pushcfunction(L, lua_Cvar_Set);
	lua_setglobal(L, "Cvar_Set");
	
	lua_pushcfunction(L, lua_Com_Printf);
	lua_setglobal(L, "Com_Printf");

	lua_pushcfunction(L, lua_Cvar_VariableString);
	lua_setglobal(L, "Cvar_VariableString"); // only game.exe

	lua_pushcfunction(L, lua_myloadfile);
	lua_setglobal(L, "myloadfile");

	// we have 3 lua-engines, so give the scripts some orientation
	lua_pushinteger(L, 0);
	lua_setglobal(L, "CLIENT"); // only cgame.dll
	lua_pushinteger(L, 0);
	lua_setglobal(L, "SERVER"); // only game.dll
	lua_pushinteger(L, 1);
	lua_setglobal(L, "ENGINE"); // only game.exe

	lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
	luaL_openlibs(L);  /* open libraries */

	/*
	lua_pushcfunction(L, luaopen_table);
	lua_pushliteral(L, LUA_TABLIBNAME);
	lua_call(L, 1, 0);

	luaopen_base(L);
	luaopen_math(L);
	luaopen_string(L);
	luaopen_table(L);
	luaopen_io(L);
	luaopen_os(L);
	luaopen_package(L);
	luaopen_debug(L);
	luaopen_bit(L);
	luaopen_jit(L);
	luaopen_ffi(L);
	*/
	lua_gc(L, LUA_GCRESTART, -1);

	ret = doloadfile_fs(L, "lua\\ENGINE\\main.lua");
	//ret = dofile(L, "baseq3\\lua\\ENGINE\\main.lua");

	Com_Printf("[ENGINE] LUA_init global_lua=%.8p dofile=%s fs_game=%s\n", L, (!ret)?"success":"fail", Cvar_VariableString("fs_game"));
	
	if (ret) { // 1 means error
		lua_close(global_lua);
		global_lua = NULL;
	}
}
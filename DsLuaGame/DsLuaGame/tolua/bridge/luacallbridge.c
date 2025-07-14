#include "luacallbridge.h"

//创建原表
void lua_handler(const char* name, uint32_t sock, const char* data)
{
	lua_getglobal(g_L, name);
	if (lua_isfunction(g_L, -1)) {
		lua_pushinteger(g_L, sock);
		lua_pushstring(g_L, data);
		if (lua_pcall(g_L, 2, 0, 0) != LUA_OK) {
			fprintf(stderr, "Lua error: %s\n", lua_tostring(g_L, -1));
			lua_pop(g_L, 1);
		}
	}
	else {
		lua_pop(g_L, 1);
	}
}


void lua_call_mg(const char* name, sock_t sock, bytestream* bystream)
{
	lua_getglobal(g_L, name);
	if (lua_isfunction(g_L, -1)) {
		lua_pushinteger(g_L, sock);
		
		// 创建存储指针的 userdata
		bytestream** ud = (bytestream**)lua_newuserdata(g_L, sizeof(bytestream*));
		*ud = bystream;  // 复制结构体内容

		// 设置元表
		luaL_getmetatable(g_L, "bytestream");
		lua_setmetatable(g_L, -2);

		if (lua_pcall(g_L, 2, 0, 0) != LUA_OK) {
			printf("Lua error: %s\n", lua_tostring(g_L, -1));
			lua_pop(g_L, 1);
		}
	}
	else {
		lua_pop(g_L, 1);
	}
}


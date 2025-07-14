#include "public.h"
#include "logger/elog.h"

//日志对象元表
static const char* LOG_METATABLE = "elog";

static int lua_log_a(lua_State* L)
{
	const char* txt = luaL_checkstring(L, 1);
	log_a(txt);
	lua_pushnil(L);
	return 1;
}
static int lua_log_e(lua_State* L)
{
	const char* txt = luaL_checkstring(L, 1);
	log_e(txt);
	lua_pushnil(L);
	return 1;
}
static int lua_log_w(lua_State* L)
{
	const char* txt = luaL_checkstring(L, 1);
	log_w(txt);
	lua_pushnil(L);
	return 1;
}
static int lua_log_i(lua_State* L)
{
	const char* txt = luaL_checkstring(L, 1);
	log_i(txt);
	lua_pushnil(L);
	return 1;
}

// 注册所有Lua函数 
static const luaL_Reg elog_lib[] = {
	{ "log_a", lua_log_a },
	{ "log_e", lua_log_e },
	{ "log_w", lua_log_w },
	{ "log_i", lua_log_i },
	{ NULL, NULL }
};

//模块初始化
int luaopen_elog(lua_State* L){
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, elog_lib);  // 模块表在栈顶（位置-1）

	// 2. 创建元表（用于字节流对象）
	luaL_newmetatable(L, LOG_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 清理栈：弹出元表（保留模块表作为返回值）
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）
	// 5. 返回模块表（Lua require会获取这个表）
	return 1;
}
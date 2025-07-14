#include "public.h"
#include "server.h"

//application对象元表
static const char* APP_METATABLE = "app";

static int lua_init_server(lua_State* L)
{
	const char* path = luaL_checkstring(L, 1);
	init_server(path);
	lua_pushnil(L);
	return 1;
}

static int lua_wait_stop(lua_State* L)
{
	//无参数，调用waitStop;
	wait_stop();
	lua_pushnil(L);
	return 1;
}

static int lua_fina_server(lua_State* L)
{
	fina_server();
	lua_pushnil(L);
	return 1;
}

//注册所有函数
static const luaL_Reg appserver_lib[] = {
	{ "init_server", lua_init_server },
	{ "wait_stop", lua_wait_stop },
	{ "fina_server", lua_fina_server },
	{ NULL, NULL }
};

//模块初始化
int luaopen_appserver(lua_State* L){
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, appserver_lib);  // 模块表在栈顶（位置-1） 

	// 2. 创建元表（用于字节流对象） 
	luaL_newmetatable(L, APP_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 清理栈：弹出元表（保留模块表作为返回值）
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）
	// 5. 返回模块表（Lua require会获取这个表）
	return 1;
}
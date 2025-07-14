#include "public.h"
#include "objectpool/objectpool.h"

static const char* OBJECTPOOL_METATABLE = "objectpool";
// 初始化对象池
static int lua_init_bytestream_pool(lua_State* L) {
	size_t initial_capacity = luaL_checkinteger(L, 1);
	objectpool* pool = init_bytestream_pool(initial_capacity);
	lua_pushlightuserdata(L, pool);
	return 1;
}

// 获取 bytestream 对象
static int lua_obtain_bytestream(lua_State* L) {
	bytestream* bs = obtain_bytestream();
	// 创建 full userdata 并关联元表
	bytestream** ud = (bytestream**)lua_newuserdata(L, sizeof(bytestream*));
	*ud = bs;
	luaL_getmetatable(L, "bytestream");
	lua_setmetatable(L, -2);

	return 1;
}

// 归还 bytestream 对象
static int lua_giveup_bytestream(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	if (*ud) {
		giveup_bytestream(*ud);
		*ud = NULL; // 防止重复释放
	}
	return 0;
}

// 销毁对象池
static int lua_fina_bytestream_pool(lua_State* L) {
	fina_bytestream_pool();
	return 0;
}

// 元表方法：__gc 用于垃圾回收
static int lua_bytestream_gc(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	if (*ud) {
		bytestream_destroy(*ud);
		*ud = NULL;
	}
	return 0;
}

// 元表方法：获取长度
static int lua_bytestream_len(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	lua_pushinteger(L, bytestream_getdatasize(*ud));
	return 1;
}

// 创建函数表
static const luaL_Reg objectpool_lib[] = {
	{ "init_bytestream_pool", lua_init_bytestream_pool },
	{ "obtain_bytestream", lua_obtain_bytestream },
	{ "giveup_bytestream", lua_giveup_bytestream },
	{ "fina_bytestream_pool", lua_fina_bytestream_pool },
	{ NULL, NULL }
};

// 注册函数到 Lua
int luaopen_objectpool(lua_State* L) {
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, objectpool_lib);

	// 2. 创建元表（用于字节流对象） 
	luaL_newmetatable(L, OBJECTPOOL_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 设置元方法
	lua_pushcfunction(L, lua_bytestream_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushcfunction(L, lua_bytestream_len);
	lua_setfield(L, -2, "__len");

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 清理栈：弹出元表（保留模块表作为返回值）
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）
	// 5. 返回模块表（Lua require会获取这个表）
	return 1;
}
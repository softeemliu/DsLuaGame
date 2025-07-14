#include "public.h"
#include "core/timer.h"
#include "tolua/bridge/luacallbridge.h"


// 新增：Lua 回调数据结构
typedef struct {
	lua_State* L;
	int ref;        // Lua 函数引用
} LuaCallbackData;

static const char* TIMER_METATABLE = "timer";

// 新增：Lua 回调适配器
void lua_timer_callback(void* data) {
	LuaCallbackData* cb_data = (LuaCallbackData*)data;
	AUTO_LOCK(&_lualock); // 加全局锁（需实现）

	// 获取并调用 Lua 函数
	lua_rawgeti(cb_data->L, LUA_REGISTRYINDEX, cb_data->ref);
	if (lua_pcall(cb_data->L, 0, 0, 0))
	{
		fprintf(stderr, "Error running timer callback: %s\n", lua_tostring(cb_data->L, -1));
		lua_pop(cb_data->L, 1);
	}

	AUTO_UNLOCK(&_lualock); // 解全局锁
}

// 新增：Lua 用户数据释放函数
void free_lua_user_data(void* data) {
	LuaCallbackData* cb_data = (LuaCallbackData*)data;
	AUTO_LOCK(&_lualock);
	luaL_unref(cb_data->L, LUA_REGISTRYINDEX, cb_data->ref); // 释放函数引用
	AUTO_UNLOCK(&_lualock);
	free(cb_data);
}

// 新增：Lua 绑定函数 - 添加定时器
int lua_timer_add(lua_State* L) {
	// 参数检查：interval, repeat, function
	if (lua_gettop(L) < 3 || !lua_isnumber(L, 1) || !lua_isboolean(L, 2) || !lua_isfunction(L, 3)) {
		lua_pushstring(L, "Invalid arguments");
		lua_error(L);
		return 0;
	}

	uint32_t interval = (uint32_t)lua_tonumber(L, 1);
	bool repeat = lua_toboolean(L, 2);

	// 创建 Lua 回调数据
	LuaCallbackData* cb_data = malloc(sizeof(LuaCallbackData));
	cb_data->L = L;
	lua_pushvalue(L, 3); // 复制函数到栈顶
	cb_data->ref = luaL_ref(L, LUA_REGISTRYINDEX); // 创建函数引用

	// 添加定时器
	uint32_t timer_id = timer_add(
		interval,
		repeat,
		lua_timer_callback, // 使用适配器
		cb_data,
		free_lua_user_data  // 设置释放函数
		);

	lua_pushinteger(L, timer_id);
	return 1;
}

// 新增：Lua 绑定函数 - 删除定时器
int lua_timer_remove(lua_State* L) {
	if (lua_gettop(L) < 1 || !lua_isnumber(L, 1)) {
		lua_pushstring(L, "Missing timer ID");
		lua_error(L);
		return 0;
	}

	uint32_t timer_id = (uint32_t)lua_tonumber(L, 1);
	timer_remove(timer_id);
	return 0;
}

// 新增：Lua 绑定函数 - 销毁
int lua_timer_destroy(lua_State* L) {
	timer_destroy();
	return 0;
}

// 新增：Lua 模块注册
static const luaL_Reg timer_lib[] = {
	{ "add", lua_timer_add },
	{ "remove", lua_timer_remove },
	{ "destroy", lua_timer_destroy },
	{ NULL, NULL }
};

int luaopen_timer(lua_State* L) {
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, timer_lib);

	// 2. 创建元表（用于字节流对象） 
	luaL_newmetatable(L, TIMER_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 清理栈：弹出元表（保留模块表作为返回值）
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）
	// 5. 返回模块表（Lua require会获取这个表）
	return 1;
}
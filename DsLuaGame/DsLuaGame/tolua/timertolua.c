#include "public.h"
#include "core/timer.h"
#include "tolua/bridge/luacallbridge.h"


// ������Lua �ص����ݽṹ
typedef struct {
	lua_State* L;
	int ref;        // Lua ��������
} LuaCallbackData;

static const char* TIMER_METATABLE = "timer";

// ������Lua �ص�������
void lua_timer_callback(void* data) {
	LuaCallbackData* cb_data = (LuaCallbackData*)data;
	AUTO_LOCK(&_lualock); // ��ȫ��������ʵ�֣�

	// ��ȡ������ Lua ����
	lua_rawgeti(cb_data->L, LUA_REGISTRYINDEX, cb_data->ref);
	if (lua_pcall(cb_data->L, 0, 0, 0))
	{
		fprintf(stderr, "Error running timer callback: %s\n", lua_tostring(cb_data->L, -1));
		lua_pop(cb_data->L, 1);
	}

	AUTO_UNLOCK(&_lualock); // ��ȫ����
}

// ������Lua �û������ͷź���
void free_lua_user_data(void* data) {
	LuaCallbackData* cb_data = (LuaCallbackData*)data;
	AUTO_LOCK(&_lualock);
	luaL_unref(cb_data->L, LUA_REGISTRYINDEX, cb_data->ref); // �ͷź�������
	AUTO_UNLOCK(&_lualock);
	free(cb_data);
}

// ������Lua �󶨺��� - ��Ӷ�ʱ��
int lua_timer_add(lua_State* L) {
	// ������飺interval, repeat, function
	if (lua_gettop(L) < 3 || !lua_isnumber(L, 1) || !lua_isboolean(L, 2) || !lua_isfunction(L, 3)) {
		lua_pushstring(L, "Invalid arguments");
		lua_error(L);
		return 0;
	}

	uint32_t interval = (uint32_t)lua_tonumber(L, 1);
	bool repeat = lua_toboolean(L, 2);

	// ���� Lua �ص�����
	LuaCallbackData* cb_data = malloc(sizeof(LuaCallbackData));
	cb_data->L = L;
	lua_pushvalue(L, 3); // ���ƺ�����ջ��
	cb_data->ref = luaL_ref(L, LUA_REGISTRYINDEX); // ������������

	// ��Ӷ�ʱ��
	uint32_t timer_id = timer_add(
		interval,
		repeat,
		lua_timer_callback, // ʹ��������
		cb_data,
		free_lua_user_data  // �����ͷź���
		);

	lua_pushinteger(L, timer_id);
	return 1;
}

// ������Lua �󶨺��� - ɾ����ʱ��
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

// ������Lua �󶨺��� - ����
int lua_timer_destroy(lua_State* L) {
	timer_destroy();
	return 0;
}

// ������Lua ģ��ע��
static const luaL_Reg timer_lib[] = {
	{ "add", lua_timer_add },
	{ "remove", lua_timer_remove },
	{ "destroy", lua_timer_destroy },
	{ NULL, NULL }
};

int luaopen_timer(lua_State* L) {
	// 1. ����ģ����������е��������� 
	luaL_newlib(L, timer_lib);

	// 2. ����Ԫ�������ֽ������� 
	luaL_newmetatable(L, TIMER_METATABLE);  // Ԫ����ջ����λ��-1����ģ�������λ��-2 

	// 3. ����Ԫ���__indexΪģ����ؼ���֧����������﷨�� 
	lua_pushvalue(L, -2);  // ��ģ���ѹջ��λ��-1�� 
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = ģ���ջ��Ԫ�ص����� 

	// 4. ����ջ������Ԫ������ģ�����Ϊ����ֵ��
	lua_pop(L, 1);  // ����Ԫ��λ��2��������ջ����ģ���λ��1��
	// 5. ����ģ���Lua require���ȡ�����
	return 1;
}
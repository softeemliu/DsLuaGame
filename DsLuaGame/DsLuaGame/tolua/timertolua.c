#include "public.h"
#include "core/timer.h"
#include "tolua/bridge/luacallbridge.h"
#include "core/mempool.h"
#include "logger/elog.h"

extern timermanager* timermgr;
static const char* TIMER_METATABLE = "timer";

// Lua����ṹ
typedef struct {
	lua_State* L;         // Lua״̬��
	int callback_ref;      // Lua�ص���������
	int task_ref;          // Lua�����������
} luataskdata;


// Lua�ص�����
void lua_timer_callback(void* arg) 
{
	luataskdata* data = (luataskdata*)arg;
	lua_State* L = data->L;

	// ��ȡLua�ص�����
	lua_rawgeti(L, LUA_REGISTRYINDEX, data->callback_ref);

	// ����Lua����
	if (lua_pcall(L, 0, 0, 0))
	{
		// ������
		const char* err = lua_tostring(L, -1);
		log_e("Lua timer callback error: %s", err);
		lua_pop(L, 1); // ����������Ϣ
	}
}

// ����Lua��ʱ����
timertask* lua_timer_task(lua_State* L, int callback_ref, int task_ref) {
	luataskdata* data = (luataskdata*)mempool_allocate(mempool_getinstance(), sizeof(luataskdata));
	data->L = L;
	data->callback_ref = callback_ref;
	data->task_ref = task_ref;

	timertask* task = timer_task_create(lua_timer_callback, data);
	return task;
}

// ���ٶ�ʱ����
void lua_timer_task_destroy(timertask* task) {
	if (task->arg) {
		// �����Lua�����ͷ�Lua����
		luataskdata* data = (luataskdata*)task->arg;
		if (data->callback_ref != LUA_NOREF) {
			luaL_unref(data->L, LUA_REGISTRYINDEX, data->callback_ref);
		}
		if (data->task_ref != LUA_NOREF) {
			luaL_unref(data->L, LUA_REGISTRYINDEX, data->task_ref);
		}
		mempool_deallocate(mempool_getinstance(), data, sizeof(luataskdata));
	}
	timer_task_destroy(task);
}


// ����Lua�������
static int lua_timer_task_create(lua_State* L) {
	// ��ȡ�������ӳ�(����), ���(����), �ص�����
	long delay = luaL_checkinteger(L, 1);
	long interval_val = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	// ����Lua��������
	lua_pushvalue(L, 3); // ���ƻص�����
	int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	// �����������
	luataskdata** ud = (luataskdata**)lua_newuserdata(L, sizeof(luataskdata*));
	*ud = NULL;

	// ����Ԫ��
	luaL_getmetatable(L, TIMER_METATABLE);
	lua_setmetatable(L, -2);

	// ���������������
	lua_pushvalue(L, -1);
	int task_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	// ����ʵ������
	interval delay_interval = { delay };
	interval interval_interval = { interval_val };
	timertask* task = lua_timer_task(L, callback_ref, task_ref);

	// ������������
	*ud = (luataskdata*)task->arg;
	// ��������
	timer_schedule_delay(task, delay_interval, interval_interval);
	return 1; // �����������
}

// ȡ��Lua����
static int lua_timer_task_cancel(lua_State* L) {
	luataskdata** ud = (luataskdata**)luaL_checkudata(L, 1, TIMER_METATABLE);
	if (ud && *ud) {
		// ͨ�����������ҵ�ʵ������
		timertask* task = NULL;

		// �������������ҵ���Ӧ����
		AUTO_LOCK(&timermgr->lock);
		timertasknode* node = timermgr->tasks;
		while (node) {
			if (node->task->arg == *ud) {
				task = node->task;
				break;
			}
			node = node->next;
		}
		AUTO_UNLOCK(&timermgr->lock);

		if (task) {
			timer_cancel(task);
		}
	}
	return 0;
}

// Lua�������GC
static int lua_timer_task_gc(lua_State* L) {
	luataskdata** ud = (luataskdata**)luaL_checkudata(L, 1, TIMER_METATABLE);
	if (ud && *ud) {
		// ȡ������
		lua_timer_task_cancel(L);
	}
	return 0;
}


// ������Lua ģ��ע��
static const luaL_Reg timer_lib[] = {
	{ "create", lua_timer_task_create },
	{ NULL, NULL }
};

static const luaL_Reg timertask_mt[] = {
	{ "cancel", lua_timer_task_cancel },
	{ "__gc", lua_timer_task_gc },
	{ NULL, NULL }
};

int luaopen_timer(lua_State* L) {
	// ����Ԫ��
	luaL_newmetatable(L, TIMER_METATABLE);

	// ����Ԫ����
	lua_pushvalue(L, -1);  // ����Ԫ��
	lua_setfield(L, -2, "__index");  // ���� __index = ����
	luaL_setfuncs(L, timertask_mt, 0);  // ע��Ԫ����

	// ����ģ���
	luaL_newlib(L, timer_lib);

	return 1;
}
#include "public.h"
#include "core/timer.h"
#include "tolua/bridge/luacallbridge.h"
#include "core/mempool.h"
#include "logger/elog.h"

extern timermanager* timermgr;
static const char* TIMER_METATABLE = "timer";

// Lua任务结构
typedef struct {
	lua_State* L;         // Lua状态机
	int callback_ref;      // Lua回调函数引用
	int task_ref;          // Lua任务对象引用
} luataskdata;


// Lua回调函数
void lua_timer_callback(void* arg) 
{
	luataskdata* data = (luataskdata*)arg;
	lua_State* L = data->L;

	// 获取Lua回调函数
	lua_rawgeti(L, LUA_REGISTRYINDEX, data->callback_ref);

	// 调用Lua函数
	if (lua_pcall(L, 0, 0, 0))
	{
		// 错误处理
		const char* err = lua_tostring(L, -1);
		log_e("Lua timer callback error: %s", err);
		lua_pop(L, 1); // 弹出错误消息
	}
}

// 创建Lua定时任务
timertask* lua_timer_task(lua_State* L, int callback_ref, int task_ref) {
	luataskdata* data = (luataskdata*)mempool_allocate(mempool_getinstance(), sizeof(luataskdata));
	data->L = L;
	data->callback_ref = callback_ref;
	data->task_ref = task_ref;

	timertask* task = timer_task_create(lua_timer_callback, data);
	return task;
}

// 销毁定时任务
void lua_timer_task_destroy(timertask* task) {
	if (task->arg) {
		// 如果是Lua任务，释放Lua引用
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


// 创建Lua任务对象
static int lua_timer_task_create(lua_State* L) {
	// 获取参数：延迟(毫秒), 间隔(毫秒), 回调函数
	long delay = luaL_checkinteger(L, 1);
	long interval_val = luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	// 创建Lua任务数据
	lua_pushvalue(L, 3); // 复制回调函数
	int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	// 创建任务对象
	luataskdata** ud = (luataskdata**)lua_newuserdata(L, sizeof(luataskdata*));
	*ud = NULL;

	// 设置元表
	luaL_getmetatable(L, TIMER_METATABLE);
	lua_setmetatable(L, -2);

	// 保存任务对象引用
	lua_pushvalue(L, -1);
	int task_ref = luaL_ref(L, LUA_REGISTRYINDEX);

	// 创建实际任务
	interval delay_interval = { delay };
	interval interval_interval = { interval_val };
	timertask* task = lua_timer_task(L, callback_ref, task_ref);

	// 保存任务数据
	*ud = (luataskdata*)task->arg;
	// 安排任务
	timer_schedule_delay(task, delay_interval, interval_interval);
	return 1; // 返回任务对象
}

// 取消Lua任务
static int lua_timer_task_cancel(lua_State* L) {
	luataskdata** ud = (luataskdata**)luaL_checkudata(L, 1, TIMER_METATABLE);
	if (ud && *ud) {
		// 通过任务数据找到实际任务
		timertask* task = NULL;

		// 遍历任务链表找到对应任务
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

// Lua任务对象GC
static int lua_timer_task_gc(lua_State* L) {
	luataskdata** ud = (luataskdata**)luaL_checkudata(L, 1, TIMER_METATABLE);
	if (ud && *ud) {
		// 取消任务
		lua_timer_task_cancel(L);
	}
	return 0;
}


// 新增：Lua 模块注册
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
	// 创建元表
	luaL_newmetatable(L, TIMER_METATABLE);

	// 设置元表方法
	lua_pushvalue(L, -1);  // 复制元表
	lua_setfield(L, -2, "__index");  // 设置 __index = 自身
	luaL_setfuncs(L, timertask_mt, 0);  // 注册元表方法

	// 创建模块表
	luaL_newlib(L, timer_lib);

	return 1;
}
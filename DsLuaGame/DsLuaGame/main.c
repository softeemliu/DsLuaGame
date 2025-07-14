#include "public.h"
#include "config.h"
#include "luasocket.h"
#include "logger/elog.h"
#include "tolua/bridge/luacallbridge.h"
#include "server.h"

#ifdef _USE_PROTOBUF_
extern int luaopen_pb(struct lua_State *L);
extern int luaopen_pb_slice(struct lua_State *L);
extern int luaopen_pb_buffer(struct lua_State *L);
extern int luaopen_pb_conv(struct lua_State *L);
#endif

extern int luaopen_mysql(lua_State *L);
extern int luaopen_cjson(lua_State* L);
extern int luaopen_bytestream(lua_State* L);
extern int luaopen_elog(lua_State* L);
extern int luaopen_network(lua_State* L);
extern int luaopen_appserver(lua_State* L);
extern int luaopen_timer(lua_State* L);
extern int luaopen_objectpool(lua_State* L);
extern int luaopen_socket_core(lua_State* L);
extern int luaopen_mime_core(lua_State* L);
extern int luaopen_sharemen(lua_State* L);


// 日志宏简化错误处理
#define LOG_ERROR(fmt, ...) \
    elog_error(fmt, ##__VA_ARGS__); \
    if (g_L) { \
        const char* err = lua_tostring(g_L, -1); \
        if (err) elog_error("Lua error: %s", err); \
		    }

// 注册所有Lua模块
static int register_lua_libraries(lua_State* L) {
	// 使用静态数组简化注册流程
	struct {
		const char* name;
		lua_CFunction func;
		int global;
	} libs[] = {
#ifdef _USE_PROTOBUF_
		{ "pb", luaopen_pb, 0 },
		{ "pb.slice", luaopen_pb_slice, 0 },
		{ "pb.buffer", luaopen_pb_buffer, 0 },
		{ "pb.conv", luaopen_pb_conv, 0 },
#endif
		{ "mysql", luaopen_mysql, 0 },
		{ "cjson", luaopen_cjson, 0 },
		{ "bytestream", luaopen_bytestream, 0 },
		{ "elog", luaopen_elog, 0 },
		{ "network", luaopen_network, 0 },
		{ "appserver", luaopen_appserver, 0 },
		{ "timer", luaopen_timer, 0 },
		{ "objectpool", luaopen_objectpool, 0 },
		{ "socket.core", luaopen_socket_core, 1 },
		{ "mime.core", luaopen_mime_core, 1 },
		{ "sharemen", luaopen_sharemen, 1 },
		{ NULL, NULL, 0 }  // 结束标记
	};

	for (int i = 0; libs[i].name; i++) {
		luaL_requiref(L, libs[i].name, libs[i].func, libs[i].global);
		lua_pop(L, 1);  // 移除模块表
	}
	return 0;
}

// 加载函数
int LoadConfig() {
	int err = luaL_loadbufferx(g_L, load_config, strlen(load_config),
		"=[DsLuaGame config]", "t");
	if (err != LUA_OK) {
		const char* error = lua_tostring(g_L, -1);
		fprintf(stderr, "加载失败: %s\n", error);
		lua_pop(g_L, 1);
		return 0;
	}
	return 1;
}

// 加载并执行Lua文件
static int load_lua_file(const char* filename) {
	if (luaL_loadfile(g_L, filename) != LUA_OK) {
		LOG_ERROR("Failed to load Lua file: %s", filename);
		return 0;
	}

	if (lua_pcall(g_L, 0, 0, 0) != LUA_OK) {
		LOG_ERROR("Failed to execute Lua file: %s", filename);
		return 0;
	}
	return 1;
}

int main(int argc, char** argv)
{
	typedef struct _SHead
	{
		byte_t magic;
		uint_t messageSize;
	}SHead;
	int len = sizeof(SHead);

	// 参数校验
	if (argc < 2) {
		printf("command invalid! example: ./DsLuaGame ../scripts/main.lua \n");
		exit(EXIT_FAILURE);
	}
	const char* main_script = argv[1];

	// 初始化Lua虚拟机
	if (!(g_L = luaL_newstate())) {
		printf("Failed to create Lua state", "");
		exit(EXIT_FAILURE);
	}
	luaL_openlibs(g_L);  // 加载标准库

	// 注册自定义库
	if (register_lua_libraries(g_L) != 0) {
		printf("Failed to register Lua libraries", "");
		lua_close(g_L);
		g_L = NULL;
		exit(EXIT_FAILURE);;
	}

	LoadConfig();
	// 加载主脚本
	if (load_lua_file(main_script) != LUA_OK)
	{
		// 1. 获取错误信息 
		const char* errorMessage = lua_tostring(g_L, -1);
		if (errorMessage) {
			printf("Error: %s\n", errorMessage);
		}
		else {
			printf("Error: unknown error\n");
		}

		// 2. 获取堆栈跟踪 
		lua_Debug ar;
		int level = 1;  // 从堆栈第一层开始（错误发生点）
		while (lua_getstack(g_L, level, &ar)) {
			lua_getinfo(g_L, "Slnt", &ar);  // 获取短文件名、行号、函数名、是否命名函数 
			printf("  %s:%d in %s\n",
				ar.short_src,   // 短文件名（如@main.lua ）
				ar.currentline,   // 当前行号
				ar.name ? ar.name : "?");  // 函数名（若无则显示"?"
			level++;
		}

		// 3. 清空堆栈（避免后续操作受影响）
		lua_settop(g_L, 0);
	}
	// 成功加载后才初始化
	load_lua_bridge(g_L);
	//主线程阻塞等待退出,在lua里面调用等待函数
	//waitStop();
	//关闭lua虚拟机
	lua_close(g_L);

    return 0;
}
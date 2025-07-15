#include "server.h"
#include "logger/elog.h"
#include "core/timer.h"
#include "utils/datetime.h"
#include "tolua/bridge/luacallbridge.h"
#include "core/network.h"
#include "objectpool/objectpool.h"


void init_server(const char* path)
{
	elog_init(path);
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_ALL & ~ELOG_FMT_FUNC);
#ifdef ELOG_COLOR_ENABLE
	elog_set_text_color_enabled(true);
#endif
	elog_start();
	//初始化时间
	init_datetime();
	//初始化网络
	init_network(0);
	//初始化定时器
	timer_init();
	//初始化bytestream
	init_bytestream_pool(32);
}

static void signal_handler(int sig)
{
	printf("signal_handler, engine process is exit\n");
	exit(0);
}

void wait_stop()
{
#ifndef _WIN32
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
	sleep(-1);
#else
	int c;
	do 
	{
		c = getchar();
	} while (c < 0);
#endif
}

void fina_server()
{
	timer_destroy();
	fina_network();
	//释放
	fina_bytestream_pool();
}

int load_lua_bridge(lua_State* L)
{
	//初始化lua_state锁
	AUTO_LOCK_INIT(&_lualock);

    return 0;  // 成功
}
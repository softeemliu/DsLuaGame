#ifndef __SERVER_H__
#define __SERVER_H__
#include "public.h"

void init_server(const char* path);
void wait_stop();
void fina_server();

//����ȫ�ֵ�lua����
int load_lua_bridge(lua_State* L);

#endif  /*__SERVER_H__*/
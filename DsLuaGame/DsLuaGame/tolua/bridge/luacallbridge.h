#ifndef __LUACALL_BRIDGE_H__
#define __LUACALL_BRIDGE_H__
#include "public.h"
#include "utils/lightlock.h"
#include "core/bytestream.h"

lua_State* g_L;
light_lock _lualock;

void lua_handler(const char* name, uint32_t sock, const char* data);
//调用函数
void lua_call_mg(const char* name, sock_t sock, bytestream* bystream);

#endif  /*__LUACALL_BRIDGE_H__*/
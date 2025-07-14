#include "public.h"
#include "server.h"

//application����Ԫ��
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
	//�޲���������waitStop;
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

//ע�����к���
static const luaL_Reg appserver_lib[] = {
	{ "init_server", lua_init_server },
	{ "wait_stop", lua_wait_stop },
	{ "fina_server", lua_fina_server },
	{ NULL, NULL }
};

//ģ���ʼ��
int luaopen_appserver(lua_State* L){
	// 1. ����ģ����������е��������� 
	luaL_newlib(L, appserver_lib);  // ģ�����ջ����λ��-1�� 

	// 2. ����Ԫ�������ֽ������� 
	luaL_newmetatable(L, APP_METATABLE);  // Ԫ����ջ����λ��-1����ģ�������λ��-2 

	// 3. ����Ԫ���__indexΪģ����ؼ���֧����������﷨�� 
	lua_pushvalue(L, -2);  // ��ģ���ѹջ��λ��-1�� 
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = ģ���ջ��Ԫ�ص����� 

	// 4. ����ջ������Ԫ������ģ�����Ϊ����ֵ��
	lua_pop(L, 1);  // ����Ԫ��λ��2��������ջ����ģ���λ��1��
	// 5. ����ģ���Lua require���ȡ�����
	return 1;
}
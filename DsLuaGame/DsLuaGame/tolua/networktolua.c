#include "public.h"
#include "core/network.h"
#include "core/netsocket.h"
#include "core/bytestream.h"

//application����Ԫ��
static const char* NETWORK_METATABLE = "network";

static int lua_create_socket(lua_State *L) {
	int port = (int)luaL_checkinteger(L, 1);
	sock_t sock = create_socket(port);
	lua_pushinteger(L, sock);
	return 1;  // ����socket������
}

static int lua_connect_server(lua_State *L) {
	const char *host = luaL_checkstring(L, 1);
	int port = (int)luaL_checkinteger(L, 2);
	int result = connect_server(host, port);
	lua_pushinteger(L, result);
	return 1;  // �������ӽ��
}

static int lua_socket_send_data(lua_State *L) {
	// ������������������Ҫ socket �� bytestream ����
	if (lua_gettop(L) < 2) {
		return luaL_error(L, "Expecting at least 2 arguments (sock, bytestream)");
	}
	// ��ȡ��һ��������socket
	sock_t sock = (sock_t)luaL_checkinteger(L, 1);

	// ��ȡ�ڶ���������bytestream ����
	bytestream** ud = (bytestream**)luaL_checkudata(L, 2, "bytestream");
	if (!ud || !*ud) {
		return luaL_error(L, "Invalid bytestream object");
	}
	bytestream* stream = *ud;

	// ����ʵ�ʵķ��ͺ���
	int result = socket_send_data(sock, stream);
	// ���������ջ��
	lua_pushinteger(L, result);
	return 1;  // ���ط��ͽ��
}

// ��ӹر�socket����
static int lua_close_socket(lua_State *L) {
	sock_t sock = luaL_checkinteger(L, 1);
	safe_close_socket(&sock);
	return 0;
}

static int lua_socket_tostring(lua_State *L){
	sock_t sock = luaL_checkinteger(L, 1);
	lua_pushfstring(L, "network.socket(%d)", sock);
	return 1;
}

//ע�����к���
static const luaL_Reg network_lib[] = {
	{ "create_socket", lua_create_socket },
	{ "connect_server", lua_connect_server },
	{ "socket_send_data", lua_socket_send_data },
	{ NULL, NULL }
};

//ģ���ʼ��
int luaopen_network(lua_State* L){
	// 1. ����ģ����������е��������� 
	luaL_newlib(L, network_lib);  // ģ�����ջ����λ��-1�� 

	// 2. ����Ԫ�������ֽ������� 
	luaL_newmetatable(L, NETWORK_METATABLE);  // Ԫ����ջ����λ��-1����ģ�������λ��-2 

	// 3. ����Ԫ���__indexΪģ����ؼ���֧����������﷨�� 
	lua_pushvalue(L, -2);  // ��ģ���ѹջ��λ��-1�� 
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = ģ���ջ��Ԫ�ص����� 

	// 4. ����Ԫ���__gc�����������Զ��ر�socket��
	lua_pushcfunction(L, lua_close_socket);
	lua_setfield(L, -2, "__gc");

	// 5. ����Ԫ���__tostring����
	lua_pushcfunction(L, lua_socket_tostring);
	lua_setfield(L, -2, "__tostring");

	// 6. ��Ԫ����ջ��ֻ����ģ���
	lua_pop(L, 1);  // ����Ԫ������ջ����ģ���

	return 1;  // ����ģ���
}
#include "public.h"
#include "core/network.h"
#include "core/netsocket.h"
#include "core/bytestream.h"

//application对象元表
static const char* NETWORK_METATABLE = "network";

static int lua_create_socket(lua_State *L) {
	int port = (int)luaL_checkinteger(L, 1);
	sock_t sock = create_socket(port);
	lua_pushinteger(L, sock);
	return 1;  // 返回socket描述符
}

static int lua_connect_server(lua_State *L) {
	const char *host = luaL_checkstring(L, 1);
	int port = (int)luaL_checkinteger(L, 2);
	int result = connect_server(host, port);
	lua_pushinteger(L, result);
	return 1;  // 返回连接结果
}

static int lua_socket_send_data(lua_State *L) {
	// 检查参数数量（至少需要 socket 和 bytestream 对象）
	if (lua_gettop(L) < 2) {
		return luaL_error(L, "Expecting at least 2 arguments (sock, bytestream)");
	}
	// 获取第一个参数：socket
	sock_t sock = (sock_t)luaL_checkinteger(L, 1);

	// 获取第二个参数：bytestream 对象
	bytestream** ud = (bytestream**)luaL_checkudata(L, 2, "bytestream");
	if (!ud || !*ud) {
		return luaL_error(L, "Invalid bytestream object");
	}
	bytestream* stream = *ud;

	// 调用实际的发送函数
	int result = socket_send_data(sock, stream);
	// 将结果推入栈中
	lua_pushinteger(L, result);
	return 1;  // 返回发送结果
}

// 添加关闭socket功能
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

//注册所有函数
static const luaL_Reg network_lib[] = {
	{ "create_socket", lua_create_socket },
	{ "connect_server", lua_connect_server },
	{ "socket_send_data", lua_socket_send_data },
	{ NULL, NULL }
};

//模块初始化
int luaopen_network(lua_State* L){
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, network_lib);  // 模块表在栈顶（位置-1） 

	// 2. 创建元表（用于字节流对象） 
	luaL_newmetatable(L, NETWORK_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 设置元表的__gc方法（用于自动关闭socket）
	lua_pushcfunction(L, lua_close_socket);
	lua_setfield(L, -2, "__gc");

	// 5. 设置元表的__tostring方法
	lua_pushcfunction(L, lua_socket_tostring);
	lua_setfield(L, -2, "__tostring");

	// 6. 将元表弹出栈，只保留模块表
	lua_pop(L, 1);  // 弹出元表，现在栈顶是模块表

	return 1;  // 返回模块表
}
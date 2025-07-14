#include "public.h"
#include "database/mysqlconn.h"

// 定义用户数据结构体
typedef struct {
	MYSQL_RES* result;
	MYSQL_ROW row;
	unsigned long* lengths;
	int field_count;
	MYSQL_FIELD* fields;
	MYSQL* conn;        // 执行查询使用的连接
} LuaMysqlRes;


// 元表名称
static const char* RES_METATABLE = "MYSQL_RES";
// 元表名称
static const char* POOL_METATABLE = "MYSQL_CONN_POOL";


// 元表__gc方法 
static int lua_mysql_result_gc(lua_State* L) {
	LuaMysqlRes* res = (LuaMysqlRes*)luaL_checkudata(L, 1, RES_METATABLE);
	// 释放结果集
	if (res->result) {
		mysql_free_result(res->result);
		res->result = NULL;
	}
	// 释放连接
	if (res->conn) {
		release_connection_to_pool(res->conn);
		res->conn = NULL;
	}
	return 0;
}

// 获取下一行数据 
static int lua_mysql_result_fetch(lua_State* L) {
	LuaMysqlRes* res = (LuaMysqlRes*)luaL_checkudata(L, 1, RES_METATABLE);

	if ((res->row = mysql_fetch_row(res->result))) {
		res->lengths = mysql_fetch_lengths(res->result);
		lua_newtable(L);  // 创建行数据表

		for (int i = 0; i < res->field_count; i++) {
			// 添加字段名作为键
			lua_pushstring(L, res->fields[i].name);

			// 处理NULL值
			if (res->row[i] == NULL) {
				lua_pushnil(L);
			}
			else {
				// 根据字段类型转换数据 
				switch (res->fields[i].type) {
				case MYSQL_TYPE_LONG:
				case MYSQL_TYPE_INT24:
					lua_pushinteger(L, strtol(res->row[i], NULL, 10));
					break;
				case MYSQL_TYPE_DOUBLE:
					lua_pushnumber(L, strtod(res->row[i], NULL));
					break;
				default:
					lua_pushlstring(L, res->row[i], res->lengths[i]);
					break;
				}
			}
			lua_settable(L, -3);
		}
		return 1;
	}
	// 结果集处理完毕，释放连接
	if (res->conn) {
		release_connection_to_pool(res->conn);
		res->conn = NULL;
	}
	return 0;  // 无更多数据时返回nil 
}

// 执行SQL查询
static int lua_mysql_query(lua_State* L) {
	const char* query = luaL_checkstring(L, 1);

	// 从连接池获取连接
	MYSQL* conn = get_connection_from_pool();
	if (!conn) {
		lua_pushnil(L);
		lua_pushstring(L, "no available connection");
		return 2;
	}

	// 执行查询
	if (mysql_query(conn, query)) {
		release_connection_to_pool(conn);
		lua_pushnil(L);
		lua_pushstring(L, mysql_error(conn));
		return 2;
	}

	// 获取结果集
	MYSQL_RES* result = mysql_store_result(conn);
	if (!result) {
		// 对于UPDATE/INSERT等无结果集查询
		if (mysql_field_count(conn) == 0) {
			release_connection_to_pool(conn);
			lua_pushinteger(L, mysql_affected_rows(conn));
			return 1;
		}

		release_connection_to_pool(conn);
		lua_pushnil(L);
		lua_pushstring(L, mysql_error(conn));
		return 2;
	}

	// 创建用户数据 
	LuaMysqlRes* res = (LuaMysqlRes*)lua_newuserdata(L, sizeof(LuaMysqlRes));
	memset(res, 0, sizeof(LuaMysqlRes));

	res->result = result;
	res->conn = conn; // 保存连接，结果集处理完毕后释放
	res->field_count = mysql_num_fields(result);
	res->fields = mysql_fetch_fields(result);

	// 设置元表 
	luaL_getmetatable(L, RES_METATABLE);
	lua_setmetatable(L, -2);

	return 1;
}

// 初始化连接池
static int lua_mysql_init_pool(lua_State* L) {
	const char* host = luaL_checkstring(L, 1);
	const char* user = luaL_checkstring(L, 2);
	const char* passwd = luaL_checkstring(L, 3);
	const char* db = luaL_checkstring(L, 4);
	int port = (int)luaL_checkinteger(L, 5);
	int pool_size = (int)luaL_optinteger(L, 6, 5);   // 默认5个连接
	int max_size = (int)luaL_optinteger(L, 7, 20);   // 默认最大20个连接
	int timeout = (int)luaL_optinteger(L, 8, 300);   // 默认超时300秒

	if (init_connection_pool(host, user, passwd, db, port, pool_size, max_size, timeout)) {
		lua_pushboolean(L, 1);
	}
	else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

// 关闭连接池
static int lua_mysql_destoty_pool(lua_State* L) {
	destoty_connection_pool();
	lua_pushboolean(L, 1);
	return 1;
}

// 连接池元表__gc方法
static int lua_mysql_pool_gc(lua_State* L) {
	destoty_connection_pool();
	return 0;
}

// 注册模块函数
static const luaL_Reg mysql_lib[] = {
	{ "query", lua_mysql_query },
	{ "init_pool", lua_mysql_init_pool },
	{ "close_pool", lua_mysql_destoty_pool },
	{ NULL, NULL }
};

// 注册结果集元表方法
static const luaL_Reg res_meta[] = {
	{ "fetch", lua_mysql_result_fetch },
	{ "__gc", lua_mysql_result_gc },
	{ NULL, NULL }
};

// 注册连接池元表方法
static const luaL_Reg pool_meta[] = {
	{ "__gc", lua_mysql_pool_gc },
	{ NULL, NULL }
};

// 模块初始化
int luaopen_mysql(lua_State* L) {
	// 1. 创建模块表（包含导出函数）
	luaL_newlib(L, mysql_lib);  // 栈位置1：模块表
	// 2. 创建结果集元表
	luaL_newmetatable(L, RES_METATABLE);  // 栈位置2：元表
	// 3. 设置结果集元表的方法
	luaL_setfuncs(L, res_meta, 0);
	// 4. 设置元表的__index指向自身
	lua_pushvalue(L, -1);        // 复制元表到栈顶（位置3）
	lua_setfield(L, -2, "__index");  // 元表.__index = 元表（弹出位置3）
	// 5. 弹出结果集元表
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）

	// 6. 创建连接池元表
	luaL_newmetatable(L, POOL_METATABLE);  // 栈位置2：连接池元表
	// 7. 设置连接池元表的方法
	luaL_setfuncs(L, pool_meta, 0);
	// 8. 设置元表的__index指向自身
	lua_pushvalue(L, -1);        // 复制元表到栈顶（位置3）
	lua_setfield(L, -2, "__index");  // 元表.__index = 元表（弹出位置3）
	// 9. 弹出连接池元表
	lua_pop(L, 1);  // 弹出元表（位置2），现在栈顶是模块表（位置1）
	// 10. 返回模块表
	return 1;
}
#include "public.h"
#include "database/mysqlconn.h"

// �����û����ݽṹ��
typedef struct {
	MYSQL_RES* result;
	MYSQL_ROW row;
	unsigned long* lengths;
	int field_count;
	MYSQL_FIELD* fields;
	MYSQL* conn;        // ִ�в�ѯʹ�õ�����
} LuaMysqlRes;


// Ԫ������
static const char* RES_METATABLE = "MYSQL_RES";
// Ԫ������
static const char* POOL_METATABLE = "MYSQL_CONN_POOL";


// Ԫ��__gc���� 
static int lua_mysql_result_gc(lua_State* L) {
	LuaMysqlRes* res = (LuaMysqlRes*)luaL_checkudata(L, 1, RES_METATABLE);
	// �ͷŽ����
	if (res->result) {
		mysql_free_result(res->result);
		res->result = NULL;
	}
	// �ͷ�����
	if (res->conn) {
		release_connection_to_pool(res->conn);
		res->conn = NULL;
	}
	return 0;
}

// ��ȡ��һ������ 
static int lua_mysql_result_fetch(lua_State* L) {
	LuaMysqlRes* res = (LuaMysqlRes*)luaL_checkudata(L, 1, RES_METATABLE);

	if ((res->row = mysql_fetch_row(res->result))) {
		res->lengths = mysql_fetch_lengths(res->result);
		lua_newtable(L);  // ���������ݱ�

		for (int i = 0; i < res->field_count; i++) {
			// ����ֶ�����Ϊ��
			lua_pushstring(L, res->fields[i].name);

			// ����NULLֵ
			if (res->row[i] == NULL) {
				lua_pushnil(L);
			}
			else {
				// �����ֶ�����ת������ 
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
	// �����������ϣ��ͷ�����
	if (res->conn) {
		release_connection_to_pool(res->conn);
		res->conn = NULL;
	}
	return 0;  // �޸�������ʱ����nil 
}

// ִ��SQL��ѯ
static int lua_mysql_query(lua_State* L) {
	const char* query = luaL_checkstring(L, 1);

	// �����ӳػ�ȡ����
	MYSQL* conn = get_connection_from_pool();
	if (!conn) {
		lua_pushnil(L);
		lua_pushstring(L, "no available connection");
		return 2;
	}

	// ִ�в�ѯ
	if (mysql_query(conn, query)) {
		release_connection_to_pool(conn);
		lua_pushnil(L);
		lua_pushstring(L, mysql_error(conn));
		return 2;
	}

	// ��ȡ�����
	MYSQL_RES* result = mysql_store_result(conn);
	if (!result) {
		// ����UPDATE/INSERT���޽������ѯ
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

	// �����û����� 
	LuaMysqlRes* res = (LuaMysqlRes*)lua_newuserdata(L, sizeof(LuaMysqlRes));
	memset(res, 0, sizeof(LuaMysqlRes));

	res->result = result;
	res->conn = conn; // �������ӣ������������Ϻ��ͷ�
	res->field_count = mysql_num_fields(result);
	res->fields = mysql_fetch_fields(result);

	// ����Ԫ�� 
	luaL_getmetatable(L, RES_METATABLE);
	lua_setmetatable(L, -2);

	return 1;
}

// ��ʼ�����ӳ�
static int lua_mysql_init_pool(lua_State* L) {
	const char* host = luaL_checkstring(L, 1);
	const char* user = luaL_checkstring(L, 2);
	const char* passwd = luaL_checkstring(L, 3);
	const char* db = luaL_checkstring(L, 4);
	int port = (int)luaL_checkinteger(L, 5);
	int pool_size = (int)luaL_optinteger(L, 6, 5);   // Ĭ��5������
	int max_size = (int)luaL_optinteger(L, 7, 20);   // Ĭ�����20������
	int timeout = (int)luaL_optinteger(L, 8, 300);   // Ĭ�ϳ�ʱ300��

	if (init_connection_pool(host, user, passwd, db, port, pool_size, max_size, timeout)) {
		lua_pushboolean(L, 1);
	}
	else {
		lua_pushboolean(L, 0);
	}

	return 1;
}

// �ر����ӳ�
static int lua_mysql_destoty_pool(lua_State* L) {
	destoty_connection_pool();
	lua_pushboolean(L, 1);
	return 1;
}

// ���ӳ�Ԫ��__gc����
static int lua_mysql_pool_gc(lua_State* L) {
	destoty_connection_pool();
	return 0;
}

// ע��ģ�麯��
static const luaL_Reg mysql_lib[] = {
	{ "query", lua_mysql_query },
	{ "init_pool", lua_mysql_init_pool },
	{ "close_pool", lua_mysql_destoty_pool },
	{ NULL, NULL }
};

// ע������Ԫ����
static const luaL_Reg res_meta[] = {
	{ "fetch", lua_mysql_result_fetch },
	{ "__gc", lua_mysql_result_gc },
	{ NULL, NULL }
};

// ע�����ӳ�Ԫ����
static const luaL_Reg pool_meta[] = {
	{ "__gc", lua_mysql_pool_gc },
	{ NULL, NULL }
};

// ģ���ʼ��
int luaopen_mysql(lua_State* L) {
	// 1. ����ģ�����������������
	luaL_newlib(L, mysql_lib);  // ջλ��1��ģ���
	// 2. ���������Ԫ��
	luaL_newmetatable(L, RES_METATABLE);  // ջλ��2��Ԫ��
	// 3. ���ý����Ԫ��ķ���
	luaL_setfuncs(L, res_meta, 0);
	// 4. ����Ԫ���__indexָ������
	lua_pushvalue(L, -1);        // ����Ԫ��ջ����λ��3��
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = Ԫ������λ��3��
	// 5. ���������Ԫ��
	lua_pop(L, 1);  // ����Ԫ��λ��2��������ջ����ģ���λ��1��

	// 6. �������ӳ�Ԫ��
	luaL_newmetatable(L, POOL_METATABLE);  // ջλ��2�����ӳ�Ԫ��
	// 7. �������ӳ�Ԫ��ķ���
	luaL_setfuncs(L, pool_meta, 0);
	// 8. ����Ԫ���__indexָ������
	lua_pushvalue(L, -1);        // ����Ԫ��ջ����λ��3��
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = Ԫ������λ��3��
	// 9. �������ӳ�Ԫ��
	lua_pop(L, 1);  // ����Ԫ��λ��2��������ջ����ģ���λ��1��
	// 10. ����ģ���
	return 1;
}
#include "public.h"
#include "utils/cjson.h"
// 最大递归深度限制
#define MAX_JSON_DEPTH 100

// 模块元表名称
static const char* JSON_METATABLE = "cjson";

// 辅助函数声明
static cJSON* parse_lua_value(lua_State* L, int index, int depth);
static void push_cjson_value(lua_State* L, cJSON* item, int depth);

// 区分数组和对象
static int is_array(lua_State* L, int index) {
	// 获取表长度
	lua_len(L, index);
	int len = lua_tointeger(L, -1);
	lua_pop(L, 1);  // 弹出长度

	lua_pushnil(L);
	int count = 0;
	while (lua_next(L, index) != 0) {
		// 检查键类型
		if (lua_type(L, -2) != LUA_TNUMBER) {
			lua_pop(L, 2);  // 弹出键和值
			return 0; // 非数字键，是对象
		}
		lua_pop(L, 1);  // 弹出值，保留键用于下一次迭代
		count++;
	}
	return (count == len) ? 1 : 0; // 所有键都是连续整数则是数组
}

// 递归解析Lua表
static cJSON* parse_lua_table(lua_State* L, int index, int depth) {
	if (depth > MAX_JSON_DEPTH) {
		luaL_error(L, "JSON nesting too deep (max %d)", MAX_JSON_DEPTH);
	}

	if (is_array(L, index)) {
		// 处理数组
		cJSON* array = cJSON_CreateArray();
		lua_pushnil(L);
		while (lua_next(L, index) != 0) {
			// 只处理数字键（数组部分）
			if (lua_type(L, -2) == LUA_TNUMBER) {
				cJSON_AddItemToArray(array, parse_lua_value(L, -1, depth + 1));
			}
			lua_pop(L, 1);  // 弹出值，保留键用于下一次迭代
		}
		return array;
	}
	else {
		// 处理对象
		cJSON* object = cJSON_CreateObject();
		lua_pushnil(L);
		while (lua_next(L, index) != 0) {
			// 只处理字符串键
			if (lua_type(L, -2) == LUA_TSTRING) {
				const char* key = lua_tostring(L, -2);
				cJSON_AddItemToObject(object, key, parse_lua_value(L, -1, depth + 1));
			}
			lua_pop(L, 1);  // 弹出值，保留键用于下一次迭代
		}
		return object;
	}
}

// 处理Lua值
static cJSON* parse_lua_value(lua_State* L, int index, int depth) {
	switch (lua_type(L, index)) {
	case LUA_TTABLE:
		return parse_lua_table(L, index, depth + 1);
	case LUA_TSTRING:
		return cJSON_CreateString(lua_tostring(L, index));
	case LUA_TNUMBER:
		return cJSON_CreateNumber(lua_tonumber(L, index));
	case LUA_TBOOLEAN:
		return cJSON_CreateBool(lua_toboolean(L, index));
	default:
		return cJSON_CreateNull();
	}
}

// 递归转换cJSON对象到Lua
static void push_cjson_value(lua_State* L, cJSON* item, int depth) {
	if (depth > MAX_JSON_DEPTH) {
		luaL_error(L, "JSON nesting too deep (max %d)", MAX_JSON_DEPTH);
	}

	switch (item->type) {
	case cJSON_Object:
		lua_newtable(L);
		cJSON* child;
		cJSON_ArrayForEach(child, item) {
			push_cjson_value(L, child, depth + 1);
			lua_setfield(L, -2, child->string);
		}
		break;
	case cJSON_Array:
		lua_newtable(L);
		int i = 1;
		cJSON_ArrayForEach(child, item) {
			push_cjson_value(L, child, depth + 1);
			lua_rawseti(L, -2, i++);
		}
		break;
	case cJSON_String:
		lua_pushstring(L, item->valuestring);
		break;
	case cJSON_Number:
		lua_pushnumber(L, item->valuedouble);
		break;
	case cJSON_True:
		lua_pushboolean(L, 1);
		break;
	case cJSON_False:
		lua_pushboolean(L, 0);
		break;
	default:
		lua_pushnil(L);
	}
}

// JSON编码函数
static int lua_json_encode(lua_State* L) {
	// 检查参数类型
	luaL_checktype(L, 1, LUA_TTABLE);

	// 递归转换Lua表为cJSON对象
	cJSON* json = parse_lua_table(L, 1, 0);

	// 生成JSON字符串
	char* json_str = cJSON_PrintUnformatted(json);
	lua_pushstring(L, json_str);

	// 清理内存
	cJSON_Delete(json);
	free(json_str);

	return 1; // 返回1个结果
}

// JSON解码函数
static int lua_json_decode(lua_State* L) {
	// 获取JSON字符串
	const char* json_str = luaL_checkstring(L, 1);

	// 解析JSON
	cJSON* json = cJSON_Parse(json_str);
	if (!json) {
		// 解析失败
		lua_pushnil(L);
		lua_pushstring(L, "parse error");
		return 2; // 返回两个值：nil和错误信息
	}

	// 递归转换cJSON到Lua表
	push_cjson_value(L, json, 0);

	// 清理内存
	cJSON_Delete(json);

	return 1; // 返回解析后的Lua表
}

// 模块函数注册
static const luaL_Reg jsonlib[] = {
	{ "encode", lua_json_encode },
	{ "decode", lua_json_decode },
	{ NULL, NULL }
};

// 模块初始化函数
int luaopen_cjson(lua_State* L) {
	// 1. 创建模块表
	luaL_newlib(L, jsonlib);

	// 2. 创建元表（如果需要）
	luaL_newmetatable(L, JSON_METATABLE);

	// 3. 设置元表属性（如果需要）
	lua_pushvalue(L, -1); // 复制元表
	lua_setfield(L, -2, "__index"); // 元表.__index = 元表

	// 4. 弹出元表（保留模块表）
	lua_pop(L, 1);

	// 5. 返回模块表
	return 1;
}
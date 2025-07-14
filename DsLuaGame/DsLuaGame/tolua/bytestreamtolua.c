#include "public.h" 
#include "core/bytestream.h" 

// 字节流对象元表 
static const char* BYTE_STREAM_METATABLE = "bytestream";

/*
// 辅助宏：检查参数是否为 userdata 并获取 bytestream 指针
#define check_bytestream(L, idx) \
    (*(bytestream**)luaL_checkudata(L, idx, "bytestream"))
	*/

// 检查是否为有效字节流对象 
static bytestream* check_byte_stream(lua_State* L, int idx) {
	// 1. 检查idx位置的对象是否是BYTE_STREAM_METATABLE元表的userdata 
	bytestream** ud = (bytestream**)luaL_checkudata(L, idx, BYTE_STREAM_METATABLE);
	// 2. 验证userdata非空且内部stream指针有效 
	luaL_argcheck(L, ud != NULL && *ud != NULL, idx, "invalid byte stream object");

	return *ud; // 返回C层的bytestream*指针 
}

// 创建字节流对象 
static int lua_bytestream_create(lua_State* L) {
	int len = (int)luaL_checkinteger(L, 1); // 检查输入参数（长度） 
	bytestream* stream = bytestream_create(len); // 调用C层创建函数 
	if (!stream) { // 内存分配失败 
		lua_pushnil(L);
		lua_pushstring(L, "out of memory");
		return 2; // 返回nil+错误信息 
	}
	// 1. 创建Lua userdata，存储bytestream*指针（大小为sizeof(bytestream*)） 
	bytestream** ud = (bytestream**)lua_newuserdata(L, sizeof(bytestream*));
	*ud = stream; // 将stream指针存入userdata 
	// 2. 设置userdata的元表（用于类型检查和GC） 
	luaL_setmetatable(L, BYTE_STREAM_METATABLE);

	return 1; // 返回1个值（userdata对象） 
}

// 从现有缓冲区创建 
static int lua_bytestream_createfrombuffer(lua_State* L) {
	size_t len;
	const char* buf = luaL_checklstring(L, 1, &len); // 检查输入缓冲区 
	bytestream* stream = bytestream_createfrombuffer(buf, (int)len); // 调用C层函数 
	if (!stream) { // 缓冲区无效或内存失败 
		lua_pushnil(L);
		lua_pushstring(L, "invalid buffer or out of memory");
		return 2;
	}

	// 1. 创建userdata并存储stream指针 
	bytestream** ud = (bytestream**)lua_newuserdata(L, sizeof(bytestream*));
	*ud = stream;
	// 2. 设置元表 
	luaL_setmetatable(L, BYTE_STREAM_METATABLE);

	return 1; // 返回userdata对象 
}

// 销毁 bytestream
static int lua_bytestream_destroy(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	if (stream) {
		bytestream_destroy(stream);
		stream = NULL;
	}
	return 0;
}


// 销毁字节流（由GC自动调用） 
static int lua_bytestream_gc(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, BYTE_STREAM_METATABLE);

	if (ud != NULL && *ud != NULL) {
		bytestream_destroy(*ud); // 销毁内部资源
		free(*ud);               // 释放结构体内存
		*ud = NULL;
	}
	return 0;

}

// 清空缓冲区 
static int lua_bytestream_clear(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	bytestream_clear(stream);
	return 0;
}

// 重置读取索引 
static int lua_bytestream_reset(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	bytestream_reset(stream);
	return 0;
}

// 获取剩余字节数 
static int lua_bytestream_getbytesleft(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	lua_pushinteger(L, bytestream_getbytesleft(stream));
	return 1;
}

static int lua_bytestream_getdata(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	lua_pushlstring(L, bytestream_getdata(stream), bytestream_getdatasize(stream));
	return 1;
}

static int lua_bytestream_getdatasize(lua_State* L)
{
	bytestream* stream = check_byte_stream(L, 1);
	lua_pushinteger(L, bytestream_getdatasize(stream));
	return 1;
}

// 追加数据 
static int lua_bytestream_append(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	
	// 获取第二个参数：bytestream 对象
	bytestream** ud = (bytestream**)luaL_checkudata(L, 2, "bytestream");
	if (!ud || !*ud) {
		return luaL_error(L, "Invalid bytestream object");
	}
	bytestream* inputstream = *ud;

	CDFErrorCode err = bytestream_append(stream, bytestream_getdata(inputstream), bytestream_getdatasize(inputstream));
	if (err != CDF_SUCCESS) {
		lua_pushnil(L);
		lua_pushstring(L, "append failed");
		return 2;
	}
	lua_pushboolean(L, 1);
	return 1;
}

// 写入单字节
static int lua_bytestream_writebyte(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	lua_Integer value = luaL_checkinteger(L, 2);
	bytestream_writebyte(stream, (byte)value);
	return 0;
}

// 读取单字节
static int lua_bytestream_readbyte(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	byte value = bytestream_readbyte(stream);
	lua_pushinteger(L, value);
	return 1;
}

// 写入字节数组
static int lua_bytestream_writebytearray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	size_t len;
	const char* data = luaL_checklstring(L, 2, &len);

	CDFErrorCode err = bytestream_writebytearray(stream, (const byte*)data, (int)len);
	lua_pushboolean(L, err == CDF_SUCCESS);
	return 1;
}

// 读取字节数组
static int lua_bytestream_readbytearray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	int len = (int)luaL_checkinteger(L, 2);

	byte* data = bytestream_readbytearray(stream, len);
	if (!data) {
		lua_pushnil(L);
		return 1;
	}

	lua_pushlstring(L, (const char*)data, len);
	free(data);
	return 1;
}

// 写入 bool 值
static int lua_bytestream_writebool(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	int value = lua_toboolean(L, 2);
	bytestream_writebool(stream, value);
	return 0;
}

// 读取 bool 值
static int lua_bytestream_readbool(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	bool value = bytestream_readbool(stream);
	lua_pushboolean(L, value);
	return 1;
}

// 读取 bool 值
static int lua_bytestream_writeboolarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty bool array");
		return 2;
	}

	// 分配内存存储 bool 值
	bool* arr = (bool*)malloc(count * sizeof(bool));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 bool 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = lua_toboolean(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 bool 数组
	CDFErrorCode err = bytestream_writeboolarray(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write bool array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

// 读取 bool 值
static int lua_bytestream_readboolarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	int count;
	bool* arr = bytestream_readboolarray(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read bool array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 bool 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushboolean(L, arr[i]);
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入 short
static int lua_bytestream_writeshort(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	
	lua_Integer value = luaL_checkinteger(L, 2);
	bytestream_writeshort(stream, (short)value);
	return 0;
}

// 读取 short
static int lua_bytestream_readshort(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	
	short value = bytestream_readshort(stream);
	lua_pushinteger(L, value);
	return 1;
}

static int lua_bytestream_writeshortarray(lua_State* L){
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
		if (!lua_istable(L, 2)) {
			lua_pushboolean(L, false);
			lua_pushstring(L, "Argument 2 must be a table");
			return 2;
		}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty short array");
		return 2;
	}

	// 分配内存存储 short 值
	short* arr = (short*)malloc(count * sizeof(short));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 short 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = (short)luaL_checkinteger(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 short 数组
	CDFErrorCode err = bytestream_writeshortarray(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write short array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 1;
}

// 读取 short
static int lua_bytestream_readshortarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	short* arr = bytestream_readshortarray(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read short array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 short 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushinteger(L, arr[i]);
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}



// 写入整数 
static int lua_bytestream_writeint(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	int value = (int)luaL_checkinteger(L, 2);
	bytestream_writeint(stream, value);
	return 0;
}

// 读取整数 
static int lua_bytestream_readint(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	lua_pushinteger(L, bytestream_readint(stream));
	return 1;
}

// 写入整数 
static int lua_bytestream_writeintarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty int array");
		return 2;
	}

	// 分配内存存储 int 值
	int* arr = (int*)malloc(count * sizeof(int));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 int 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = (int)luaL_checkinteger(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 int 数组
	CDFErrorCode err = bytestream_writeintarray(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write int array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 0;
}

// 读取整数 
static int lua_bytestream_readintarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	int* arr = bytestream_readintarray(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read int array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 int 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushinteger(L, arr[i]);
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入 float
static int lua_bytestream_writefloat(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	float value = (float)luaL_checknumber(L, 2);
	bytestream_writefloat(stream, value);
	return 0;
}

// 读取 float
static int lua_bytestream_readfloat(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	float value = bytestream_readfloat(stream);
	lua_pushnumber(L, value);
	return 1;
}


// 写入 float
static int lua_bytestream_writefloatarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty float array");
		return 2;
	}

	// 分配内存存储 float 值
	float* arr = (float*)malloc(count * sizeof(float));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 float 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = (float)luaL_checknumber(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 float 数组
	CDFErrorCode err = bytestream_writefloatarray(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write float array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 0;
}

// 读取 float
static int lua_bytestream_readfloatarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	float* arr = bytestream_readfloatarray(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read float array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 float 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, arr[i]); // 转换为 Lua number (double)
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入 double
static int lua_bytestream_writedouble(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	double value = luaL_checknumber(L, 2);
	bytestream_writedouble(stream, value);
	return 0;
}

// 读取 double
static int lua_bytestream_readdouble(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	double value = bytestream_readdouble(stream);
	lua_pushnumber(L, value);
	return 1;
}

// 写入 double
static int lua_bytestream_writedoublearray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty double array");
		return 2;
	}

	// 分配内存存储 double 值
	double* arr = (double*)malloc(count * sizeof(double));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 double 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = luaL_checknumber(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 double 数组
	CDFErrorCode err = bytestream_writedoublearray(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write double array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 0;
}

// 读取 double
static int lua_bytestream_readdoublearray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	double* arr = bytestream_readdoublearray(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read double array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 double 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, arr[i]); // 转换为 Lua number
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入long64_t 
static int lua_bytestream_writelong64_t(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	lua_Number value = luaL_checknumber(L, 2);
	bytestream_writelong64(stream, (long64_t)value);
	return 0;
}

// 读取 long64_t
static int lua_bytestream_readlong64_t(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	long64_t value = bytestream_readlong64(stream);
	lua_pushnumber(L, (lua_Number)value);
	return 1;
}

// 写入 long64_tarray
static int lua_bytestream_writelong64_tarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty long64 array");
		return 2;
	}

	// 分配内存存储 long64_t 值
	long64_t* arr = (long64_t*)malloc(count * sizeof(long64_t));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 long64_t 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		arr[i] = (long64_t)luaL_checknumber(L, -1);
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 long64_t 数组
	CDFErrorCode err = bytestream_writelong64array(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write long64 array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 0;
}

// 读取 long64_tarray
static int lua_bytestream_readlong64_tarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	long64_t* arr = bytestream_readlong64array(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read long64 array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 long64_t 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, (lua_Number)arr[i]);
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入 uint64_t
static int lua_bytestream_writeuint64_t(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	lua_Number value = luaL_checknumber(L, 2);
	// 确保值是非负的
	if (value < 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "uint64 value cannot be negative");
		return 2;
	}

	bytestream_writeuint64(stream, (uint64_t)value);
	return 0;
}

// 读取 uint64_t
static int lua_bytestream_readuint64_t(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	uint64_t value = bytestream_readuint64(stream);

	// 将 uint64_t 推入 Lua 栈
	lua_pushnumber(L, (lua_Number)value);
	return 1;
}


// 写入 uint64_t
static int lua_bytestream_writeuint64_tarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty uint64 array");
		return 2;
	}

	// 分配内存存储 uint64_t 值
	uint64_t* arr = (uint64_t*)malloc(count * sizeof(uint64_t));
	if (!arr) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Out of memory");
		return 2;
	}

	// 从 Lua 表中提取 uint64_t 值
	for (int i = 0; i < count; i++) {
		lua_rawgeti(L, 2, i + 1); // Lua 索引从 1 开始
		lua_Number value = luaL_checknumber(L, -1);

		// 确保值是非负的
		if (value < 0) {
			free(arr);
			lua_pushboolean(L, false);
			lua_pushstring(L, "uint64 value cannot be negative");
			return 2;
		}

		arr[i] = (uint64_t)value;
		lua_pop(L, 1); // 弹出获取的值
	}

	// 调用 C 函数写入 uint64_t 数组
	CDFErrorCode err = bytestream_writeuint64array(stream, arr, count);
	free(arr); // 释放临时数组

	if (err != CDF_SUCCESS) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Failed to write uint64 array");
		return 2;
	}

	lua_pushboolean(L, true);
	return 0;
}
// 读取 uint64_t
static int lua_bytestream_readuint64_tarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	int count;
	uint64_t* arr = bytestream_readuint64array(stream, &count);

	if (!arr || count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Failed to read uint64 array");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 将 uint64_t 值填充到表中
	for (int i = 0; i < count; i++) {
		lua_pushnumber(L, (lua_Number)arr[i]);
		lua_rawseti(L, -2, i + 1); // Lua 索引从 1 开始
	}

	free(arr); // 释放 C 分配的数组
	return 1;
}

// 写入字符串（包含长度） 
static int lua_bytestream_writestring(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	size_t len;
	const char* str = luaL_checklstring(L, 2, &len);
	bytestream_writestring(stream, str);
	return 0;
}

// 读取字符串 
static int lua_bytestream_readstring(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	char* str = bytestream_readstring(stream);
	if (!str) {
		lua_pushnil(L);
		return 1;
	}
	size_t str_len = strlen(str);
	lua_pushlstring(L, str, str_len);
	free(str); // 假设C函数返回的内存需要释放 
	return 1;
}

// 写入字符串（包含长度） 
static int lua_bytestream_writestringarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 检查第二个参数是否为表
	if (!lua_istable(L, 2)) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Argument 2 must be a table");
		return 2;
	}

	// 获取表的大小
	int count = (int)lua_rawlen(L, 2);
	if (count <= 0) {
		lua_pushboolean(L, false);
		lua_pushstring(L, "Empty string array");
		return 2;
	}

	// 准备写入数组长度
	bytestream_writeint(stream, count);

	// 遍历表并写入每个字符串
	for (int i = 1; i <= count; i++) {
		lua_rawgeti(L, 2, i); // 获取第 i 个元素
		if (lua_type(L, -1) != LUA_TSTRING) {
			lua_pop(L, 1); // 弹出非字符串值
			lua_pushboolean(L, false);
			lua_pushfstring(L, "Element %d is not a string", i);
			return 2;
		}

		const char* str = lua_tostring(L, -1);
		bytestream_writestring(stream, str);
		lua_pop(L, 1); // 弹出字符串
	}

	lua_pushboolean(L, true);
	return 0;
}

// 读取字符串 
static int lua_bytestream_readstringarray(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);

	// 读取数组长度
	int count = bytestream_readint(stream);
	if (count <= 0) {
		lua_pushnil(L);
		lua_pushstring(L, "Invalid string array count");
		return 2;
	}

	// 创建 Lua 表存储结果
	lua_createtable(L, count, 0);

	// 读取每个字符串
	for (int i = 1; i <= count; i++) {
		char* str = bytestream_readstring(stream);
		if (!str) {
			// 释放已读取的字符串
			for (int j = 1; j < i; j++) {
				lua_rawgeti(L, -1, j);
				free((void*)lua_tostring(L, -1));
				lua_pop(L, 1);
			}
			lua_pushnil(L);
			lua_pushfstring(L, "Failed to read string at position %d", i);
			return 2;
		}

		lua_pushstring(L, str);
		free(str); // 释放 C 分配的字符串
		lua_rawseti(L, -2, i); // 存储到表中
	}
	return 1;
}

// 序列化开始 
static int lua_bytestream_startseq(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	int numElements = (int)luaL_checkinteger(L, 2);
	int minSize = (int)luaL_checkinteger(L, 3);
	bytestream_startseq(stream, numElements, minSize);
	return 0;
}

// 序列化结束 
static int lua_bytestream_endseq(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	bytestream_endseq(stream);
	return 0;
}

// 错误处理设置只读 
static int lua_bytestream_setreadonly(lua_State* L) {
	bytestream* stream = check_byte_stream(L, 1);
	bool readOnly = lua_toboolean(L, 2);
	bytestream_setreadonly(stream, readOnly);
	return 0;
}

// 注册所有Lua函数 
static const luaL_Reg bytestream_lib[] = {
	{ "create", lua_bytestream_create },
	{ "createfrombuffer", lua_bytestream_createfrombuffer },
	{ "clear", lua_bytestream_clear },
	{ "reset", lua_bytestream_reset },
	{ "getbytesleft", lua_bytestream_getbytesleft },
	{ "getdata", lua_bytestream_getdata },
	{ "getdatasize", lua_bytestream_getdatasize },
	{ "append", lua_bytestream_append },
	//byte
	{ "writebyte", lua_bytestream_writebyte },
	{ "readbyte", lua_bytestream_readbyte },
	{ "writebytearray", lua_bytestream_writebytearray },
	{ "readbytearray", lua_bytestream_readbytearray },
	//bool
	{ "writebool", lua_bytestream_writebool },
	{ "readbool", lua_bytestream_readbool },
	{ "writeboolarray", lua_bytestream_readboolarray },
	{ "readboolarray", lua_bytestream_readboolarray },
	//short
	{ "writeshort", lua_bytestream_writeshort },
	{ "readshort", lua_bytestream_readshort },
	{ "writeshortarray", lua_bytestream_readshortarray },
	{ "readshortarray", lua_bytestream_readshortarray },
	//int
	{ "writeint", lua_bytestream_writeint },
	{ "readint", lua_bytestream_readint },
	{ "writeintarray", lua_bytestream_readintarray },
	{ "readintarray", lua_bytestream_readintarray },
	//float
	{ "writefloat", lua_bytestream_writefloat },
	{ "readfloat", lua_bytestream_readfloat },
	{ "writefloatarray", lua_bytestream_readfloatarray },
	{ "readfloatarray", lua_bytestream_readfloatarray },
	//double
	{ "writedouble", lua_bytestream_writedouble },
	{ "readdouble", lua_bytestream_readdouble },
	{ "writedoublearray", lua_bytestream_writedoublearray },
	{ "readdoublearray", lua_bytestream_readdoublearray },
	//long64_t
	{ "writelong64_t", lua_bytestream_writelong64_t },
	{ "readlong64_t", lua_bytestream_readlong64_t },
	{ "writelong64_tarray", lua_bytestream_writelong64_tarray },
	{ "readlong64_tarray", lua_bytestream_readlong64_tarray },
	//uint64_t
	{ "writeuint64_t", lua_bytestream_writeuint64_t },
	{ "readuint64_t", lua_bytestream_readuint64_t },
	{ "writeuint64_tarray", lua_bytestream_writeuint64_tarray },
	{ "readuint64_tarray", lua_bytestream_readuint64_tarray },
	//string
	{ "writestring", lua_bytestream_writestring },
	{ "readstring", lua_bytestream_readstring },
	{ "writestringarray", lua_bytestream_writestringarray },
	{ "readstringarray", lua_bytestream_readstringarray },
	{ NULL, NULL }
};

// 模块初始化 
int luaopen_bytestream(lua_State* L) {
	// 1. 创建模块表（包含所有导出函数） 
	luaL_newlib(L, bytestream_lib);  // 模块表在栈顶（位置-1） 

	// 2. 创建元表（用于字节流对象） 
	luaL_newmetatable(L, BYTE_STREAM_METATABLE);  // 元表在栈顶（位置-1），模块表现在位置-2 

	// 3. 设置元表的__index为模块表（关键：支持面向对象语法） 
	lua_pushvalue(L, -2);  // 将模块表压栈（位置-1） 
	lua_setfield(L, -2, "__index");  // 元表.__index = 模块表（栈顶元素弹出） 

	// 4. 设置元表的__gc（自动销毁对象） 
	lua_pushcfunction(L, lua_bytestream_gc);  // __gc函数压栈（位置-1） 
	lua_setfield(L, -2, "__gc");  // 元表.__gc = 函数（栈顶元素弹出） 

	// 5. 弹出元表（现在栈顶是模块表） 
	lua_pop(L, 1);
	return 1;
}
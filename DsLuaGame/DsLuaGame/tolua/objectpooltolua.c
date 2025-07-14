#include "public.h"
#include "objectpool/objectpool.h"

static const char* OBJECTPOOL_METATABLE = "objectpool";
// ��ʼ�������
static int lua_init_bytestream_pool(lua_State* L) {
	size_t initial_capacity = luaL_checkinteger(L, 1);
	objectpool* pool = init_bytestream_pool(initial_capacity);
	lua_pushlightuserdata(L, pool);
	return 1;
}

// ��ȡ bytestream ����
static int lua_obtain_bytestream(lua_State* L) {
	bytestream* bs = obtain_bytestream();
	// ���� full userdata ������Ԫ��
	bytestream** ud = (bytestream**)lua_newuserdata(L, sizeof(bytestream*));
	*ud = bs;
	luaL_getmetatable(L, "bytestream");
	lua_setmetatable(L, -2);

	return 1;
}

// �黹 bytestream ����
static int lua_giveup_bytestream(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	if (*ud) {
		giveup_bytestream(*ud);
		*ud = NULL; // ��ֹ�ظ��ͷ�
	}
	return 0;
}

// ���ٶ����
static int lua_fina_bytestream_pool(lua_State* L) {
	fina_bytestream_pool();
	return 0;
}

// Ԫ������__gc ������������
static int lua_bytestream_gc(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	if (*ud) {
		bytestream_destroy(*ud);
		*ud = NULL;
	}
	return 0;
}

// Ԫ��������ȡ����
static int lua_bytestream_len(lua_State* L) {
	bytestream** ud = (bytestream**)luaL_checkudata(L, 1, "bytestream");
	lua_pushinteger(L, bytestream_getdatasize(*ud));
	return 1;
}

// ����������
static const luaL_Reg objectpool_lib[] = {
	{ "init_bytestream_pool", lua_init_bytestream_pool },
	{ "obtain_bytestream", lua_obtain_bytestream },
	{ "giveup_bytestream", lua_giveup_bytestream },
	{ "fina_bytestream_pool", lua_fina_bytestream_pool },
	{ NULL, NULL }
};

// ע�ắ���� Lua
int luaopen_objectpool(lua_State* L) {
	// 1. ����ģ����������е��������� 
	luaL_newlib(L, objectpool_lib);

	// 2. ����Ԫ�������ֽ������� 
	luaL_newmetatable(L, OBJECTPOOL_METATABLE);  // Ԫ����ջ����λ��-1����ģ�������λ��-2 

	// ����Ԫ����
	lua_pushcfunction(L, lua_bytestream_gc);
	lua_setfield(L, -2, "__gc");

	lua_pushcfunction(L, lua_bytestream_len);
	lua_setfield(L, -2, "__len");

	// 3. ����Ԫ���__indexΪģ����ؼ���֧����������﷨�� 
	lua_pushvalue(L, -2);  // ��ģ���ѹջ��λ��-1�� 
	lua_setfield(L, -2, "__index");  // Ԫ��.__index = ģ���ջ��Ԫ�ص����� 

	// 4. ����ջ������Ԫ������ģ�����Ϊ����ֵ��
	lua_pop(L, 1);  // ����Ԫ��λ��2��������ջ����ģ���λ��1��
	// 5. ����ģ���Lua require���ȡ�����
	return 1;
}
#include "sharemem.h"
#include "public.h"

void* create_shared_memory(const char* name, size_t size) {
#ifdef _WIN32 
	HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,   // 使用物理内存 
		NULL,                   // 默认安全属性 
		PAGE_READWRITE,         // 可读可写
		0,                      // 高位文件大小 
		(DWORD)size,            // 低位文件大小 
		name);                  // 共享内存名称 

	if (hMapFile == NULL) return NULL;

	void* pBuf = MapViewOfFile(
		hMapFile,               // 内存映射对象句柄 
		FILE_MAP_ALL_ACCESS,    // 可读写权限 
		0,
		0,
		size);

	return pBuf;
#else 
	int fd = shm_open(name, O_CREAT | O_RDWR, 0666);
	if (fd == -1) return NULL;

	if (ftruncate(fd, size) == -1) {
		close(fd);
		return NULL;
	}

	void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	return ptr;
#endif 
}



void* map_shared_memory(const char* name, size_t size)
{
#ifdef _WIN32 
	HANDLE hMapFile = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,    // 可读写权限 
		FALSE,                  // 不继承句柄 
		name);                  // 共享内存名称 

	if (hMapFile == NULL) return NULL;

	return MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else 
	int fd = shm_open(name, O_RDWR, 0666);
	if (fd == -1) return NULL;

	void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	return ptr;
#endif 
}

void unmap_shared_memory(void* addr, size_t size)
{
#ifdef _WIN32 
	UnmapViewOfFile(addr);
	CloseHandle(addr);  // 关闭映射对象句柄 
#else 
	munmap(addr, size);
#endif 
}

int load_json_to_shm(const char* shm_name, const char* filename) {
	// 读取JSON文件 
	FILE* fp = fopen(filename, "rb");
	if (!fp) return -1;

	fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	rewind(fp);

	// 计算总内存需求（包含结构体头部）
	size_t total_size = sizeof(SharedJSON) + file_size;
	char* buffer = (char*)malloc(total_size);
	if (!buffer) {
		fclose(fp);
		return -1;
	}

	// 构建内存结构 
	SharedJSON* shm_ptr = (SharedJSON*)buffer;
	strncpy(shm_ptr->filename, filename, sizeof(shm_ptr->filename) - 1);
	shm_ptr->data_size = file_size;
	fread(shm_ptr->json_data, 1, file_size, fp);
	fclose(fp);

	// 写入共享内存 
	void* shm = create_shared_memory(shm_name, total_size);
	if (!shm) {
		free(buffer);
		return -1;
	}

	memcpy(shm, buffer, total_size);
	free(buffer);

#ifdef _WIN32 
	FlushViewOfFile(shm, total_size);  // Windows数据持久化
#endif 
	return 0;
}

const char* get_json_config(const char* shm_name, const char* filename)
{
	// 获取内存映射 
	void* shm = map_shared_memory(shm_name, 0);  // 0表示自动获取大小 
	if (!shm) return NULL;

	SharedJSON* header = (SharedJSON*)shm;

	// 校验文件名匹配性
	if (strncmp(header->filename, filename, sizeof(header->filename)) != 0) {
		unmap_shared_memory(shm, 0);
		return NULL;
	}

	// 校验数据完整性 
	if (header->data_size <= 0 ||
		header->data_size > (1024 * 1024 * 10)) {  // 限制最大10MB 
		unmap_shared_memory(shm, 0);
		return NULL;
	}

	// 返回数据指针（需保持内存映射）
	return header->json_data;
}


static int lua_load_json(lua_State* L) {
	const char* shm_name = luaL_checkstring(L, 1);
	const char* filename = luaL_checkstring(L, 2);
	int ret = load_json_to_shm(shm_name, filename);
	lua_pushinteger(L, ret);
	return 1;
}

static int lua_get_config(lua_State* L) {
	const char* shm_name = luaL_checkstring(L, 1);
	const char* filename = luaL_checkstring(L, 2);
	const char* json = get_json_config(shm_name, filename);
	lua_pushstring(L, json);
	return 1;
}

// 注册Lua模块 
 LUA_API int luaopen_sharemen(lua_State* L) {
	static const luaL_Reg funcs[] = {
		{ "load_json", lua_load_json },
		{ "get_config", lua_get_config },
		{ NULL, NULL }
	};
	luaL_newlib(L, funcs);
	return 1;
}
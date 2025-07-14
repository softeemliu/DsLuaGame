#ifndef __SHARE_MEM_H__
#define __SHARE_MEM_H__

#include <stddef.h>

#ifdef _WIN32 
//#include <windows.h>
#else 
#include <sys/mman.h>
#include <fcntl.h>
#endif 

// 共享内存结构体（需内存对齐）
#pragma pack(push, 1)
typedef struct {
	char filename[256];  // 配置文件名 
	size_t data_size;    // JSON数据大小 
	char json_data[1];   // 柔性数组存储实际数据 
} SharedJSON;
#pragma pack(pop)

// 共享内存操作接口 
void* create_shared_memory(const char* name, size_t size);
void* map_shared_memory(const char* name, size_t size);
void unmap_shared_memory(void* addr, size_t size);
int load_json_to_shm(const char* shm_name, const char* filename);
const char* get_json_config(const char* shm_name, const char* filename);

#endif /*__SHARE_MEM_H__*/
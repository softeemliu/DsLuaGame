#include "mempool.h"

// 单例模式实现 
static mempool* g_mempool_instance = NULL;

mempool* mempool_getinstance() {
	if (!g_mempool_instance)
	{
		g_mempool_instance = (mempool*)malloc(sizeof(mempool));
		mempool_init(g_mempool_instance);
	}
	return g_mempool_instance;
}

// 初始化函数 
void mempool_init(mempool* self) {
	AUTO_LOCK_INIT(&self->_light_lock);
    memset(self->_S_free_list, 0, sizeof(self->_S_free_list));
    self->_S_start_free = NULL;
    self->_S_end_free = NULL;
    self->_S_heap_size = 0;
}
 
// 内存对齐计算 
size_t mempool_s_round_up(size_t bytes) {
    return ((bytes + _ALIGN - 1) & ~(_ALIGN - 1));
}
 
// 计算自由列表索引 
int mempool_s_freelist_index(size_t bytes) {
    return ((bytes + _ALIGN - 1) / _ALIGN - 1);
}
 
// 分配新内存块
char* mempool_s_chunk_alloc(mempool* self, size_t size, int* nobjs) {
    char* result;
    size_t total_bytes = size * *nobjs;
    size_t bytes_left = (self->_S_end_free - self->_S_start_free);
 
    if (bytes_left >= total_bytes) {
        result = self->_S_start_free;
        self->_S_start_free += total_bytes;
        return result;
    } else if (bytes_left >= size) {
        *nobjs = (int)(bytes_left / size);
        total_bytes = size * *nobjs;
        result = self->_S_start_free;
        self->_S_start_free += total_bytes;
        return result;
    } else {
        size_t bytes_to_get = 2 * total_bytes + mempool_s_round_up(self->_S_heap_size >> 4);
        
        if (bytes_left > 0) {
            int index = mempool_s_freelist_index(bytes_left);
            _Obj** my_free_list = &self->_S_free_list[index];
            _Obj* obj = (_Obj*)self->_S_start_free;
            obj->_M_free_list_link = *my_free_list;
            *my_free_list = obj;
        }
 
        self->_S_start_free = (char*)malloc(bytes_to_get);
        if (!self->_S_start_free) {
            size_t i;
            for (i = size; i <= _MAX_BYTES; i += _ALIGN) {
                int index = mempool_s_freelist_index(i);
                _Obj** my_free_list = &self->_S_free_list[index];
                _Obj* obj = *my_free_list;
                if (obj) {
                    *my_free_list = obj->_M_free_list_link;
                    self->_S_start_free = (char*)obj;
                    self->_S_end_free = self->_S_start_free + i;
                    return mempool_s_chunk_alloc(self, size, nobjs);
                }
            }
            self->_S_end_free = NULL;
            self->_S_start_free = (char*)malloc(bytes_to_get);
        }
 
        self->_S_heap_size += bytes_to_get;
        self->_S_end_free = self->_S_start_free + bytes_to_get;
        return mempool_s_chunk_alloc(self, size, nobjs);
    }
}
 
// 构建自由链表 
void* mempool_s_refill(mempool* self, size_t n) {
    int nobjs = 20;
    char* chunk = mempool_s_chunk_alloc(self, n, &nobjs);
    _Obj** my_free_list = &self->_S_free_list[mempool_s_freelist_index(n)];
    _Obj* result = (_Obj*)chunk;
    _Obj* current_obj = (_Obj*)(chunk + n);
    _Obj* next_obj;
 
    *my_free_list = current_obj;
    for (int i = 1; i < nobjs - 1; i++) {
        next_obj = (_Obj*)((char*)current_obj + n);
        current_obj->_M_free_list_link = next_obj;
        current_obj = next_obj;
    }
    current_obj->_M_free_list_link = NULL;
    return result;
}
 
// 重新分配内存 
void* mempool_reallocate(mempool* self, void* p, size_t old_sz, size_t new_sz) {
    void* result;
    size_t copy_sz;
 
    if (old_sz > _MAX_BYTES && new_sz > _MAX_BYTES) {
        return realloc(p, new_sz);
    }
 
    if (mempool_s_round_up(old_sz) == mempool_s_round_up(new_sz)) {
        return p;
    }
 
    result = malloc(new_sz);
    copy_sz = (new_sz > old_sz) ? old_sz : new_sz;
    memcpy(result, p, copy_sz);
    mempool_deallocate(self, p, old_sz);
    return result;
}
 
// 释放内存 
void mempool_deallocate(mempool* self, void* p, size_t n) {
    lightlock_lock(&self->_light_lock); 
    if (n > _MAX_BYTES) {
        free(p);
    } else {
        int index = mempool_s_freelist_index(n);
        _Obj* obj = (_Obj*)p;
        obj->_M_free_list_link = self->_S_free_list[index];
        self->_S_free_list[index] = obj;
    }
    lightlock_unlock(&self->_light_lock); 
}
 
// 分配内存 
void* mempool_allocate(mempool* self, size_t n) {
    lightlock_lock(&self->_light_lock); 
    void* ret = NULL;
 
    if (n > _MAX_BYTES) {
        ret = malloc(n);
    } else {
        int index = mempool_s_freelist_index(n);
        _Obj** my_free_list = &self->_S_free_list[index];
        _Obj* result = *my_free_list;
 
        if (!result) {
            ret = mempool_s_refill(self, mempool_s_round_up(n));
        } else {
            *my_free_list = result->_M_free_list_link;
            ret = result;
        }
    }
 
    lightlock_unlock(&self->_light_lock); 
    return ret;
}
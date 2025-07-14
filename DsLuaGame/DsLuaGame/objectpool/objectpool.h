#ifndef __OBJECT_POOL_H__
#define __OBJECT_POOL_H__
#include "public.h"
#include "utils/lightlock.h"
#include "core/bytestream.h"
#include <stdarg.h>

// ��������ؽṹ
typedef struct {
	void** pool;        // ����ָ������
	size_t capacity;    // ������
	size_t index;       // ��ǰ���ö�������
	bool final;         // �Ƿ�������
	light_lock lock; // ������
} objectpool;


// ��ʼ�������
objectpool* objectpool_create(size_t initial_capacity);
// ���ٶ����
void objectpool_destroy(objectpool* pool, void(*destructor)(void*));
// �ӳ��л�ȡ����
void* objectpool_acquire(objectpool* pool, void* (*constructor)(va_list args), ...);
// �ͷŶ���س���
void objectpool_release(objectpool* pool, void* obj, void(*reset)(void*), void(*destructor)(void*));

//���������
objectpool* init_bytestream_pool(size_t initial_capacity);
bytestream* obtain_bytestream();
void giveup_bytestream(bytestream* btstream);
void fina_bytestream_pool();


#endif /*__OBJECT_POOL_H__*/
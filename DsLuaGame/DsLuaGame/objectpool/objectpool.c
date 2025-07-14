#include "objectpool/objectpool.h"

static objectpool* bufferpool = NULL;
// ��ʼ�������
objectpool* objectpool_create(size_t initial_capacity) {
	if (initial_capacity == 0) return NULL;

	objectpool* pool = (objectpool*)malloc(sizeof(objectpool));
	if (!pool) return NULL;

	pool->pool = (void**)malloc(initial_capacity * sizeof(void*));
	if (!pool->pool) {
		free(pool);
		return NULL;
	}

	pool->capacity = initial_capacity;
	pool->index = 0;
	pool->final = false;
	AUTO_LOCK_INIT(&pool->lock);

	return pool;
}

// ���ٶ����
void objectpool_destroy(objectpool* pool, void(*destructor)(void*)) {
	if (!pool) return;

	AUTO_LOCK(&pool->lock);
	pool->final = true;
	AUTO_UNLOCK(&pool->lock);

	for (size_t i = 0; i < pool->index; i++) {
		if (destructor && pool->pool[i]) {
			destructor(pool->pool[i]);
		}
	}

	free(pool->pool);
	AUTO_LOCK_DESTORY(&pool->lock);
	free(pool);
}

// �ӳ��л�ȡ����
void* objectpool_acquire(objectpool* pool, void* (*constructor)(va_list args), ...) {
	if (!pool || pool->final) return NULL;

	void* obj = NULL;

	AUTO_LOCK(&pool->lock);
	if (pool->index > 0) {
		pool->index--;
		obj = pool->pool[pool->index];
	}
	AUTO_UNLOCK(&pool->lock);

	if (!obj && constructor) {
		va_list args;
		va_start(args, constructor);
		obj = constructor(args);
		va_end(args);
	}

	return obj;
}

// �ͷŶ���س���
void objectpool_release(objectpool* pool, void* obj, void(*reset)(void*), void(*destructor)(void*)) {
	if (!pool || !obj || pool->final) return;

	if (reset) {
		reset(obj);
	}

	AUTO_LOCK(&pool->lock);
	if (pool->index >= pool->capacity) {
		// ��Ҫ����
		size_t new_capacity = pool->capacity * 2;
		void** new_pool = (void**)realloc(pool->pool, new_capacity * sizeof(void*));
		if (!new_pool) {
			// ����ʧ��ʱ��ȫ���ٶ���
			AUTO_UNLOCK(&pool->lock);
			if (destructor) destructor(obj);
			return;
		}
		pool->pool = new_pool;
		pool->capacity = new_capacity;
	}

	if (pool->index < pool->capacity) {
		pool->pool[pool->index] = obj;
		pool->index++;
	}
	AUTO_UNLOCK(&pool->lock);
}


//////////////////////////////////////////////////////////////////////////
// �������������� va_list ����ת��Ϊ bytestream_create ��Ҫ�Ĳ���
static void* bytestream_create_adapter(va_list args) {
	int size = va_arg(args, int);  // ��ȡ�ɱ�����е� int ֵ
	return bytestream_create(size);  // ����ʵ�ʹ��캯��
}

objectpool* init_bytestream_pool(size_t initial_capacity)
{
	bufferpool = objectpool_create(initial_capacity);
	return bufferpool;
}

bytestream* obtain_bytestream()
{
	void* obj = objectpool_acquire(bufferpool, bytestream_create_adapter, 1024);
	return (bytestream*)obj;
}

void giveup_bytestream(bytestream* btstream)
{
	objectpool_release(bufferpool, (void*)btstream, (void(*)(void*))bytestream_clear,
		(void(*)(void*))bytestream_destroy);
}

void fina_bytestream_pool()
{
	// ���ٶ����
	objectpool_destroy(bufferpool, (void(*)(void*))bytestream_destroy);
}
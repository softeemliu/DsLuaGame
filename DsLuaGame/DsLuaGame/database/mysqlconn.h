#ifndef __MYSQL_CONNECTION_H__
#define __MYSQL_CONNECTION_H__
#include "public.h"
#include <mysql.h>
#include "utils/lightlock.h"


/* 连接池结构体 */
typedef struct {
	MYSQL* conn;			// MySQL连接 
	int in_use;				 // 使用标记 
	long long last_used;	// 最后使用时间 
} PooledConnection;

/* 连接池管理器 */
typedef struct {
	PooledConnection* connections;    // 连接数组 
	int size;           // 连接池大小
	int max_size;       // 最大连接数
	int timeout;        // 连接超时时间(秒)
	char* host;                 // 数据库地址 
	char* user;                 // 用户名 
	char* passwd;               // 密码 
	char* db;                   // 数据库名
	int port;
	light_lock lock; // 线程锁
} ConnectionPool;

/*初始化db连接*/
int init_connection_pool(const char* host, const char* user,
	const char* passwd, const char* db,
	unsigned int port, int pool_size, int timeout, int max_size);

/* 获取数据库连接 */
MYSQL* get_connection_from_pool();

/* 释放连接 */
void release_connection_to_pool(MYSQL* conn);

/* 销毁连接池 */
void destoty_connection_pool();

#endif
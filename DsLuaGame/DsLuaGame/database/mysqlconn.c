#include "mysqlconn.h"
#include <time.h>

/* 单例实例 */
static ConnectionPool* g_conn_pool = NULL;

/* 初始化连接池 */  
int init_connection_pool(const char* host, const char* user,
	const char* passwd, const char* db,
	unsigned int port, int pool_size, int timeout, int max_size) {
	if (!g_conn_pool) {
		g_conn_pool = (ConnectionPool*)malloc(sizeof(ConnectionPool));
		if (!g_conn_pool) return 0;

		memset(g_conn_pool, 0, sizeof(ConnectionPool));
		g_conn_pool->size = pool_size;
		g_conn_pool->max_size = max_size;
		g_conn_pool->timeout = timeout;

		// 初始化互斥锁
		AUTO_LOCK_INIT(&g_conn_pool->lock);
		g_conn_pool->connections = (PooledConnection*)calloc(pool_size, sizeof(PooledConnection));
		if (!g_conn_pool->connections) {
			free(g_conn_pool);
			g_conn_pool = NULL;
			return 0;
		}

		// 设置连接参数                                        
		g_conn_pool->host = strdup(host);
		g_conn_pool->user = strdup(user);
		g_conn_pool->passwd = strdup(passwd);
		g_conn_pool->db = strdup(db);
		g_conn_pool->port = port;

		// 初始化连接
		for (int i = 0; i < pool_size; i++) {
			MYSQL* conn = mysql_init(NULL);
			if (!conn) continue;

			// 从配置获取连接参数（这里使用伪代码，实际应从配置文件读取）
			if (mysql_real_connect(conn, host, user, passwd, db, port, NULL, 0)) {
				g_conn_pool->connections[i].conn = conn;
				g_conn_pool->connections[i].in_use = 0;
				g_conn_pool->connections[i].last_used = time(NULL);
			}
			else {
				mysql_close(conn);
			}
		}
	}
	return 1;
}

// 从连接池获取连接
MYSQL* get_connection_from_pool() {
	if (!g_conn_pool) return NULL;

	AUTO_LOCK(&g_conn_pool->lock);

	time_t now = time(NULL);
	// 1. 查找空闲连接
	for (int i = 0; i < g_conn_pool->size; i++) {
		if (!g_conn_pool->connections[i].in_use) {
			// 检查连接是否超时失效
			if (now - g_conn_pool->connections[i].last_used > g_conn_pool->timeout) {
				mysql_ping(g_conn_pool->connections[i].conn); // 保活
			}

			g_conn_pool->connections[i].in_use = 1;
			g_conn_pool->connections[i].last_used = now;

			AUTO_UNLOCK(&g_conn_pool->lock);
			return g_conn_pool->connections[i].conn;
		}
	}

	// 2. 连接池已满且未达上限，创建新连接
	if (g_conn_pool->size < g_conn_pool->max_size) {
		MYSQL* conn = mysql_init(NULL);
		if (conn) {
			if (mysql_real_connect(conn, g_conn_pool->host, g_conn_pool->user, g_conn_pool->passwd, g_conn_pool->db, g_conn_pool->port, NULL, 0)) {
				// 扩展连接池
				PooledConnection* new_conns = realloc(g_conn_pool->connections,
					(g_conn_pool->size + 1) * sizeof(PooledConnection));
				if (new_conns) {
					g_conn_pool->connections = new_conns;
					g_conn_pool->connections[g_conn_pool->size].conn = conn;
					g_conn_pool->connections[g_conn_pool->size].in_use = 1;
					g_conn_pool->connections[g_conn_pool->size].last_used = now;
					g_conn_pool->size++;

					AUTO_UNLOCK(&g_conn_pool->lock);
					return conn;
				}
				else {
					mysql_close(conn);
				}
			}
			else {
				mysql_close(conn);
			}
		}
	}
	AUTO_UNLOCK(&g_conn_pool->lock);
	return NULL; // 无法获取连接
}

// 释放连接到池
void release_connection_to_pool(MYSQL* conn) {
	if (!g_conn_pool || !conn) return;

	AUTO_LOCK(&g_conn_pool->lock);
	for (int i = 0; i < g_conn_pool->size; i++) {
		if (g_conn_pool->connections[i].conn == conn) {
			g_conn_pool->connections[i].in_use = 0;
			g_conn_pool->connections[i].last_used = time(NULL);
			break;
		}
	}
	AUTO_UNLOCK(&g_conn_pool->lock);
}

// 关闭连接池
void destoty_connection_pool() {
	if (!g_conn_pool) return;

	AUTO_LOCK(&g_conn_pool->lock);

	for (int i = 0; i < g_conn_pool->size; i++) {
		if (g_conn_pool->connections[i].conn) {
			mysql_close(g_conn_pool->connections[i].conn);
		}
	}
	//释放连接参数
	free(g_conn_pool->host);
	free(g_conn_pool->user);
	free(g_conn_pool->passwd);
	free(g_conn_pool->db);

	free(g_conn_pool->connections);
	AUTO_UNLOCK(&g_conn_pool->lock);
	free(g_conn_pool);
	g_conn_pool = NULL;
}
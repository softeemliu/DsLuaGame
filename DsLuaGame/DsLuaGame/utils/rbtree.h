#ifndef __RB_TREE_H__
#define __RB_TREE_H__
#include <stdio.h>
#include <stdlib.h>

// 定义颜色枚举 
typedef enum { RED, BLACK } Color;

// 定义树节点结构体 
typedef struct TreeNode {
	int key;
	void* value;
	Color color;
	struct TreeNode* left;
	struct TreeNode* right;
	struct TreeNode* parent;
} TreeNode;

// 定义映射结构体 
typedef struct {
	TreeNode* root;
	int(*compare)(const int, const int);
	void(*free_value)(void*);  // 新增：用于释放值的回调
} CMap;

// 定义迭代器结构
typedef struct {
	TreeNode* current;
} CMapIterator;


// 创建新节点 
TreeNode* create_node(int key, void* value);
// 左旋操作 
void left_rotate(CMap* map, TreeNode* x);
// 右旋操作（代码类似左旋，方向相反）
void right_rotate(CMap* map, TreeNode* y);
// 插入修复红黑树性质 
void fix_insert(CMap* map, TreeNode* z);
// 插入操作 
void map_insert(CMap* map, int key, void* value);
// 修复删除后的红黑树性质 
void fix_delete(CMap* map, TreeNode* x);
// 删除节点 
void map_delete(CMap* map, int key);
// 查找操作 
TreeNode* map_find(CMap* map, int key);
// 用另一个节点替换一个节点 
void transplant(CMap* map, TreeNode* u, TreeNode* v);
// 示例使用 
int compare_int(const int a, const int b);
// 递归中序遍历辅助函数
void in_order_recursive(TreeNode* node, void(*visit)(TreeNode*));
// 遍历整个红黑树（中序遍历）
void map_traverse(CMap* map, void(*visit)(TreeNode*));
// 递归销毁树节点
void destroy_tree_recursive(TreeNode* node, void(*free_value)(void*));
// 销毁整个映射
void map_destroy(CMap* map);

// 遍历操作函数声明
CMapIterator map_iterator_begin(CMap* map);
void map_iterator_next(CMapIterator* it);
TreeNode* map_iterator_current(CMapIterator* it);
int map_iterator_done(CMapIterator* it);


#endif  /*__RB_TREE_H__*/
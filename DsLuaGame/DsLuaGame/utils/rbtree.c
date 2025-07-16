#include "rbtree.h"

// 创建新节点 
TreeNode* create_node(int key, void* value) {
	TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
	node->key = key;
	node->value = value;
	node->color = RED; // 新节点默认红色 
	node->left = node->right = node->parent = NULL;
	return node;
}

// 左旋操作 
void left_rotate(CMap* map, TreeNode* x) {
	TreeNode* y = x->right;
	x->right = y->left;
	if (y->left != NULL)
		y->left->parent = x;
	y->parent = x->parent;

	if (x->parent == NULL)
		map->root = y;
	else if (x == x->parent->left)
		x->parent->left = y;
	else
		x->parent->right = y;

	y->left = x;
	x->parent = y;
}

// 右旋操作（代码类似左旋，方向相反） 
void right_rotate(CMap* map, TreeNode* y) {
	TreeNode* x = y->left;                      // 1. 获取左子节点 
	y->left = x->right; 						// 2. 将y的右子树转移给x 
	if (x->right != NULL) { 						// 3. 更新转移子树的父指针 
		x->right->parent = y;
	}
	x->parent = y->parent; 						// 4. 继承x的父节点 
	if (y->parent == NULL) { 					// 5. 更新父节点的子指针根节点情况 
		map->root = x;
	}
	else if (y == y->parent->right) { 			// 原为左子节点 
		y->parent->right = x;
	}
	else {
		y->parent->left = x; 					// 原为右子节点 
	}
	x->right = y; 								// 6. 将x设为y的右子树 
	y->parent = x; 								// 7. 更新x的父指针  
}

// 插入修复红黑树性质
void fix_insert(CMap* map, TreeNode* z) {
	while (z->parent && z->parent->color == RED) {
		if (z->parent == z->parent->parent->left) { // 父节点是左子节点 
			TreeNode* y = z->parent->parent->right;
			if (y && y->color == RED) { // Case 1 
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->right) { // Case 2 
					z = z->parent;
					left_rotate(map, z);
				}
				// Case 3 
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				right_rotate(map, z->parent->parent);
			}
		}
		else { // 父节点是右子节点（补全对称处理） 
			TreeNode* y = z->parent->parent->left;
			if (y && y->color == RED) { // Case 1镜像 
				z->parent->color = BLACK;
				y->color = BLACK;
				z->parent->parent->color = RED;
				z = z->parent->parent;
			}
			else {
				if (z == z->parent->left) { // Case 2镜像 
					z = z->parent;
					right_rotate(map, z);
				}
				// Case 3镜像 
				z->parent->color = BLACK;
				z->parent->parent->color = RED;
				left_rotate(map, z->parent->parent);
			}
		}
	}
	map->root->color = BLACK; // 确保根节点为黑色 
}

// 插入操作 
void map_insert(CMap* map, int key, void* value) {
	TreeNode** curr = &map->root;
	TreeNode* parent = NULL;

	// 查找插入位置 
	while (*curr) {
		parent = *curr;
		int cmp = map->compare(key, (*curr)->key);
		if (cmp < 0) {
			curr = &(*curr)->left;
		}
		else if (cmp > 0) {
			curr = &(*curr)->right;
		}
		else {
			// 键已存在时更新值并退出，避免死循环 
			(*curr)->value = value;
			return;
		}
	}

	// 创建新节点 
	TreeNode* new_node = create_node(key, value);
	new_node->parent = parent;
	*curr = new_node;

	// 修复红黑树性质 
	fix_insert(map, new_node);
}

// 查找操作 
TreeNode* map_find(CMap* map, int key) {
	TreeNode* curr = map->root;
	while (curr) {
		int cmp = map->compare(key, curr->key);
		if (cmp == 0) return curr;
		curr = cmp < 0 ? curr->left : curr->right;
	}
	return NULL;
}

// 用另一个节点替换一个节点 
void transplant(CMap* map, TreeNode* u, TreeNode* v) {
	if (u->parent == NULL) {
		map->root = v;
	}
	else if (u == u->parent->left) {
		u->parent->left = v;
	}
	else {
		u->parent->right = v;
	}
	// 确保 v 不为 NULL 再设置父指针
	if (v != NULL) {
		v->parent = u->parent;
	}
}

// 修复删除后的红黑树性质 
void fix_delete(CMap* map, TreeNode* x) {
	while (x != map->root && (x == NULL || x->color == BLACK)) {
		// 关键修改 6：添加 NULL 检查
		if (x == NULL) {
			break;
		}

		// 关键修改 7：增强指针安全检查
		if (x->parent == NULL) {
			break;
		}

		if (x == x->parent->left) {
			TreeNode* w = x->parent->right;
			// 关键修改 8：检查兄弟节点是否存在
			if (w == NULL) {
				break;
			}

			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				left_rotate(map, x->parent);
				w = x->parent->right;
				if (w == NULL) break; // 再次检查
			}

			if ((w->left == NULL || w->left->color == BLACK) &&
				(w->right == NULL || w->right->color == BLACK)) {
				w->color = RED;
				x = x->parent;
			}
			else {
				if (w->right == NULL || w->right->color == BLACK) {
					if (w->left != NULL) w->left->color = BLACK;
					w->color = RED;
					right_rotate(map, w);
					w = x->parent->right;
					if (w == NULL) break;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				if (w->right != NULL) w->right->color = BLACK;
				left_rotate(map, x->parent);
				x = map->root;
			}
		}
		else {
			// 对称处理右子树（添加相同的安全检查）
			TreeNode* w = x->parent->left;
			if (w == NULL) {
				break;
			}

			if (w->color == RED) {
				w->color = BLACK;
				x->parent->color = RED;
				right_rotate(map, x->parent);
				w = x->parent->left;
				if (w == NULL) break;
			}

			if ((w->right == NULL || w->right->color == BLACK) &&
				(w->left == NULL || w->left->color == BLACK)) {
				w->color = RED;
				x = x->parent;
			}
			else {
				if (w->left == NULL || w->left->color == BLACK) {
					if (w->right != NULL) w->right->color = BLACK;
					w->color = RED;
					left_rotate(map, w);
					w = x->parent->left;
					if (w == NULL) break;
				}
				w->color = x->parent->color;
				x->parent->color = BLACK;
				if (w->left != NULL) w->left->color = BLACK;
				right_rotate(map, x->parent);
				x = map->root;
			}
		}
	}

	// 关键修改 9：安全设置颜色
	if (x != NULL) {
		x->color = BLACK;
	}
}

// 删除节点 
void map_delete(CMap* map, int key) {
	TreeNode* z = map->root;
	TreeNode* y = NULL;
	TreeNode* x = NULL;
	Color y_original_color;
	// 查找要删除的节点
	while (z != NULL && z->key != key) {
		if (map->compare(key, z->key) < 0) {
			z = z->left;
		}
		else {
			z = z->right;
		}
	}

	if (z == NULL) return;

	y = z;
	y_original_color = y->color;
	if (z->left == NULL) {
		x = z->right;
		transplant(map, z, z->right);
	}
	else if (z->right == NULL) {
		x = z->left;
		transplant(map, z, z->left);
	}
	else {
		y = z->right;
		while (y->left != NULL) {
			y = y->left;
		}
		y_original_color = y->color;
		x = y->right;

		// 关键修改 1：正确处理后继节点指针
		if (y->parent == z) {
			// 如果 y 是 z 的直接右子节点
			if (x != NULL) {
				x->parent = y;
			}
		}
		else {
			transplant(map, y, y->right);
			y->right = z->right;
			// 关键修改 2：添加 NULL 检查
			if (y->right != NULL) {
				y->right->parent = y;
			}
		}

		transplant(map, z, y);
		y->left = z->left;
		// 关键修改 3：添加 NULL 检查
		if (y->left != NULL) {
			y->left->parent = y;
		}
		y->color = z->color;
	}

	// 关键修改 4：安全调用修复函数
	if (y_original_color == BLACK) {
		fix_delete(map, x);
	}

	// 关键修改 5：安全释放节点
	z->left = z->right = z->parent = NULL; // 断开所有指针连接
	free(z);
}

// 示例使用 
int compare_int(const int a, const int b) {
	return a - b;
}

// 递归中序遍历辅助函数
void in_order_recursive(TreeNode* node, void(*visit)(TreeNode*)) {
	if (node == NULL) return;
	in_order_recursive(node->left, visit);
	visit(node); // 处理当前节点
	in_order_recursive(node->right, visit);
}

// 遍历整个红黑树（中序遍历）
void map_traverse(CMap* map, void(*visit)(TreeNode*)) {
	if (map == NULL || map->root == NULL) return;
	in_order_recursive(map->root, visit);
}

// 递归销毁树节点
void destroy_tree_recursive(TreeNode* node, void(*free_value)(void*)) {
	if (node == NULL) return;
	destroy_tree_recursive(node->left, free_value);
	destroy_tree_recursive(node->right, free_value);
	if (free_value != NULL) {
		free_value(node->value); // 释放节点存储的值
	}
	free(node); // 释放节点本身
}

// 销毁整个映射
void map_destroy(CMap* map) {
	if (map == NULL) return;
	destroy_tree_recursive(map->root, map->free_value);
	map->root = NULL; // 避免悬空指针
}

// 初始化迭代器（指向第一个节点）
CMapIterator map_iterator_begin(CMap* map) {
	CMapIterator it;
	it.current = map->root;

	// 找到最左边的节点（最小节点）
	if (it.current != NULL) {
		while (it.current->left != NULL) {
			it.current = it.current->left;
		}
	}
	return it;
}

// 移动到下一个节点
void map_iterator_next(CMapIterator* it) {
	if (it->current == NULL) return;

	// 1. 如果有右子树：移动到右子树的最左节点
	if (it->current->right != NULL) {
		it->current = it->current->right;
		while (it->current->left != NULL) {
			it->current = it->current->left;
		}
		return;
	}

	// 2. 没有右子树：向上回溯
	TreeNode* p = it->current;
	while (p->parent != NULL && p == p->parent->right) {
		p = p->parent;
	}
	it->current = p->parent;
}

// 获取当前节点
TreeNode* map_iterator_current(CMapIterator* it) {
	return it->current;
}

// 检查迭代是否完成
int map_iterator_done(CMapIterator* it) {
	return it->current == NULL;
}
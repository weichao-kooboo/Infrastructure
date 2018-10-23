#ifndef _HKY_RBTREE_H_INCLUDE_
#define _HKY_RBTREE_H_INCLUDE_

#include "Config.h"

typedef hky_uint_t  hky_rbtree_key_t;
typedef hky_int_t   hky_rbtree_key_int_t;


typedef struct hky_rbtree_node_s  hky_rbtree_node_t;

struct hky_rbtree_node_s {
	hky_rbtree_key_t       key;
	hky_rbtree_node_t     *left;
	hky_rbtree_node_t     *right;
	hky_rbtree_node_t     *parent;
	u_char                 color;
	u_char                 data;
};


typedef struct hky_rbtree_s  hky_rbtree_t;

typedef void(*hky_rbtree_insert_pt) (hky_rbtree_node_t *root,
	hky_rbtree_node_t *node, hky_rbtree_node_t *sentinel);

struct hky_rbtree_s {
	hky_rbtree_node_t     *root;
	hky_rbtree_node_t     *sentinel;
	hky_rbtree_insert_pt   insert;
};


#define hky_rbtree_init(tree, s, i)                                           \
    hky_rbtree_sentinel_init(s);                                              \
    (tree)->root = s;                                                         \
    (tree)->sentinel = s;                                                     \
    (tree)->insert = i


void hky_rbtree_insert(hky_rbtree_t *tree, hky_rbtree_node_t *node);
void hky_rbtree_delete(hky_rbtree_t *tree, hky_rbtree_node_t *node);
void hky_rbtree_insert_value(hky_rbtree_node_t *root, hky_rbtree_node_t *node,
	hky_rbtree_node_t *sentinel);
void hky_rbtree_insert_timer_value(hky_rbtree_node_t *root,
	hky_rbtree_node_t *node, hky_rbtree_node_t *sentinel);
hky_rbtree_node_t *hky_rbtree_next(hky_rbtree_t *tree,
	hky_rbtree_node_t *node);


#define hky_rbt_red(node)               ((node)->color = 1)
#define hky_rbt_black(node)             ((node)->color = 0)
#define hky_rbt_is_red(node)            ((node)->color)
#define hky_rbt_is_black(node)          (!hky_rbt_is_red(node))
#define hky_rbt_copy_color(n1, n2)      (n1->color = n2->color)


/* a sentinel must be black */

#define hky_rbtree_sentinel_init(node)  hky_rbt_black(node)


static hky_inline hky_rbtree_node_t *
hky_rbtree_min(hky_rbtree_node_t *node, hky_rbtree_node_t *sentinel)
{
	while (node->left != sentinel) {
		node = node->left;
	}

	return node;
}


#endif
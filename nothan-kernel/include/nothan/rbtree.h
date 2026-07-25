#ifndef _NOTHAN_RBTREE_H
#define _NOTHAN_RBTREE_H

/*
 * include/nothan/rbtree.h - Red-black tree (balanced BST)
 *
 * O(log n) insert / erase / find-min. Written from the CLRS algorithm
 * (textbook red-black tree), NOT ported from any upstream kernel.
 *
 * Generic: ordering is supplied by the caller via a compare callback at
 * insert time. Embed a `struct rbnode` in your object; recover the object
 * from a node with rb_entry().
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 */

#ifndef NULL
#define NULL ((void *)0)
#endif

struct rbnode {
	struct rbnode *parent;
	struct rbnode *left;
	struct rbnode *right;
	int            red;	/* 1 = red, 0 = black */
};

struct rbtree {
	struct rbnode *root;
};

/* Order two nodes: <0 if @a comes before @b, >0 if after, 0 if equal. */
typedef int (*rb_cmp_t)(const struct rbnode *a, const struct rbnode *b);

static inline void rb_init(struct rbtree *t)          { t->root = NULL; }
static inline int  rb_empty(const struct rbtree *t)   { return t->root == NULL; }

void rb_insert(struct rbtree *t, struct rbnode *node, rb_cmp_t cmp);
void rb_erase(struct rbtree *t, struct rbnode *node);

struct rbnode *rb_first(const struct rbtree *t);	/* minimum (leftmost), or NULL */
struct rbnode *rb_next(const struct rbnode *node);	/* in-order successor, or NULL */

#define rb_entry(ptr, type, member) \
	((type *)((char *)(ptr) - __builtin_offsetof(type, member)))

#endif /* _NOTHAN_RBTREE_H */

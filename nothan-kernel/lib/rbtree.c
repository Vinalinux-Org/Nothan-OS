/*
 * lib/rbtree.c - Red-black tree implementation (CLRS algorithm)
 *
 * Written by Doan Phu Hai <haidoan2098@gmail.com>
 *
 * Invariants maintained:
 *   1. Every node is red or black.
 *   2. The root is black.
 *   3. A red node has no red child (no two reds in a row).
 *   4. Every root-to-NULL path has the same number of black nodes.
 * These give height <= 2*log2(n+1), so all operations are O(log n).
 */

#include <nothan/rbtree.h>

/* --- rotations --- */

static void rotate_left(struct rbtree *t, struct rbnode *x)
{
	struct rbnode *y = x->right;

	x->right = y->left;
	if (y->left)
		y->left->parent = x;
	y->parent = x->parent;
	if (!x->parent)
		t->root = y;
	else if (x == x->parent->left)
		x->parent->left = y;
	else
		x->parent->right = y;
	y->left = x;
	x->parent = y;
}

static void rotate_right(struct rbtree *t, struct rbnode *x)
{
	struct rbnode *y = x->left;

	x->left = y->right;
	if (y->right)
		y->right->parent = x;
	y->parent = x->parent;
	if (!x->parent)
		t->root = y;
	else if (x == x->parent->right)
		x->parent->right = y;
	else
		x->parent->left = y;
	y->right = x;
	x->parent = y;
}

/* --- insert --- */

static void insert_fixup(struct rbtree *t, struct rbnode *z)
{
	while (z->parent && z->parent->red) {
		struct rbnode *gp = z->parent->parent;	/* grandparent (non-NULL: root is black) */

		if (z->parent == gp->left) {
			struct rbnode *uncle = gp->right;

			if (uncle && uncle->red) {		/* case 1: recolor, climb */
				z->parent->red = 0;
				uncle->red = 0;
				gp->red = 1;
				z = gp;
			} else {
				if (z == z->parent->right) {	/* case 2 -> case 3 */
					z = z->parent;
					rotate_left(t, z);
				}
				z->parent->red = 0;		/* case 3 */
				z->parent->parent->red = 1;
				rotate_right(t, z->parent->parent);
			}
		} else {					/* mirror of the above */
			struct rbnode *uncle = gp->left;

			if (uncle && uncle->red) {
				z->parent->red = 0;
				uncle->red = 0;
				gp->red = 1;
				z = gp;
			} else {
				if (z == z->parent->left) {
					z = z->parent;
					rotate_right(t, z);
				}
				z->parent->red = 0;
				z->parent->parent->red = 1;
				rotate_left(t, z->parent->parent);
			}
		}
	}
	t->root->red = 0;
}

void rb_insert(struct rbtree *t, struct rbnode *node, rb_cmp_t cmp)
{
	struct rbnode *parent = NULL;
	struct rbnode *cur = t->root;
	int left = 0;

	while (cur) {				/* BST descent to a leaf slot */
		parent = cur;
		left = cmp(node, cur) < 0;
		cur = left ? cur->left : cur->right;
	}

	node->parent = parent;
	node->left = NULL;
	node->right = NULL;
	node->red = 1;				/* new nodes start red */

	if (!parent)
		t->root = node;
	else if (left)
		parent->left = node;
	else
		parent->right = node;

	insert_fixup(t, node);
}

/* --- erase --- */

static struct rbnode *subtree_min(struct rbnode *x)
{
	while (x->left)
		x = x->left;
	return x;
}

/* Replace subtree rooted at @u with subtree rooted at @v (v may be NULL). */
static void transplant(struct rbtree *t, struct rbnode *u, struct rbnode *v)
{
	if (!u->parent)
		t->root = v;
	else if (u == u->parent->left)
		u->parent->left = v;
	else
		u->parent->right = v;
	if (v)
		v->parent = u->parent;
}

/*
 * Restore invariants after removing a black node. @x is the node that took the
 * removed node's place (may be NULL); @parent is x's parent (passed explicitly
 * because x can be NULL). The sibling on x's side is always non-NULL here.
 */
static void erase_fixup(struct rbtree *t, struct rbnode *x, struct rbnode *parent)
{
	while (x != t->root && (!x || !x->red)) {
		if (x == parent->left) {
			struct rbnode *w = parent->right;	/* sibling */

			if (w->red) {				/* case 1 */
				w->red = 0;
				parent->red = 1;
				rotate_left(t, parent);
				w = parent->right;
			}
			if ((!w->left || !w->left->red) &&
			    (!w->right || !w->right->red)) {	/* case 2 */
				w->red = 1;
				x = parent;
				parent = x->parent;
			} else {
				if (!w->right || !w->right->red) {	/* case 3 */
					if (w->left)
						w->left->red = 0;
					w->red = 1;
					rotate_right(t, w);
					w = parent->right;
				}
				w->red = parent->red;		/* case 4 */
				parent->red = 0;
				if (w->right)
					w->right->red = 0;
				rotate_left(t, parent);
				x = t->root;			/* done */
				parent = NULL;
			}
		} else {					/* mirror */
			struct rbnode *w = parent->left;

			if (w->red) {
				w->red = 0;
				parent->red = 1;
				rotate_right(t, parent);
				w = parent->left;
			}
			if ((!w->right || !w->right->red) &&
			    (!w->left || !w->left->red)) {
				w->red = 1;
				x = parent;
				parent = x->parent;
			} else {
				if (!w->left || !w->left->red) {
					if (w->right)
						w->right->red = 0;
					w->red = 1;
					rotate_left(t, w);
					w = parent->left;
				}
				w->red = parent->red;
				parent->red = 0;
				if (w->left)
					w->left->red = 0;
				rotate_right(t, parent);
				x = t->root;
				parent = NULL;
			}
		}
	}
	if (x)
		x->red = 0;
}

void rb_erase(struct rbtree *t, struct rbnode *z)
{
	struct rbnode *y = z;		/* node actually removed from its place */
	struct rbnode *x;		/* node that replaces y (may be NULL) */
	struct rbnode *x_parent;
	int y_black = !y->red;

	if (!z->left) {
		x = z->right;
		x_parent = z->parent;
		transplant(t, z, z->right);
	} else if (!z->right) {
		x = z->left;
		x_parent = z->parent;
		transplant(t, z, z->left);
	} else {
		y = subtree_min(z->right);	/* successor takes z's place */
		y_black = !y->red;
		x = y->right;
		if (y->parent == z) {
			x_parent = y;		/* x may be NULL; its parent is y */
		} else {
			x_parent = y->parent;
			transplant(t, y, y->right);
			y->right = z->right;
			y->right->parent = y;
		}
		transplant(t, z, y);
		y->left = z->left;
		y->left->parent = y;
		y->red = z->red;
	}

	if (y_black)
		erase_fixup(t, x, x_parent);
}

/* --- traversal --- */

struct rbnode *rb_first(const struct rbtree *t)
{
	struct rbnode *n = t->root;

	if (!n)
		return NULL;
	while (n->left)
		n = n->left;
	return n;
}

struct rbnode *rb_next(const struct rbnode *node)
{
	struct rbnode *n = (struct rbnode *)node;

	if (n->right) {			/* successor is min of right subtree */
		n = n->right;
		while (n->left)
			n = n->left;
		return n;
	}
	/* else climb until we go up a left edge */
	while (n->parent && n == n->parent->right)
		n = n->parent;
	return n->parent;
}

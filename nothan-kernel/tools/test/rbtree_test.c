/*
 * tools/test/rbtree_test.c - Host-side red-black tree verification.
 *
 * Compiles with the HOST gcc (not the cross toolchain) and links lib/rbtree.c
 * directly, so the pure-logic tree is verified on a PC before it ever touches
 * the kernel (no board / no JTAG needed for this part).
 *
 * Build & run:
 *   cc -I include -o /tmp/rbtree_test tools/test/rbtree_test.c lib/rbtree.c && /tmp/rbtree_test
 *
 * After EVERY insert and erase it checks the full red-black invariant set,
 * BST ordering via rb_first/rb_next, parent-pointer consistency, and count.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nothan/rbtree.h>

struct item {
	int key;
	int in_tree;
	struct rbnode node;
};

static int cmp(const struct rbnode *a, const struct rbnode *b)
{
	int ka = rb_entry(a, struct item, node)->key;
	int kb = rb_entry(b, struct item, node)->key;
	return (ka > kb) - (ka < kb);
}

/* --- validator: returns black-height, or -1 on any invariant violation --- */
static int fails;
static void fail(const char *msg) { printf("  FAIL: %s\n", msg); fails++; }

static int check(struct rbnode *n, struct rbnode *parent)
{
	if (!n)
		return 1;			/* NULL leaf counts as one black */

	if (n->parent != parent)
		fail("parent pointer mismatch");
	if (n->red && parent && parent->red)
		fail("red node has red parent");

	int lh = check(n->left, n);
	int rh = check(n->right, n);
	if (lh != rh)
		fail("unequal black-height");

	return lh + (n->red ? 0 : 1);
}

static void validate(struct rbtree *t, int expect_count)
{
	if (t->root && t->root->red)
		fail("root is red");
	if (t->root && t->root->parent)
		fail("root has a parent");

	check(t->root, NULL);

	/* in-order walk must be strictly ascending and visit every node once */
	int count = 0, prev = -2000000000, first = 1;
	for (struct rbnode *n = rb_first(t); n; n = rb_next(n)) {
		int k = rb_entry(n, struct item, node)->key;
		if (!first && k <= prev)
			fail("in-order not strictly ascending");
		prev = k; first = 0; count++;
	}
	if (count != expect_count)
		fail("node count mismatch");
}

int main(void)
{
	enum { N = 400, ROUNDS = 30000 };
	struct item *items = calloc(N, sizeof(*items));
	struct rbtree t;
	rb_init(&t);

	unsigned seed = 12345;			/* deterministic */
	srand(seed);

	for (int i = 0; i < N; i++)
		items[i].key = i;		/* distinct keys 0..N-1 */

	int live = 0;
	validate(&t, 0);

	for (int r = 0; r < ROUNDS && !fails; r++) {
		int i = rand() % N;
		if (!items[i].in_tree) {
			rb_insert(&t, &items[i].node, cmp);
			items[i].in_tree = 1;
			live++;
		} else {
			rb_erase(&t, &items[i].node);
			items[i].in_tree = 0;
			live--;
		}
		validate(&t, live);
	}

	/* drain the tree */
	for (int i = 0; i < N && !fails; i++) {
		if (items[i].in_tree) {
			rb_erase(&t, &items[i].node);
			items[i].in_tree = 0;
			live--;
			validate(&t, live);
		}
	}
	if (!rb_empty(&t))
		fail("tree not empty after draining");

	free(items);
	if (fails) {
		printf("RBTREE TEST: %d FAILURES\n", fails);
		return 1;
	}
	printf("RBTREE TEST: PASS  (%d ops, invariants held every step)\n", ROUNDS);
	return 0;
}


#include "sync/tree.h"

#include "lib/test.h"
#include "lib/util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
	kFileCount = 200
};

static UInt32 Random(UInt32 state)
{
	/* Taken from Wikipedia's LGC
	   https://en.wikipedia.org/wiki/Linear_congruential_generator
	   These are the constants used from Numerical Recipes, according to that
	   article. */
	return 1664525 * state + 1013904223;
}

/* Return a random permutation of 1..count. */
static UInt32 *Shuffle(UInt32 seed, int count)
{
	UInt32 *arr, div;
	int i, j;

	arr = malloc(sizeof(*arr) * count);
	if (arr == 0) {
		Fatalf("out of memory");
	}
	arr[0] = 1;
	for (i = 1; i < count; i++) {
		div = (0xffffffff - i) / (i + 1) + 1;
		do {
			j = seed / div;
			seed = Random(seed);
		} while (j >= i);
		arr[i] = arr[j];
		arr[j] = i + 1;
	}
	return arr;
}

static struct FileNode *GetNode(struct FileTree *tree, FileRef ref)
{
	if (ref <= 0 || tree->count < ref) {
		fprintf(stderr, "Error: invalid ref: %d (size = %ld)\n", ref,
		        tree->count);
		exit(1);
	}
	return *tree->nodes + ref - 1;
}

static void SetKey(FileName *key, UInt32 n)
{
	key->u8[0] = 3;
	key->u8[1] = n >> 16;
	key->u8[2] = n >> 8;
	key->u8[3] = n;
}

static int CheckSubTree(struct FileTree *tree, FileRef root);

/* Check tree invariants, return number of black nodes in path from here to
   leaf. Return -1 if there is an error. */
static int CheckTreeNode(struct FileTree *tree, FileRef ref,
                         struct FileNode *parent, int cidx)
{
	struct FileNode *node;
	int cmp, height[2], i;

	if (ref == 0) {
		return 0;
	}
	node = GetNode(tree, ref);
	if (node->file.type_code != 0) {
		Failf("node visited twice: node=%u", node->file.creator_code);
		return -1;
	}
	node->file.type_code = 1;
	if (parent != NULL) {
		if (parent->color == kNodeRed && node->color == kNodeRed) {
			Failf("red child of red node: parent=%u, child=%u",
			      parent->file.creator_code, node->file.creator_code);
			return -1;
		}
		cmp = memcmp(&node->key, &parent->key, sizeof(node->key));
		if (cmp == 0) {
			Failf("child and parent have same key: parent=%u, child=%u",
			      parent->file.creator_code, node->file.creator_code);
			return -1;
		}
		if ((cmp > 0) != cidx) {
			Failf("bad sort order: parent=%u, child=%u",
			      parent->file.creator_code, node->file.creator_code);
			return -1;
		}
	}
	if (node->directory_root != 0) {
		if (CheckSubTree(tree, node->directory_root)) {
			return -1;
		}
	}
	for (i = 0; i < 2; i++) {
		height[i] = CheckTreeNode(tree, node->children[i], node, i);
		if (height[i] < 0) {
			return -1;
		}
	}
	if (height[0] != height[1]) {
		Failf("mismatched tree length: node=%u, len=%d,%d",
		      node->file.creator_code, height[0], height[1]);
		return -1;
	}
	return height[0] + (node->color == kNodeBlack);
}

static void PrintNode(struct FileTree *tree, FileRef ref, int indent)
{
	struct FileNode *node;
	int i;
	char bl, br;

	node = GetNode(tree, ref);
	if (node->color == kNodeBlack) {
		bl = '[';
		br = ']';
	} else {
		bl = ' ';
		br = ' ';
	}
	if (node->file.type_code > 1) {
		/* This node loops. */
		for (i = 0; i < indent; i++) {
			fputc(' ', stderr);
		}
		fprintf(stderr, "%c%d...%c\n", bl, node->file.creator_code, br);
		return;
	}
	node->file.type_code = 2;
	if (node->children[0] != 0) {
		PrintNode(tree, node->children[0], indent + 4);
	}
	for (i = 0; i < indent; i++) {
		fputc(' ', stderr);
	}
	fprintf(stderr, "%c%d%c\n", bl, node->file.creator_code, br);
	if (node->children[1] != 0) {
		PrintNode(tree, node->children[1], indent + 4);
	}
}

static void PrintTree(struct FileTree *tree, FileRef root)
{
	if (root == 0) {
		fputs("(empty)\n", stderr);
	} else {
		PrintNode(tree, root, 0);
	}
}

static int CheckSubTree(struct FileTree *tree, FileRef root)
{
	int r;
	if (root != 0 && GetNode(tree, root)->color != kNodeBlack) {
		Failf("root is not black");
		r = -1;
	} else {
		r = CheckTreeNode(tree, root, NULL, 0);
	}
	if (r < 0) {
		PrintTree(tree, root);
		return -1;
	}
	return 0;
}

static int CheckTree(struct FileTree *tree)
{
	struct FileNode *nodes;
	int i, n;

	/* Mark all as unvisited. */
	nodes = *tree->nodes;
	for (i = 0, n = tree->count; i < n; i++) {
		nodes[i].file.type_code = 0;
	}
	return CheckSubTree(tree, tree->root);
}

static void TestTree1(struct FileTree *tree, FileRef directory, int count,
                      const UInt32 *files)
{
	int i;
	UInt32 fl;
	FileName key;
	FileRef ref;
	struct FileNode *node;

	/* Insert into the tree in random order. */
	memset(&key, 0, sizeof(key));
	for (i = 0; i < count; i++) {
		fl = files[i];
		SetKey(&key, fl);
		ref = TreeInsert(tree, directory, &key);
		if (ref < 0) {
			Failf("TreeInsert (i=%d): %s", i, ErrorDescriptionOrDie(-ref));
			return;
		}
		if (ref == 0) {
			Failf("ref == 0 (i=%d)", i);
			return;
		}
		node = GetNode(tree, ref);
		node->file.creator_code = fl; /* To test we get same node back. */

		/* Check tree invariants. */
		if (CheckTree(tree)) {
			return;
		}
	}

	/* Test that we can find all nodes. */
	for (i = 0; i < count; i++) {
		fl = files[i];
		SetKey(&key, fl);
		ref = TreeInsert(tree, directory, &key);
		if (ref < 0) {
			Failf("second TreeInsert (i=%d): %s", i,
			      ErrorDescriptionOrDie(-ref));
		}
		node = GetNode(tree, ref);
		if (memcmp(&node->key, &key, sizeof(key)) != 0) {
			Failf("got node with wrong key");
			continue;
		}
		/* Check we don't get a new node. */
		if (node->file.creator_code != fl) {
			Failf(
				"got wrong node back from equery "
				"(i=%d, ref=%d, cc=%u, expect cc=%u)",
				i, ref, node->file.creator_code, fl);
		}
	}
}

static void ClearTree(struct FileTree *tree)
{
	tree->nodes = NULL;
	tree->count = 0;
	tree->alloc = 0;
	tree->root = 0;
}

static void TestRandomTree(UInt32 seed)
{
	UInt32 *files;
	struct FileTree tree;

	files = Shuffle(seed, kFileCount);
	ClearTree(&tree);

	SetTestNamef("Random(%u,root)", seed);
	TestTree1(&tree, 0, kFileCount, files);
	SetTestNamef("Random(%u,subdir)", seed);
	TestTree1(&tree, 1, kFileCount, files);

	DisposeHandle((Handle)tree.nodes);
	free(files);
}

static void TestLinearTree(void)
{
	UInt32 *files;
	struct FileTree tree;
	int i;

	files = malloc(kFileCount * sizeof(*files));
	if (files == NULL) {
		Fatalf("out of memory");
	}

	for (i = 0; i < kFileCount; i++) {
		files[i] = i + 1;
	}
	ClearTree(&tree);
	TestTree1(&tree, 0, kFileCount, files);

	for (i = 0; i < kFileCount; i++) {
		files[i] = kFileCount - i;
	}
	tree.count = 0;
	tree.root = 0;
	TestTree1(&tree, 0, kFileCount, files);

	DisposeHandle((Handle)tree.nodes);
	free(files);
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;
	TestRandomTree(123);
	TestRandomTree(456);
	TestLinearTree();
	return TestsDone();
}

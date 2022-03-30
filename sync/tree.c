#include "sync/tree.h"

#include <stdio.h>

#define GetNode(nodes, ref) ((nodes) + (ref)-1)

/* Compare two filenames. Return <0 if x comes before y, >0 if x comes after y,
   or 0 if x is equal to y. Only compares the raw bytes in the filename. The
   filenames must be zero-padded. */
static int CompareFilename(const FileName *x, const FileName *y)
{
	UInt32 ux, uy;
	int i;

	for (i = 0; i < kFilenameSizeU32; i++) {
		ux = x->u32[i];
		uy = y->u32[i];
		if (ux != uy) {
			return ux < uy ? -1 : 1;
		}
	}
	return 0;
}

enum
{
	/*
	  Maximum height of a file tree.

	  If a red-black tree has N black nodes in every path from root to leaf, and
	  has maximum height (which is 2N), then it contains at least 2^N+N-1 nodes.
	  Therefore, given a maximum height of 32, we can have at least 65551 nodes.
	*/
	kTreeMaxHeight = 32,

	/* Initial number of nodes allocated. */
	kTreeInitialSize = 16
};

/* Allocate a new node in the tree. Retrun a negative error code on failure. */
static FileRef TreeNewNode(struct FileTree *tree)
{
	Handle h;
	Size newalloc;

	if (tree->count >= tree->alloc) {
		if (tree->alloc == 0) {
			h = NewHandle(kTreeInitialSize * sizeof(struct FileNode));
			if (h == NULL) {
				return -kErrorNoMemory;
			}
			tree->nodes = (struct FileNode **)h;
			tree->alloc = kTreeInitialSize;
		} else {
			newalloc = tree->alloc * 2;
			if (!ResizeHandle((Handle)tree->nodes,
			                  newalloc * sizeof(struct FileNode))) {
				return -kErrorNoMemory;
			}
			tree->alloc = newalloc;
		}
	}

	return ++tree->count;
}

/* Initialize a node in a file tree. The key is set, and all other fields are
   zeroed. */
static void TreeInitNode(struct FileNode *node, const FileName *key)
{
	node->key = *key;
	MemClear((char *)node + sizeof(FileName), sizeof(*node) - sizeof(FileName));
}

FileRef TreeInsert(struct FileTree *tree, FileRef directory,
                   const FileName *key)
{
	FileRef path[kTreeMaxHeight], root, cref, pref, sref, gref, nref;
	int depth, cmp, cidx, pidx;
	struct FileNode *nodes, *cnode, *pnode, *gnode, *snode;

	if (directory == 0) {
		root = tree->root;
	} else {
		root = GetNode(*tree->nodes, directory)->directory_root;
	}

	/* Empty tree. */
	if (root == 0) {
		cref = TreeNewNode(tree);
		if (cref <= 0) {
			return cref;
		}
		TreeInitNode(GetNode(*tree->nodes, cref), key);
		if (directory == 0) {
			tree->root = cref;
		} else {
			GetNode(*tree->nodes, directory)->directory_root = cref;
		}
		return cref;
	}

	/* Find the node, or the parent where it will be inserted. */
	nodes = *tree->nodes;
	pref = root;
	for (depth = 0; depth < kTreeMaxHeight; depth++) {
		path[depth] = pref;
		pnode = GetNode(nodes, pref);
		cmp = CompareFilename(key, &pnode->key);
		if (cmp == 0) {
			return pref;
		}
		cidx = cmp < 0 ? 0 : 1;
		cref = pnode->children[cidx];
		if (cref == 0) {
			goto insert;
		}
		pref = cref;
	}

	/* Maximum depth exceeded: too many files in one directory. */
	return -kErrorDirectoryTooLarge;

insert:
	/* Insert a new node into the tree. */
	if (depth + 1 >= kTreeMaxHeight) {
		return -kErrorDirectoryTooLarge;
	}
	cref = TreeNewNode(tree);
	if (cref <= 0) {
		return cref;
	}
	nref = cref;
	nodes = *tree->nodes;
	pnode = GetNode(nodes, pref);
	cnode = GetNode(nodes, cref);
	TreeInitNode(cnode, key);
	cnode->color = kNodeRed;
	pnode->children[cidx] = cref;
	path[depth + 1] = cref;

	if (pnode->color != kNodeBlack) {
		assert(depth > 0);
		depth--;
		/*
		  Loop invariants:
		  path[depth+2] == cref, cnode: child node, red
		  path[depth+1] == pref, pnode: parent node, red
		  cidx: cref is which child of parent
		*/
		for (;;) {
			/*
			  path[depth] == gref, gnode: grandparent node, black
			  pidx: pref is which child of grandparent
			  sref: sibling of parent node
			*/
			gref = path[depth];
			gnode = GetNode(nodes, gref);
			assert(gnode->color == kNodeBlack);
			pidx = gnode->children[0] != pref;
			assert(gnode->children[pidx] == pref);
			sref = gnode->children[pidx ^ 1];
			snode = sref == 0 ? NULL : GetNode(nodes, sref);
			if (sref == 0 || snode->color == kNodeBlack) {
				/* Rotate. */
				if (cidx == pidx) {
					/* Move parent to top. */
					gnode->children[cidx] = pnode->children[cidx ^ 1];
					pnode->children[cidx ^ 1] = gref;
					cnode->color = kNodeBlack;
					cref = pref;
					cnode = pnode;
				} else {
					/* Move child to top. */
					gnode->children[cidx ^ 1] = cnode->children[cidx];
					pnode->children[cidx] = cnode->children[cidx ^ 1];
					cnode->children[cidx] = gref;
					cnode->children[cidx ^ 1] = pref;
					pnode->color = kNodeBlack;
				}
				if (depth == 0) {
					cnode->color = kNodeBlack;
					if (directory == 0) {
						tree->root = cref;
					} else {
						GetNode(nodes, directory)->directory_root = cref;
					}
					break;
				}
				pref = path[depth - 1];
				pnode = GetNode(nodes, pref);
				cidx = pnode->children[0] != gref;
				assert(pnode->children[cidx] == gref);
				pnode->children[cidx] = cref;
				if (depth == 1 || pnode->color == kNodeBlack) {
					break;
				}
			} else {
				/* Recolor. */
				pnode->color = kNodeBlack;
				snode->color = kNodeBlack;
				if (depth <= 1) {
					if (depth == 1) {
						gnode->color = kNodeRed;
					}
					break;
				}
				gnode->color = kNodeRed;
				cref = gref;
				cnode = gnode;
				pref = path[depth - 1];
				pnode = GetNode(nodes, pref);
				if (pnode->color == kNodeBlack) {
					break;
				}
				cidx = pnode->children[0] != cref;
				assert(pnode->children[cidx] == cref);
			}
			depth -= 2;
		}
	}
	return nref;
}

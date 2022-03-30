#ifndef sync_tree_h
#define sync_tree_h
/* tree.h - binary search trees of file metadata */

#include "sync/meta.h"

/* A reference to a file node by 1-based index, or 0 for none. */
typedef int FileRef;

typedef enum
{
	kNodeBlack,
	kNodeRed,
} NodeColor;

/* A file record stored in a binary search tree. */
struct FileNode {
	/* The sort key. This is a case-folded version of the local filename. */
	FileName key;

	struct FileRec file;

	/* The root of the tree for the directory contents, if this is a
	   directory. */
	FileRef directory_root;

	/* Binary search tree bookkeeping. */
	NodeColor color;
	FileRef children[2];
};

/* Binary search tree of files. */
struct FileTree {
	struct FileNode **nodes;
	Size count;
	Size alloc;
	FileRef root;
};

/* Insert a node into the tree with the given key. On success, return a positive
   FileRef. On failure, return a negative error code. To get the error code
   value, negate the function result. */
FileRef TreeInsert(struct FileTree *tree, FileRef directory,
                   const FileName *key);

#endif

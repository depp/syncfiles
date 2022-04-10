// Copyright 2022 Dietrich Epp.
// This file is part of SyncFiles. SyncFiles is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
/* convert_1r.c - Reverse conversion from UTF-8 to extended ASCII. */
#include "convert/convert.h"
#include "lib/defs.h"

enum {
	/* Maximum length of encoded character. */
	kMaxEncodedLength = 8,

	/* Initial number of nodes to allocate when building the tree. */
	kInitialTableAlloc = 8
};

struct TEntry {
	/* The output character, or zero if no output. */
	UInt8 output;
	/* The next node, or zero if no next node. */
	UInt8 next;
};

/* A node for building the converter. */
struct TNode {
	struct TEntry entries[256];
};

struct TTree {
	struct TNode **nodes;
	int count;
};

static ErrorCode CreateTree(struct TTree *tree, Handle data, Size datasz)
{
	struct TNode **nodes, *node;
	int i, j, dpos, enclen, encend, state, cur, nodecount, nodealloc;
	unsigned ch;

	/* Create a tree with a root node mapping all the ASCII characters except
	   NUL, CR, and LF. NUL won't map because an output of 0 is interpreted as
	   no output. CR and LF are removed so they can be handled specially be the
	   decoder. */
	nodes =
		(struct TNode **)NewHandle(kInitialTableAlloc * sizeof(struct TNode));
	if (nodes == NULL) {
		return kErrorNoMemory;
	}
	nodecount = 1;
	nodealloc = kInitialTableAlloc;
	node = *nodes;
	MemClear(node, sizeof(struct TNode));
	for (i = 0; i < 128; i++) {
		node->entries[i].output = i;
	}
	node->entries[kCharLF].output = 0;
	node->entries[kCharCR].output = 0;

	/* Parse the table data and build up a tree of TNode. */
	dpos = 1;
	/* For each high character (128..255). */
	for (i = 0; i < 128; i++) {
		/* For each encoding of that character. */
		for (j = 0; j < 2; j++) {
			if (dpos >= datasz) {
				goto bad_table;
			}
			enclen = (UInt8)(*data)[dpos++];
			if (enclen != 0) {
				if (enclen < 2 || enclen > datasz - dpos ||
				    enclen > kMaxEncodedLength) {
					goto bad_table;
				}
				/* Iterate over all but last byte in encoding, to find the node
				   which will produce the decoded byte as output. */
				state = 0;
				node = *nodes;
				for (encend = dpos + enclen - 1; dpos < encend; dpos++) {
					ch = (UInt8)(*data)[dpos];
					cur = state;
					state = node->entries[ch].next;
					if (state == 0) {
						if (nodecount >= nodealloc) {
							nodealloc *= 2;
							if (!ResizeHandle(
									(Handle)nodes,
									nodealloc * sizeof(struct TNode))) {
								DisposeHandle((Handle)nodes);
								return kErrorNoMemory;
							}
							node = *nodes + cur;
						}
						state = nodecount++;
						node->entries[ch].next = state;
						node = (*nodes) + state;
						MemClear(node, sizeof(*node));
					} else {
						node = *nodes + state;
					}
				}
				ch = (UInt8)(*data)[dpos++];
				if (node->entries[ch].output != 0) {
					goto bad_table;
				}
				node->entries[ch].output = i | 0x80;
			}
		}
	}
	if (!ResizeHandle((Handle)nodes, nodecount * sizeof(struct TNode))) {
		DisposeHandle((Handle)nodes);
		return kErrorNoMemory;
	}
	tree->nodes = nodes;
	tree->count = nodecount;
	return 0;

bad_table:
	DisposeHandle((Handle)nodes);
	return kErrorBadData;
}

struct NodeInfo {
	UInt8 min;
	UInt8 max;
	UInt16 offset;
};

struct CEntry {
	UInt16 output;
	UInt16 next;
};

/* A compressed table node. Followed by an array of centry. */
struct CNode {
	/* First byte in table. */
	UInt8 base;
	/* Number of entries in table, minus one. */
	UInt8 span;
};

static ErrorCode CompactTree(Handle *out, struct TNode **nodes, int nodecount)
{
	Handle ctree;
	struct TNode *node;
	struct NodeInfo **infos, *info;
	struct CNode *cnode;
	struct CEntry *centry;
	int i, j, min, max, count, next;
	unsigned offset;

	/* Figure out where each compacted node will go. */
	infos = (struct NodeInfo **)NewHandle(sizeof(struct NodeInfo) * nodecount);
	if (infos == NULL) {
		return kErrorNoMemory;
	}
	offset = 0;
	for (i = 0; i < nodecount; i++) {
		node = *nodes + i;
		min = 0;
		while (node->entries[min].output == 0 && node->entries[min].next == 0) {
			min++;
		}
		max = 255;
		while (node->entries[max].output == 0 && node->entries[max].next == 0) {
			max--;
		}
		info = *infos + i;
		info->min = min;
		info->max = max;
		info->offset = offset;
		count = max - min + 1;
		offset += sizeof(struct CNode) + count * sizeof(struct CEntry);
	}

	/* Create the compacted tree. */
	ctree = NewHandle(offset);
	if (ctree == NULL) {
		DisposeHandle((Handle)infos);
		return kErrorNoMemory;
	}
	for (i = 0; i < nodecount; i++) {
		node = *nodes + i;
		info = *infos + i;
		min = info->min;
		max = info->max;
		offset = info->offset;
		cnode = (void *)(*ctree + offset);
		cnode->base = min;
		cnode->span = max - min;
		centry = (void *)(*ctree + offset + sizeof(struct CNode));
		for (j = min; j <= max; j++) {
			centry->output = node->entries[j].output;
			next = node->entries[j].next;
			if (next != 0) {
				next = (*infos)[next].offset;
			}
			centry->next = next;
			centry++;
		}
	}

	DisposeHandle((Handle)infos);
	*out = ctree;
	return 0;
}

ErrorCode Convert1rBuild(Handle *out, Handle data, Size datasz)
{
	struct TTree table;
	ErrorCode err;

	err = CreateTree(&table, data, datasz);
	if (err != 0) {
		return err;
	}
	err = CompactTree(out, table.nodes, table.count);
	DisposeHandle((Handle)table.nodes);
	return err;
}

struct Convert1rState {
	UInt8 lastch;
	UInt8 output;
	UInt16 tableoffset;
};

void Convert1rRun(const void *cvtptr, LineBreakConversion lc,
                  struct ConverterState *stateptr, UInt8 **optr, UInt8 *oend,
                  const UInt8 **iptr, const UInt8 *iend)
{
	struct Convert1rState *state = (struct Convert1rState *)stateptr;
	const struct CNode *node;
	const struct CEntry *entry;
	UInt8 *opos = *optr;
	const UInt8 *ipos = *iptr, *savein;
	unsigned ch, lastch, chlen, output, saveout, toffset, savetoffset;

	ch = state->lastch;
	savein = ipos;
	saveout = state->output;
	toffset = state->tableoffset;
	savetoffset = toffset;
	if (oend - opos < 2) {
		goto done;
	}
	goto resume;

next_out:
	if (oend - opos < 2) {
		goto done;
	}

	/* Follow state machine to the end. */
	savein = ipos;
	saveout = 0;
	toffset = 0;
	savetoffset = 0;
resume:
	for (;;) {
		if (ipos >= iend) {
			goto done;
		}
		lastch = ch;
		ch = *ipos++;

		node = (const void *)((const UInt8 *)cvtptr + toffset);
		ch -= node->base;
		if (ch > node->span) {
			toffset = 0;
			goto bad_char;
		}
		entry =
			(const void *)((const UInt8 *)cvtptr + toffset +
		                   sizeof(struct CNode) + ch * sizeof(struct CEntry));
		output = entry->output;
		toffset = entry->next;
		if (toffset == 0) {
			/* Reached end of tree. */
			if (output == 0) {
				goto bad_char;
			}
			*opos++ = output;
			goto next_out;
		}
		if (output != 0) {
			/* Can produce output here, or can consume more input. We try
			    consuming more input, but save the state to rewind if that
			    fails. */
			savein = ipos;
			saveout = output;
			savetoffset = toffset;
		}
	}

bad_char:
	/* Bad character. Back up and try again. */
	ipos = savein;
	if (saveout != 0) {
		/* Produce saved output. */
		*opos++ = saveout;
		ch = 0;
	} else {
		/* No saved output, this really is a bad character. Consume one
		   UTF-8 character, emit it as a fallback, and continue. */
		ch = *ipos++;
		if ((ch & 0x80) == 0) {
			/* ASCII character: either NUL, CR, or LF, because only
			   these
			   characters will result in a transition to state 0. */
			if (ch == 0) {
				*opos++ = ch;
			} else if (ch == kCharLF && lastch == kCharCR) {
				if (lc == kLineBreakKeep) {
					*opos++ = ch;
				}
			} else {
				switch (lc) {
				case kLineBreakKeep:
					*opos++ = ch;
					break;
				case kLineBreakLF:
					*opos++ = kCharLF;
					break;
				case kLineBreakCR:
					*opos++ = kCharCR;
					break;
				case kLineBreakCRLF:
					*opos++ = kCharCR;
					*opos++ = kCharLF;
					break;
				}
			}
		} else {
			if ((ch & 0xe0) == 0xc0) {
				chlen = 1;
			} else if ((ch & 0xf0) == 0xe0) {
				chlen = 2;
			} else if ((ch & 0xf8) == 0xf0) {
				chlen = 3;
			} else {
				chlen = 0;
			}
			for (; chlen > 0; chlen--) {
				if (ipos == iend) {
					goto done;
				}
				ch = *ipos;
				if ((ch & 0xc0) != 0x80) {
					break;
				}
				ipos++;
			}
			*opos++ = kCharSubstitute;
		}
	}
	goto next_out;

done:
	state->lastch = ch;
	state->output = saveout;
	state->tableoffset = savetoffset;
	*optr = opos;
	*iptr = savein;
}

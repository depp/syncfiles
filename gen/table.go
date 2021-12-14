package main

import (
	"bufio"
	"errors"
	"fmt"
)

var (
	errEmptyString = errors.New("empty input")
	errZeroInput   = errors.New("zero byte input")
	errZeroOutput  = errors.New("zero byte output")
)

type inputConflictErr struct {
	input []byte
	out1  byte
	out2  byte
}

func (e *inputConflictErr) Error() string {
	return fmt.Sprintf("table conflict: %d maps to both %d and %d", e.input, e.out1, e.out2)
}

// A node is an element in a Unicode decoding graph.
type node struct {
	chars    [256]uint8
	children [256]*node
}

// add adds the mapping from "in" to "out", creating additional nodes as
// necessary.
func (n *node) add(out byte, in []byte) error {
	if len(in) == 0 {
		return errEmptyString
	}
	if in[0] == 0 {
		if out == 0 {
			return nil
		}
	}
	if out == 0 {
		return errZeroOutput
	}
	for _, b := range in[:len(in)-1] {
		old := n
		n = n.children[b]
		if n == nil {
			n = new(node)
			old.children[b] = n
		}
	}
	b := in[len(in)-1]
	x := n.chars[b]
	if x == 0 {
		n.chars[b] = out
		return nil
	}
	if x == out {
		return nil
	}
	return &inputConflictErr{
		input: in,
		out1:  x,
		out2:  out,
	}
}

func (n *node) size() int {
	sz := 1
	for _, c := range n.children {
		if c != nil {
			sz += c.size()
		}
	}
	return sz
}

func (n *node) writeTable(table decoderTable, pos int) int {
	data := table[pos*256 : pos*256+256 : pos*256+256]
	pos++
	for i, c := range n.chars {
		data[i] = uint16(c)
	}
	for i, c := range n.children {
		if c != nil {
			data[i] |= uint16(pos << 8)
			pos = c.writeTable(table, pos)
		}
	}
	return pos
}

func (n *node) genTable() decoderTable {
	sz := n.size()
	table := make(decoderTable, 256*sz)
	pos := n.writeTable(table, 0)
	if pos != sz {
		panic("bad table")
	}
	return table
}

type decoderTable []uint16

func (t decoderTable) dumpTransitions(w *bufio.Writer) {
	n := len(t) >> 8
	for i := 0; i < n; i++ {
		t := t[i<<8 : (i+1)<<8]
		fmt.Fprintf(w, "State $%02x\n", i)
		for m, v := range t {
			if v != 0 {
				fmt.Fprintf(w, "    $%02x ->", m)
				st := v >> 8
				chr := v & 255
				if st != 0 {
					fmt.Fprintf(w, " state $%02x", st)
				}
				if chr != 0 {
					fmt.Fprintf(w, " char $%02x", chr)
				}
				w.WriteByte('\n')
			}
		}
		w.WriteByte('\n')
	}
}

func (t decoderTable) toBytes() []byte {
	b := make([]byte, len(t)*2)
	for i, x := range t {
		b[i*2] = byte(x >> 8)
		b[i*2+1] = byte(x)
	}
	return b
}

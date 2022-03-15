// Package charmap provides a way to read character maps.
package charmap

import (
	"bufio"
	"bytes"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
	"os"
	"strconv"
	"unicode"
)

// An Error indicates an error parsing a character mapping file.
type Error struct {
	File string
	Line int
	Err  error
}

func (e *Error) Error() (s string) {
	if e.File != "" {
		s += e.File + ":"
	}
	if e.Line != 0 {
		s += strconv.Itoa(e.Line) + ":"
	}
	m := e.Err.Error()
	if s == "" {
		return m
	}
	return s + " " + m
}

var (
	errBadType        = errors.New("unknown entry type")
	errColumns        = errors.New("expected 2 columns")
	errUnicode        = errors.New("invalid unicode sequence")
	errCodePointRange = errors.New("code point out of range")
	errDuplicate      = errors.New("duplicate entry")
)

// A Direction is a direction context for Unicode characters.
type Direction uint32

const (
	// DirectionAny indicates that the character can be omitted in any
	// direction.
	DirectionAny Direction = iota
	// DirectionLR indicates that the character requires left-to-right context.
	DirectionLR
	// DirectionRL indicates that the character requires right-to-left context.
	DirectionRL
)

// An Entry is a single entry in a character map.
type Entry struct {
	Direction Direction
	Unicode   []rune
}

// A Charmap is a character map, mapping characters from a platform encoding to
// Unicode.
type Charmap struct {
	// Pairs of valid one-byte characters that have an alternate Unicode
	// representation.
	Digraph map[[2]byte]Entry

	// Valid single-byte characters.
	OneByte map[byte]Entry

	// Valid two-byte characters.
	TwoByte map[[2]byte]Entry
}

func (m *Charmap) parseLine(line []byte) error {
	// Remove comment.
	if i := bytes.IndexByte(line, '#'); i != -1 {
		line = line[:i]
	}
	if len(line) == 0 {
		return nil
	}

	// Split into two columns.
	i := bytes.IndexByte(line, '\t')
	if i == -1 || i == 0 {
		return errColumns
	}
	c1 := line[:i]
	c2 := line[i+1:]
	if i := bytes.IndexByte(c2, '\t'); i != -1 {
		c2 = c2[:i]
	}
	if len(c2) == 0 {
		return errColumns
	}

	// Parse Unicode sequence and context.
	var e Entry
	if c2[0] == '<' {
		i := bytes.IndexByte(c2, '+')
		if i == -1 {
			return errors.New("invalid Unicode string")
		}
		ctx := c2[:i]
		c2 = c2[i+1:]
		switch string(ctx) {
		case "<LR>":
			e.Direction = DirectionLR
		case "<RL>":
			e.Direction = DirectionRL
		default:
			return fmt.Errorf("unknown context: %q", ctx)
		}
	}
	var ubuf [8]rune
	var ulen int
	for c2 != nil {
		var cp []byte
		if i := bytes.IndexByte(c2, '+'); i != -1 {
			cp = c2[:i]
			c2 = c2[i+1:]
		} else {
			cp = c2
			c2 = nil
		}
		if len(cp) < 2 || string(cp[0:2]) != "0x" {
			return errUnicode
		}
		cp = cp[2:]
		x, err := strconv.ParseUint(string(cp), 16, 32)
		if err != nil {
			return err
		}
		if x > unicode.MaxRune {
			return errCodePointRange
		}
		if ulen >= len(ubuf) {
			return errors.New("Unicode sequence too long")
		}
		ubuf[ulen] = rune(x)
		ulen++
	}
	e.Unicode = make([]rune, ulen)
	copy(e.Unicode, ubuf[:])

	// Parse platform encoded value, store value there.
	switch len(c1) {
	case 4:
		if string(c1[0:2]) != "0x" {
			return errBadType
		}
		var k [1]byte
		if _, err := hex.Decode(k[:], c1[2:]); err != nil {
			return err
		}
		ch := k[0]
		if m.OneByte == nil {
			m.OneByte = make(map[byte]Entry)
		}
		if _, ok := m.OneByte[ch]; ok {
			return errDuplicate
		}
		m.OneByte[ch] = e
	case 6:
		if string(c1[0:2]) != "0x" {
			return errBadType
		}
		var k [2]byte
		if _, err := hex.Decode(k[:], c1[2:]); err != nil {
			return err
		}
		if m.TwoByte == nil {
			m.TwoByte = make(map[[2]byte]Entry)
		}
		if _, ok := m.TwoByte[k]; ok {
			return errDuplicate
		}
		m.TwoByte[k] = e
	case 9:
		if string(c1[0:2]) != "0x" || string(c1[4:7]) != "+0x" {
			return errBadType
		}
		var k [2]byte
		if _, err := hex.Decode(k[0:1], c1[2:4]); err != nil {
			return err
		}
		if _, err := hex.Decode(k[1:2], c1[7:9]); err != nil {
			return err
		}
		if m.Digraph == nil {
			m.Digraph = make(map[[2]byte]Entry)
		}
		if _, ok := m.Digraph[k]; ok {
			return errDuplicate
		}
		m.Digraph[k] = e
	default:
		return errBadType
	}

	return nil
}

// Read reads a charmap from a stream.
func Read(r io.Reader, name string) (*Charmap, error) {
	sc := bufio.NewScanner(r)
	var m Charmap
	for lineno := 1; sc.Scan(); lineno++ {
		if err := m.parseLine(sc.Bytes()); err != nil {
			return nil, &Error{name, lineno, err}
		}
	}
	if err := sc.Err(); err != nil {
		return nil, err
	}
	return &m, nil
}

// ReadFile reads a charmap from a file on disk.
func ReadFile(name string) (*Charmap, error) {
	fp, err := os.Open(name)
	if err != nil {
		return nil, err
	}
	defer fp.Close()
	return Read(fp, name)
}

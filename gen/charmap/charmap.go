// Package charmap provides a way to read character maps.
package charmap

import (
	"bufio"
	"bytes"
	"encoding/hex"
	"errors"
	"fmt"
	"io"
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
)

// An EntryType is a type of entry in a character mapping.
type EntryType uint32

const (
	// EntryOneByte is an entry where a one-byte character is mapped to Unicode.
	EntryOneByte EntryType = iota
	// EntryTwoByte is an entry where a two-byte character is mapped to Unicode.
	EntryTwoByte
	// EntryDigraph is an entry where two one-byte characters are mapped to
	// Unicode.
	EntryDigraph
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
	Type      EntryType
	Encoded   []byte
	Direction Direction
	Unicode   []byte
}

// A Reader is a character map reader.
type Reader struct {
	name    string
	lineno  int
	scanner *bufio.Scanner
	encoded [2]byte
	unicode bytes.Buffer
}

// NewReader creates a character map reader.
func NewReader(r io.Reader, name string) *Reader {
	return &Reader{
		name:    name,
		scanner: bufio.NewScanner(r),
	}
}

func (r *Reader) parseEntry(line []byte) (e Entry, err error) {
	// Split into two columns.
	i := bytes.IndexByte(line, '\t')
	if i == -1 || i == 0 {
		return e, errColumns
	}
	c1 := line[:i]
	c2 := line[i+1:]
	if i := bytes.IndexByte(c2, '\t'); i != -1 {
		c2 = c2[:i]
	}
	if len(c2) == 0 {
		return e, errColumns
	}

	// Parse columns.
	switch len(c1) {
	case 4:
		if string(c1[0:2]) != "0x" {
			return e, errBadType
		}
		if _, err := hex.Decode(r.encoded[:], c1[2:]); err != nil {
			return e, err
		}
		e.Type = EntryOneByte
		e.Encoded = r.encoded[0:1]
	case 6:
		if string(c1[0:2]) != "0x" {
			return e, errBadType
		}
		if _, err := hex.Decode(r.encoded[:], c1[2:]); err != nil {
			return e, err
		}
		e.Type = EntryTwoByte
		e.Encoded = r.encoded[0:2]
	case 9:
		if string(c1[0:2]) != "0x" || string(c1[4:7]) != "+0x" {
			return e, errBadType
		}
		if _, err := hex.Decode(r.encoded[0:1], c1[2:4]); err != nil {
			return e, err
		}
		if _, err := hex.Decode(r.encoded[1:2], c1[7:9]); err != nil {
			return e, err
		}
		e.Type = EntryDigraph
		e.Encoded = r.encoded[0:2]
	default:
		return e, errBadType
	}
	if c2[0] == '<' {
		i := bytes.IndexByte(c2, '+')
		if i == -1 {
			return e, errors.New("invalid Unicode string")
		}
		ctx := c2[:i]
		c2 = c2[i+1:]
		switch string(ctx) {
		case "<LR>":
			e.Direction = DirectionLR
		case "<RL>":
			e.Direction = DirectionRL
		default:
			return e, fmt.Errorf("unknown context: %q", ctx)
		}
	}
	r.unicode.Reset()
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
			return e, errUnicode
		}
		cp = cp[2:]
		x, err := strconv.ParseUint(string(cp), 16, 32)
		if err != nil {
			return e, err
		}
		if x > unicode.MaxRune {
			return e, errCodePointRange
		}
		r.unicode.WriteRune(rune(x))
	}
	e.Unicode = r.unicode.Bytes()
	return e, nil
}

// Next returns the next entry in the character map. Returns io.EOF if there are
// no more entries.
func (r *Reader) Next() (e Entry, err error) {
	for {
		if !r.scanner.Scan() {
			if err := r.scanner.Err(); err != nil {
				return e, err
			}
			return e, io.EOF
		}
		r.lineno++
		line := r.scanner.Bytes()
		// Remove comment.
		if i := bytes.IndexByte(line, '#'); i != -1 {
			line = line[:i]
		}
		if len(line) != 0 {
			e, err = r.parseEntry(line)
			if err != nil {
				err = &Error{r.name, r.lineno, err}
			}
			return
		}
	}
}

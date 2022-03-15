package table

import (
	"bytes"
	"errors"
	"fmt"
	"unicode/utf8"

	"golang.org/x/text/unicode/norm"

	"moria.us/macscript/charmap"
)

// An UnsupportedError indicates that the charmap is not supported by the
// conversion routines.
type UnsupportedError struct {
	Message string
}

func (e *UnsupportedError) Error() string {
	return "unsupported charmap: " + e.Message
}

// Table type identifiers.
const (
	extendedASCIITable = iota + 1
)

type Table interface {
	Data() []byte
}

func Create(m *charmap.Charmap) (Table, error) {
	if m.OneByte == nil {
		return nil, errors.New("missing one-byte map")
	}
	if m.TwoByte != nil {
		return nil, &UnsupportedError{"multibyte encoding"}
	}
	if m.Digraph != nil {
		return nil, &UnsupportedError{"contains digraphs"}
	}
	var t ExtendedASCII
	for c, e := range m.OneByte {
		if e.Direction != charmap.DirectionAny {
			return nil, &UnsupportedError{
				fmt.Sprintf("character has bidirectional context: 0x%02x", c)}
		}
		var u rune
		switch len(e.Unicode) {
		case 0:
		case 1:
			u = e.Unicode[0]
		default:
			return nil, &UnsupportedError{
				fmt.Sprintf("character maps to multiple code points: 0x%02x", c)}
		}
		if c < 128 {
			if u != rune(c) {
				return nil, &UnsupportedError{
					fmt.Sprintf("character is not equal to ASCII equivalent: 0x%02x", c)}
			}
		} else {
			t.HighCharacters[c-128] = u
		}
	}
	return &t, nil
}

// An ExtendedASCII is a table for converting from extended ASCII.
type ExtendedASCII struct {
	HighCharacters [128]rune
}

func (t *ExtendedASCII) Data() []byte {
	var ubuf [4]byte
	var nbuf [16]byte
	d := []byte{extendedASCIITable}
	for _, u := range t.HighCharacters {
		var udata, ndata []byte
		if u != 0 {
			n := utf8.EncodeRune(ubuf[:], u)
			udata = ubuf[:n]
			ndata = norm.NFD.Append(nbuf[:0], udata...)
			if bytes.Equal(udata, ndata) {
				ndata = nil
			}
		}
		d = append(d, byte(len(udata)))
		d = append(d, udata...)
		d = append(d, byte(len(ndata)))
		d = append(d, ndata...)
	}
	return d
}

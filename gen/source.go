package main

import (
	"bufio"
	"fmt"
	"math"
	"os"
	"os/exec"
	"path"
	"strconv"
)

const (
	width     = 80
	formatOff = "/* clang-format off */\n"
	formatOn  = "/* clang-format on */\n"
)

type csource struct {
	filename string
	file     *os.File
	writer   *bufio.Writer
}

func createCSource(filename string) (s csource, err error) {
	if !flagQuiet {
		fmt.Fprintln(os.Stderr, "Writing:", filename)
	}

	fp, err := os.Create(filename)
	if err != nil {
		return s, err
	}
	w := bufio.NewWriter(fp)
	w.WriteString(header)
	return csource{
		filename: filename,
		file:     fp,
		writer:   w,
	}, nil
}

func (s *csource) close() {
	if s.file != nil {
		s.file.Close()
		s.file = nil
	}
	if s.filename != "" {
		os.Remove(s.filename)
	}
}

func (s *csource) flush() error {
	if s.file == nil {
		panic("already closed")
	}
	err := s.writer.Flush()
	s.writer = nil
	if err != nil {
		return err
	}
	err = s.file.Close()
	s.file = nil
	if err != nil {
		return err
	}
	if flagFormat {
		cmd := exec.Command("clang-format", "-i", s.filename)
		if err := cmd.Run(); err != nil {
			return err
		}
	}
	s.filename = ""
	return nil
}

func (s *csource) include(name string) {
	fmt.Fprintf(s.writer, "#include \"%s\"\n", path.Join(srcdirname, name))
}

func (s *csource) bytes(data []byte, final bool) {
	if len(data) == 0 {
		return
	}
	line := make([]byte, 0, width+8)
	for i, x := range data {
		cur := line
		line = strconv.AppendUint(line, uint64(x), 10)
		if i < len(data)-1 || !final {
			line = append(line, ',')
		}
		if len(line) > width-4 {
			s.writer.WriteString("\n\t")
			s.writer.Write(cur)
			nline := line[len(cur):]
			copy(line, nline)
			line = line[:len(nline)]
		}
	}
	s.writer.WriteString("\n\t")
	s.writer.Write(line)
}

func (s *csource) ints(data []int) {
	if len(data) == 0 {
		return
	}
	line := make([]byte, 0, width+16)
	for i, x := range data {
		cur := line
		line = strconv.AppendInt(line, int64(x), 10)
		if i < len(data)-1 {
			line = append(line, ',')
		}
		if len(line) > width-4 {
			s.writer.WriteString("\n\t")
			s.writer.Write(cur)
			nline := line[len(cur):]
			copy(line, nline)
			line = line[:len(nline)]
		}
	}
	s.writer.WriteString("\n\t")
	s.writer.Write(line)
}

func (s *csource) strings(data []string) {
	for i, x := range data {
		s.writer.WriteString("\n\t\"")
		var last byte
		for _, c := range []byte(x) {
			if 32 <= c && c <= 126 {
				if c == '\\' || c == '"' {
					s.writer.WriteByte('\\')
				} else if '0' <= c && c <= '9' && last == 0 && i == 0 {
					s.writer.WriteString("00")
				}
				s.writer.WriteByte(c)
			} else {
				var e string
				switch c {
				case 0:
					e = `\0`
				case '\t':
					e = `\t`
				case '\n':
					e = `\n`
				case '\r':
					e = `\r`
				}
				if e == "" {
					fmt.Fprintf(s.writer, "\\x%02x", c)
				} else {
					s.writer.WriteString(e)
				}
			}
			last = c
		}
		if i < len(data)-1 {
			s.writer.WriteString(`\0`)
		}
		s.writer.WriteByte('"')
	}
}

func intType(maxval int) string {
	if maxval <= math.MaxUint8 {
		return "UInt8"
	}
	if maxval <= math.MaxUint16 {
		return "UInt16"
	}
	return "UInt32"
}

func arrayIntType(arr []int) string {
	var max int
	for _, x := range arr {
		if x > max {
			max = x
		}
	}
	return intType(max)
}

type stringtable struct {
	data    []string
	offset  int
	offsets map[string]int
}

func newStringtable() (s stringtable) {
	s.offsets = make(map[string]int)
	return
}

func (t *stringtable) add(s string) int {
	if offset, exist := t.offsets[s]; exist {
		return offset
	}
	t.data = append(t.data, s)
	offset := t.offset
	t.offset += len(s) + 1
	t.offsets[s] = offset
	return offset
}

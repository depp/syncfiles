package main

import (
	"strconv"
)

func writeFilenames(charmaps []string, filename string) error {
	s, err := createCSource(filename)
	if err != nil {
		return err
	}

	w := s.writer
	w.WriteString(header)
	s.include("test.h")
	w.WriteString("const char *const kCharsetFilename[] = {\n")
	for _, fn := range charmaps {
		if fn != "" {
			w.WriteByte('\t')
			w.WriteString(strconv.Quote(fn))
			w.WriteString(",\n")
		}
	}
	w.WriteString("\tNULL\n};\n")

	return s.flush()
}

package main

import (
	"bufio"
	"fmt"
	"os"
)

// writeStrings writes a 'STR#' resource in Rez format.
func writeStrings(w *bufio.Writer, rsrc string, strs []string) {
	fmt.Fprintf(w, "resource 'STR#' (%s) {{\n", rsrc)
	for i, s := range strs {
		fmt.Fprintf(w, "\t%q", s)
		if i < len(strs)-1 {
			w.WriteByte(',')
		}
		w.WriteByte('\n')
	}
	w.WriteString("}};\n")
}

func charmapNames(d *scriptdata) []string {
	r := make([]string, len(d.charmaps))
	for i, c := range d.charmaps {
		r[i] = c.name
	}
	return r
}

func constStrings(c *constmap) []string {
	var n int
	for i := range c.values {
		if i >= n {
			n = i + 1
		}
	}
	r := make([]string, n)
	for i, name := range c.values {
		r[i] = name
	}
	return r
}

func writeRez(d *scriptdata, charmaps []string, filename string) error {
	fmt.Fprintln(os.Stderr, "Writing:", filename)
	fp, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer fp.Close()
	w := bufio.NewWriter(fp)

	w.WriteString(header)
	w.WriteString("#include \"resources.h\"\n")
	writeStrings(w, `rSTRS_Charmaps, "Character Maps"`, charmapNames(d))
	writeStrings(w, `rSTRS_Scripts, "Scripts"`, constStrings(&d.scripts))
	writeStrings(w, `rSTRS_Regions, "Regions"`, constStrings(&d.regions))
	for i, cm := range charmaps {
		if cm != "" {
			fmt.Fprintf(w, "read 'cmap' (%d, %q) %q;\n", 128+i, d.charmaps[i].name, cm)
		}
	}

	if err := w.Flush(); err != nil {
		return err
	}
	return fp.Close()
}

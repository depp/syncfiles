package main

import (
	"fmt"
	"sort"
)

type regionmap struct {
	regions []int
	charmap int
}

type regionmaps []regionmap

func (s regionmaps) Len() int      { return len(s) }
func (s regionmaps) Swap(i, j int) { s[i], s[j] = s[j], s[i] }
func (s regionmaps) Less(i, j int) bool {
	x := s[i]
	y := s[j]
	switch {
	case len(y.regions) == 0:
		return true
	case len(x.regions) == 0:
		return false
	default:
		return x.regions[0] < y.regions[0]
	}
}

type scriptmap struct {
	script  int
	regions []regionmap
}

type scriptmaps []*scriptmap

func (s scriptmaps) Len() int           { return len(s) }
func (s scriptmaps) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s scriptmaps) Less(i, j int) bool { return s[i].script < s[j].script }

// genMap generates a map from scripts and regions to charmaps.
func genMap(d *scriptdata) []*scriptmap {
	m := make(map[int]*scriptmap)
	var r []*scriptmap
	for i, cm := range d.charmaps {
		s := m[cm.script]
		if s == nil {
			s = &scriptmap{script: cm.script}
			m[cm.script] = s
			r = append(r, s)
		}
		var rgs []int
		if len(cm.regions) != 0 {
			rgs = make([]int, len(cm.regions))
			copy(rgs, cm.regions)
			sort.Ints(rgs)
		}
		s.regions = append(s.regions, regionmap{
			regions: cm.regions,
			charmap: i,
		})
	}
	for _, s := range r {
		sort.Sort(regionmaps(s.regions))
	}
	sort.Sort(scriptmaps(r))
	return r
}

// writeMap writes out a C function that returns the correct character map for a
// given script and region.
func writeMap(d *scriptdata, m []*scriptmap, filename string) error {
	s, err := createCSource(filename)
	if err != nil {
		return err
	}
	defer s.close()

	w := s.writer
	w.WriteString(header)
	w.WriteString(
		"#include \"src/convert.h\"\n" +
			"int GetCharmap(int script, int region) {\n" +
			"switch (script) {\n")
	for _, s := range m {
		fmt.Fprintf(w, "case %d: /* %s */\n", s.script, d.scripts.values[s.script])
		if len(s.regions) == 1 && len(s.regions[0].regions) == 0 {
			r := s.regions[0]
			fmt.Fprintf(w, "return %d; /* %s */\n", r.charmap, d.charmaps[r.charmap].name)
		} else {
			w.WriteString("switch (region) {\n")
			var hasdefault bool
			for _, r := range s.regions {
				if len(r.regions) == 0 {
					w.WriteString("default:\n")
					hasdefault = true
				} else {
					for _, rg := range r.regions {
						fmt.Fprintf(w, "case %d: /* %s */\n", rg, d.regions.values[rg])
					}
				}
				fmt.Fprintf(w, "return %d; /* %s */\n", r.charmap, d.charmaps[r.charmap].name)
			}
			if !hasdefault {
				w.WriteString("default:\nreturn -1;\n")
			}
			w.WriteString("}\n")
		}
	}
	w.WriteString(
		"default:\n" +
			"return -1;\n" +
			"}\n" +
			"}\n")

	return s.flush()
}

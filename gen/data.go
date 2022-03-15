package main

import (
	"encoding/csv"
	"errors"
	"fmt"
	"io"
	"os"
	"regexp"
	"strconv"
	"strings"
)

var isIdent = regexp.MustCompile("^[a-zA-Z][_a-zA-Z0-9]*$")

// A dataError indicates an error in the contents of one of the data files.
type dataError struct {
	filename string
	line     int
	column   int
	err      error
}

func (e *dataError) Error() string {
	var b strings.Builder
	b.WriteString(e.filename)
	if e.line != 0 {
		b.WriteByte(':')
		b.WriteString(strconv.Itoa(e.line))
		if e.column != 0 {
			b.WriteByte(':')
			b.WriteString(strconv.Itoa(e.column))
		}
	}
	b.WriteString(": ")
	b.WriteString(e.err.Error())
	return b.String()
}

// readHeader reads the header row of a CSV file and checks that columns exist with the given names.
func readHeader(filename string, r *csv.Reader, names ...string) error {
	row, err := r.Read()
	if err != nil {
		return err
	}
	for i, name := range names {
		if len(row) <= i {
			line, _ := r.FieldPos(0)
			return &dataError{filename, line, 0, fmt.Errorf("missing column: %q", name)}
		}
		cname := row[i]
		if !strings.EqualFold(name, cname) {
			line, col := r.FieldPos(i)
			return &dataError{filename, line, col, fmt.Errorf("column name is %q, expected %q", cname, name)}
		}
	}
	return nil
}

// A constmap is a map between names and integer values.
type constmap struct {
	names  map[string]int
	values map[int]string
}

// readConsts reads a CSV file containing a map between names and integer values.
func readConsts(filename string) (m constmap, err error) {
	fp, err := os.Open(filename)
	if err != nil {
		return m, err
	}
	defer fp.Close()
	r := csv.NewReader(fp)
	r.ReuseRecord = true
	if err := readHeader(filename, r, "name", "value"); err != nil {
		return m, err
	}
	m.names = make(map[string]int)
	m.values = make(map[int]string)
	for {
		row, err := r.Read()
		if err != nil {
			if err == io.EOF {
				break
			}
			return m, err
		}
		if len(row) < 2 {
			line, _ := r.FieldPos(0)
			return m, &dataError{filename, line, 0, errors.New("expected at least two columns")}
		}
		name := row[0]
		if !isIdent.MatchString(name) {
			line, col := r.FieldPos(0)
			return m, &dataError{filename, line, col, fmt.Errorf("invalid name: %q", name)}
		}
		if _, e := m.names[name]; e {
			line, col := r.FieldPos(0)
			return m, &dataError{filename, line, col, fmt.Errorf("duplicate name: %q", name)}
		}
		value, err := strconv.Atoi(row[1])
		if err != nil {
			line, col := r.FieldPos(1)
			return m, &dataError{filename, line, col, fmt.Errorf("invalid value: %v", err)}
		}
		m.names[name] = value
		if _, e := m.values[value]; !e {
			m.values[value] = name
		}
	}
	return m, nil
}

type charmapinfo struct {
	name    string
	file    string
	script  string
	regions []string
}

// readCharmaps reads and parses the charmaps.csv file.
func readCharmaps(filename string, scripts, regions map[string]int) ([]charmapinfo, error) {
	fp, err := os.Open(filename)
	if err != nil {
		return nil, err
	}
	defer fp.Close()
	r := csv.NewReader(fp)
	r.ReuseRecord = true
	if err := readHeader(filename, r, "name", "file", "script", "regions"); err != nil {
		return nil, err
	}
	var arr []charmapinfo
	for {
		row, err := r.Read()
		if err != nil {
			if err == io.EOF {
				break
			}
			return nil, err
		}
		if len(row) < 3 {
			line, _ := r.FieldPos(0)
			return nil, &dataError{filename, line, 0, errors.New("expected at least three columns")}
		}
		ifo := charmapinfo{
			name:   row[0],
			file:   row[1],
			script: row[2],
		}
		if _, e := scripts[ifo.script]; !e {
			line, col := r.FieldPos(2)
			return nil, &dataError{filename, line, col, fmt.Errorf("unknown script: %q", ifo.script)}
		}
		if len(row) >= 4 && row[3] != "" {
			ifo.regions = strings.Split(row[3], ";")
			for _, region := range ifo.regions {
				if _, e := regions[region]; !e {
					line, col := r.FieldPos(3)
					return nil, &dataError{filename, line, col, fmt.Errorf("unknown region: %q", region)}
				}
			}
		}
		arr = append(arr, ifo)
	}
	return arr, nil
}

type scriptdata struct {
	scripts  constmap
	regions  constmap
	charmaps []charmapinfo
}

func readData() (d scriptdata, err error) {
	d.scripts, err = readConsts("scripts/script.csv")
	if err != nil {
		return d, err
	}
	d.regions, err = readConsts("scripts/region.csv")
	if err != nil {
		return d, err
	}
	d.charmaps, err = readCharmaps("scripts/charmap.csv", d.scripts.names, d.regions.names)
	return
}

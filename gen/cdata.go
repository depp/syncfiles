package main

import "fmt"

const strlookup = `const char *%s(int cmap)
{
	if (cmap < 0 || CHARMAP_COUNT <= cmap) {
		return 0;
	}
	return kCharmapText + %s[cmap];
}
`

const datalookup = `struct CharmapData CharmapData(int cmap) {
	struct CharmapData data;
	UInt32 off0, off1;
	data.ptr = 0;
	data.size = 0;
	if (cmap < 0 || CHARMAP_COUNT <= cmap) {
		return data;
	}
	off0 = kCharmapOffset[cmap];
	off1 = kCharmapOffset[cmap+1];
	if (off0 == off1) {
		return data;
	}
	data.ptr = kCharmapData + off0;
	data.size = off1 - off0;
	return data;
}
`

func writeInfo(d *scriptdata, filename string) error {
	strs := newStringtable()
	ids := make([]int, len(d.charmaps))
	names := make([]int, len(d.charmaps))
	for i, cm := range d.charmaps {
		ids[i] = strs.add(cm.id)
		names[i] = strs.add(cm.name)
	}

	s, err := createCSource(filename)
	if err != nil {
		return err
	}

	w := s.writer
	s.include("data.h")

	w.WriteString(formatOff)

	fmt.Fprintf(w, "#define CHARMAP_COUNT %d\n", len(d.charmaps))

	fmt.Fprintf(w, "static const char kCharmapText[] =")
	s.strings(strs.data)
	w.WriteString(";\n")

	fmt.Fprintf(w, "static const %s kCharmapIDs[CHARMAP_COUNT] = {", arrayIntType(ids))
	s.ints(ids)
	w.WriteString("\n};\n")

	fmt.Fprintf(w, "static const %s kCharmapNames[CHARMAP_COUNT] = {", arrayIntType(ids))
	s.ints(ids)
	w.WriteString("\n};\n")

	w.WriteString(formatOn)

	fmt.Fprintf(w, strlookup, "CharmapID", "kCharmapIDs")
	fmt.Fprintf(w, strlookup, "CharmapName", "kCharmapNames")

	return s.flush()
}

func writeData(d *scriptdata, filename string) error {
	offsets := make([]int, len(d.charmaps)+1)
	var offset, last int
	for i, cm := range d.charmaps {
		offsets[i] = offset
		offset += len(cm.data)
		if len(cm.data) != 0 {
			last = i
		}
	}
	offsets[len(offsets)-1] = offset

	s, err := createCSource(filename)
	if err != nil {
		return err
	}

	w := s.writer
	w.WriteString(formatOff)
	s.include("data.h")
	fmt.Fprintf(w, "#define CHARMAP_COUNT %d\n", len(d.charmaps))

	fmt.Fprintf(w, "static const %s kCharmapOffset[CHARMAP_COUNT + 1] = {", arrayIntType(offsets))
	s.ints(offsets)
	w.WriteString("\n};\n")

	w.WriteString("static const UInt8 kCharmapData[] = {")
	for i, cm := range d.charmaps {
		if len(cm.data) != 0 {
			fmt.Fprintf(w, "\n\t/* %s */", cm.name)
			s.bytes(cm.data, i == last)
			if i != last {
				w.WriteByte('\n')
			}
		}
	}
	w.WriteString("\n};\n")

	w.WriteString(formatOn)

	w.WriteString(datalookup)

	return s.flush()
}

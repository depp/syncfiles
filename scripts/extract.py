"""Extract various constants from Mac OS header files.

This program reads Script.h and TextCommon.h from Macintosh headers. Pass the
paths to one or both of these files when invoking this program, and the program
will write out the corresponding CSV files.

It does not matter what line endings or character encodings the headers use, as
long as the encoding is ASCII-compatible.
"""
import csv
import os
import re
import sys

from typing import Iterator, List, Tuple

Item = Tuple[str, int]

def list_enums(filename: str) -> Iterator[Item]:
    """List enum definitions in a header file."""
    with open(filename, 'rb') as fp:
        data = fp.read()
    for item in re.finditer(
            rb'^\s*(\w+)\s*=\s*((?:0x)?\d+)u?l?\b',
            data, re.MULTILINE | re.IGNORECASE):
        name, value = item.groups()
        yield name.decode('ASCII'), int(value, 0)

def index_of(data: List[Item], key: str) -> int:
    """Return the index of the enum with the given name."""
    for i, (name, _) in enumerate(data):
        if name == key:
            return i
    raise ValueError('missing value: {!r}'.format(key))

def slice(data: List[Item], first: str, last: str) -> List[Item]:
    """Return a slice of enums, by giving the name of the first and last."""
    return data[index_of(data, first):index_of(data, last)+1]

def write_csv(fname: str, data: List[Item]) -> None:
    """Write a CSV file containing enum values."""
    print('Writing', fname, file=sys.stderr)
    with open(fname, 'w') as fp:
        w = csv.writer(fp)
        w.writerow(['Name', 'Value'])
        for item in data:
            w.writerow(item)

def process_script(filename: str) -> None:
    """Process the <Script.h> header file."""
    scripts: List[Item] = []
    regions: List[Item] = []
    for name, value in list_enums(filename):
        if name.startswith('sm'):
            scripts.append((name, value))
        elif name.startswith('ver'):
            regions.append((name, value))
    write_csv('script.csv', slice(scripts, 'smRoman', 'smUninterp'))
    write_csv('region.csv', slice(regions, 'verUS', 'verGreenland'))

def process_textcommon(filename: str) -> None:
    """Process the <TextCommon.h> header file."""
    encodings: List[Item] = []
    for name, value in list_enums(filename):
        if name.startswith('kTextEncoding'):
            encodings.append((name, value))
    write_csv('encoding.csv', encodings)

def process(filename: str) -> None:
    """Process any header file."""
    name = os.path.basename(filename).lower()
    if name == 'script.h':
        process_script(filename)
    elif name == 'textcommon.h':
        process_textcommon(filename)
    else:
        print('Error: unknown header file:', repr(filename), file=sys.stderr)
        raise SystemExit(1)

def main(argv: List[str]) -> None:
    if not argv:
        sys.stderr.write(
            'Usage: script_gen.py [<file.h>...]\n'
            'This will read Script.h and TextCommon.h\n')
        raise SystemExit(2)
    for arg in argv:
        process(arg)

if __name__ == '__main__':
    main(sys.argv[1:])

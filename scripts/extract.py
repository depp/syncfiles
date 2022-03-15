"""Extract script and region constants from Script.h."""
import csv
import re
import sys

from typing import List, Tuple

Item = Tuple[str, int]

def index_of(data: List[Item], key: str) -> int:
    for i, (name, _) in enumerate(data):
        if name == key:
            return i
    raise ValueError('missing value: {!r}'.format(key))

def slice(data: List[Item], first: str, last: str) -> List[Item]:
    return data[index_of(data, first):index_of(data, last)+1]

def write_csv(fname: str, data: List[Item]) -> None:
    with open(fname, 'w') as fp:
        w = csv.writer(fp)
        w.writerow(['Name', 'Value'])
        for item in data:
            w.writerow(item)

def main(argv: List[str]) -> None:
    if len(argv) != 1:
        print('usage: script_gen.py <Script.h>', file=sys.stderr)
        raise SystemExit(2)
    with open(argv[0], 'rb') as fp:
        data = fp.read()
    scripts: List[Item] = []
    regions: List[Item] = []
    for item in re.finditer(rb'^\s*(\w+)\s*=\s*(\d+)', data, re.MULTILINE):
        name, value = item.groups()
        if name.startswith(b'sm'):
            scripts.append((name.decode('ASCII'), int(value)))
        elif name.startswith(b'ver'):
            regions.append((name.decode('ASCII'), int(value)))
    write_csv('script.csv', slice(scripts, 'smRoman', 'smUninterp'))
    write_csv('region.csv', slice(regions, 'verUS', 'verGreenland'))

if __name__ == '__main__':
    main(sys.argv[1:])

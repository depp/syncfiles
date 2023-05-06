"""copymac.py -- Copy source files to/from Basilisk, with conversions."""
from abc import ABCMeta, abstractmethod
import os
import pathlib
import sys
from dataclasses import dataclass
from typing import Dict, List

import xattr  # type: ignore

SUBDIRS = ". macos sync lib".split()
TEXT_SUFFIXES = ".c .h .r".split()
TEXT_CREATOR = b"R*ch"  # BBEdit


class Converter(metaclass=ABCMeta):
    name: str

    @abstractmethod
    def convert(self, src: pathlib.Path, dest: pathlib.Path) -> None:
        pass


class TextConverter(Converter):
    enc_in: str
    enc_out: str
    newline: str

    name = "Text"

    def __init__(self, enc_in: str, enc_out: str, newline: str) -> None:
        self.enc_in = enc_in
        self.enc_out = enc_out
        self.newline = newline

    def convert(self, src: pathlib.Path, dest: pathlib.Path) -> None:
        text = src.read_text(encoding=self.enc_in)
        lines = text.splitlines()
        lines.append("")
        dest.write_text(self.newline.join(lines), encoding=self.enc_out)


class DataConverter(Converter):
    name = "Data"

    def convert(self, src: pathlib.Path, dest: pathlib.Path) -> None:
        data = src.read_bytes()
        dest.write_bytes(data)


@dataclass
class FileType:
    converter: Converter
    filetype: bytes
    creator: bytes

    def finder_info(self) -> bytes:
        assert len(self.filetype) == 4
        assert len(self.creator) == 4
        return self.filetype + self.creator + b"\x01" + b"\x00" * 23


def main(argv: List[str]) -> None:
    if not argv:
        print("Usage: python copymac.py (push|pull)", file=sys.stderr)
        print("  pull: pull FROM guest (macintosh), into guest (unix)", file=sys.stderr)
        print("  push: push TO guest (macintosh), from host (unix)", file=sys.stderr)
        raise SystemExit(2)
    unix_dir = pathlib.Path(__file__).resolve().parent
    mac_dir = pathlib.Path(pathlib.Path.home(), "SyncFiles")
    cmd = argv[0]

    suffixes: Dict[str, FileType] = {}
    text: Converter
    data = DataConverter()
    if cmd == "pull":
        text = TextConverter("macintosh", "utf-8", "\n")
        src_dir, dest_dir = mac_dir, unix_dir
    elif cmd == "push":
        text = TextConverter("utf-8", "macintosh", "\r")
        src_dir, dest_dir = unix_dir, mac_dir
    else:
        print("Error: unknown command", file=sys.stderr)
        raise SystemExit(2)

    text_type = FileType(text, b"TEXT", TEXT_CREATOR)
    for suffix in TEXT_SUFFIXES:
        suffixes[suffix] = text_type
    suffixes[".mcp"] = FileType(data, b"MMPr", b"CWIE")

    for subdir in SUBDIRS:
        src_subdir = src_dir / subdir
        dest_subdir = dest_dir / subdir
        dest_subdir.mkdir(exist_ok=True)
        fset = set()
        for src in src_subdir.iterdir():
            print(src)
            file_type = suffixes.get(src.suffix)
            if file_type is not None:
                fset.add(src.name)
                print(file_type.converter.name, pathlib.Path(subdir, src.name))
                dest = dest_subdir / src.name
                file_type.converter.convert(src, dest)
                if cmd == "push":
                    info = file_type.finder_info()
                    attr = xattr.xattr(dest)
                    attr["com.apple.FinderInfo"] = info
        for dest in dest_subdir.iterdir():
            if dest.suffix in suffixes and not dest.name in fset:
                print("Delete", pathlib.Path(subdir, dest.name))
                dest.unlink()


if __name__ == "__main__":
    main(sys.argv[1:])

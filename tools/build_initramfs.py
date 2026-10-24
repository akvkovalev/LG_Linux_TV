#!/usr/bin/env python3
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import stat


def build(root: Path, output: Path) -> dict[str, object]:
    image = bytearray()
    inode = 0
    entries: list[str] = []

    def add(name: str, mode: int, data: bytes = b"", major: int = 0, minor: int = 0) -> None:
        nonlocal inode
        inode += 1
        encoded = name.encode() + b"\0"
        fields = [inode, mode, 0, 0, 2 if stat.S_ISDIR(mode) else 1, 0,
                  len(data), 0, 0, major, minor, len(encoded), 0]
        image.extend(b"070701" + b"".join(f"{n:08x}".encode() for n in fields))
        image.extend(encoded)
        image.extend(b"\0" * (-len(image) % 4))
        image.extend(data)
        image.extend(b"\0" * (-len(image) % 4))
        entries.append(name)

    add(".", stat.S_IFDIR | 0o755)
    for path in sorted(root.rglob("*")):
        relative = path.relative_to(root).as_posix()
        info = path.lstat()
        if stat.S_ISDIR(info.st_mode):
            data = b""
        elif stat.S_ISLNK(info.st_mode):
            data = os.readlink(path).encode()
        elif stat.S_ISREG(info.st_mode):
            data = path.read_bytes()
        else:
            continue
        add(relative, info.st_mode, data)
    for name, mode, major, minor in (
        ("dev/console", 0o600, 5, 1), ("dev/null", 0o666, 1, 3),
        ("dev/zero", 0o666, 1, 5), ("dev/random", 0o666, 1, 8),
        ("dev/urandom", 0o666, 1, 9), ("dev/tty", 0o666, 5, 0),
        ("dev/mem", 0o600, 1, 1),
    ):
        add(name, stat.S_IFCHR | mode, major=major, minor=minor)
    add("TRAILER!!!", 0)
    compressed = gzip.compress(bytes(image), compresslevel=9, mtime=0)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(compressed)
    return {"files": len(entries), "cpio_bytes": len(image),
            "compressed_bytes": len(compressed),
            "sha256": hashlib.sha256(compressed).hexdigest()}


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    print(json.dumps(build(args.root, args.output), indent=2))

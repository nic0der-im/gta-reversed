#!/usr/bin/env python3
"""Inventory every StaticRef/ScopedStaticRef address used by the game code and classify it
by which section of the retail executable it lands in.

Only addresses in an initialized .rdata/.data range need an initializer in the native build;
the rest of .data is the executable's zero-filled globals (its BSS) and is already covered by
the zero-initialized storage that `notsa::native::ExeGlobals::Get` hands out.

Usage: GTASA_EXE=/path/to/gta_sa.exe tools/native/staticref_report.py [--csv PATH] [--check]
Exit code 1 with --check if any initialized address has no entry in ExeData.generated.cpp.
"""
from __future__ import annotations

import argparse
import os
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "source"
GENERATED = SRC / "app" / "platform" / "native" / "ExeData.generated.cpp"
CALL_RE = re.compile(r"\b(?:Scoped)?StaticRef\s*<")
HEX_RE = re.compile(r"0x[0-9A-Fa-f]+")


@dataclass(frozen=True)
class Site:
    addr: int
    type: str
    path: str
    line: int


@dataclass
class Section:
    name: str
    va: int
    vsize: int
    initialized: bool

    def __contains__(self, addr: int) -> bool:
        return self.va <= addr < self.va + self.vsize


def read_sections(exe: Path) -> list[Section]:
    data = exe.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    _, nsec, _, _, _, opt_size, _ = struct.unpack_from("<HHIIIHH", data, pe + 4)
    sections = []
    table = pe + 24 + opt_size
    for i in range(nsec):
        name, vsize, va, raw_size, raw_ptr = struct.unpack_from("<8sIIII", data, table + i * 40)
        sections.append(
            Section(
                name=name.rstrip(b"\0").decode(),
                va=va,
                vsize=max(vsize, raw_size),
                # A section whose virtual size exceeds its raw size ends in zero-filled data.
                initialized=False,
            )
        )
        sections[-1].raw = (raw_ptr, raw_size)
    return sections


def classify(addr: int, sections: list[Section]) -> str:
    for s in sections:
        if addr in s:
            raw_ptr, raw_size = s.raw
            if s.name == ".text":
                return "text"
            return "file" if addr < s.va + raw_size else "zero"
    # Globals above the image (0x8B0000+) are regions the retail executable reserves/allocates
    # at startup; they start zeroed and get their values from the exe's static initializers.
    return "runtime"


def parse_sites() -> list[Site]:
    sites = []
    for path in sorted(SRC.rglob("*")):
        if path.suffix not in (".h", ".hpp", ".cpp", ".ixx") or not path.is_file():
            continue
        text = path.read_text(errors="replace")
        # Only sites behind an address literal are interesting; ignore the template definition.
        for m in CALL_RE.finditer(text):
            depth, i = 1, m.end()
            while i < len(text) and depth:
                depth += {"<": 1, ">": -1}.get(text[i], 0)
                i += 1
            type_name = " ".join(text[m.end() : i - 1].split())
            close = text.find(")", i)
            if close < 0:
                continue
            first_arg = text[i:close].split(",")[0].strip().strip("()").strip()
            if not HEX_RE.fullmatch(first_arg):
                continue
            line = text.count("\n", 0, m.start()) + 1
            sites.append(Site(int(first_arg, 16), type_name, str(path.relative_to(ROOT)), line))
    return sites


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", default=str(ROOT / ".local" / "native" / "staticrefs.csv"))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    exe = Path(os.environ.get("GTASA_EXE", ROOT / ".local" / "gta_sa.exe"))
    if not exe.is_file():
        print(f"Executable not found: {exe} (set GTASA_EXE)", file=sys.stderr)
        return 2
    sections = read_sections(exe)
    sites = parse_sites()
    if not sites:
        print("No StaticRef sites found", file=sys.stderr)
        return 2

    rows = [(s, classify(s.addr, sections)) for s in sites]
    csv_path = Path(args.csv)
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    with csv_path.open("w") as f:
        f.write("addr,type,section,file,line\n")
        for s, kind in sorted(rows, key=lambda r: r[0].addr):
            f.write(f"0x{s.addr:X},{s.type},{kind},{s.path},{s.line}\n")

    counts = {}
    for _, kind in rows:
        counts[kind] = counts.get(kind, 0) + 1
    init_addrs = sorted({s.addr for s, k in rows if k == "file"})
    print(f"{len(rows)} sites, {len({s.addr for s in sites})} distinct addresses -> {csv_path}")
    print("by section: " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))
    print(f"in file-backed regions: {len(init_addrs)} (table/constant pool or runtime-written globals)")
    top = {}
    for s, k in rows:
        if k == "file":
            top[s.type] = top.get(s.type, 0) + 1
    for name, n in sorted(top.items(), key=lambda kv: -kv[1])[:15]:
        print(f"  {n:4}  {name}")

    if args.check:
        have = set()
        if GENERATED.is_file():
            text = GENERATED.read_text()
            for m in re.finditer(r"^\s*INIT\((0x[0-9A-Fa-f]+)", text, re.M):
                have.add(int(m.group(1), 16))
            # Rows of the exe static-initializer table (`tools/native/gen_exedata.py`):
            # `EXEDATA_ROW(kind, addr, aux, size, count, value)`.
            for m in re.finditer(
                r"EXEDATA_ROW\(\w+,\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+),\s*(\d+),\s*(\d+)", text
            ):
                addr, stride, size, count = (int(m.group(i), 0) for i in range(1, 5))
                for i in range(max(count, 1)):
                    base = addr + i * stride
                    have.update(range(base, base + max(size, 1)))
        missing = [a for a in init_addrs if a not in have]
        if missing:
            print(f"{len(missing)} StaticRef addresses in file-backed regions are absent from ExeData.generated.cpp:", file=sys.stderr)
            for a in missing[:20]:
                print(f"  0x{a:X}", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

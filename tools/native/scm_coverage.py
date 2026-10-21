#!/usr/bin/env python3
"""Report which SCM commands used by the game's scripts the native build cannot execute.

The tool linearly decodes the script segments of ``data/script/main.scm`` (the ``MAIN``
segment plus every mission of the mission table) and of every ``.scm`` stored in
``data/script/script.img``, then classifies each opcode it encounters against the
handlers registered in ``source/game_sa/Scripts/Commands/*.cpp``:

* ``no_handler``      - no handler is registered for the opcode at all (the native build
                       falls through to the retail handler table, which is empty there).
* ``not_implemented`` - ``REGISTER_COMMAND_UNIMPLEMENTED`` / ``REGISTER_UNSUPPORTED_COMMAND_HANDLER``.

Outputs (under ``.local/native`` by default):

* ``scm_missing.csv``  - ``opcode,name,status,segment,uses``, one row per opcode and segment.
* ``scm_desync.csv``   - ``segment,offset,reason``, segments whose linear decode desynced.

``segment`` is ``MAIN``, ``MISSION_<n>`` (0-based index into the main.scm mission table) or
``IMG:<name>``. ``MAIN`` covers ``[0, MainScriptSize)`` of main.scm, including the SCM header
chunks, whose ``GOTO`` instructions are followed so that chunk payloads are skipped exactly
like the script VM does.

The opcode/parameter database is ``.local/native/sa.json`` (Sanny Builder library, downloaded
on first use). Only the Python standard library is used.

Usage: tools/native/scm_coverage.py [--game-dir DIR] [--out-dir DIR] [--refresh-library]
"""
from __future__ import annotations

import argparse
import csv
import json
import os
import re
import struct
import sys
import urllib.error
import urllib.request
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COMMANDS_DIR = ROOT / "source" / "game_sa" / "Scripts" / "Commands"
ENUM_FILE = ROOT / "source" / "game_sa" / "Enums" / "eScriptCommands.h"
LOCAL_DIR = ROOT / ".local" / "native"
LIBRARY_URL = "https://raw.githubusercontent.com/sannybuilder/library/master/sa/sa.json"
DEFAULT_GAME_DIR = Path(os.environ.get("GTASA_DIR", "/Users/ignacio/Games/GTASA"))

#! Vanilla SA opcode ceiling (`COMMAND_HIGHEST_VANILLA_ID` in `eScriptCommands.h`). An opcode
#! above it without the NOT bit means the linear decode lost sync.
HIGHEST_VANILLA_OPCODE = 0x0A8E

#! Payload size of each script parameter type, in bytes, excluding the type byte itself.
#! Types 7/8/12/13/18/19 are `scm::ArrayAccess` (6 bytes); 9/15 are 8/16 byte strings;
#! 14 is a Pascal string and is handled separately because it is variable length.
PARAM_SIZES = {
    1: 4,   # SCRIPT_PARAM_STATIC_INT_32BITS
    2: 2,   # SCRIPT_PARAM_GLOBAL_NUMBER_VARIABLE
    3: 2,   # SCRIPT_PARAM_LOCAL_NUMBER_VARIABLE
    4: 1,   # SCRIPT_PARAM_STATIC_INT_8BITS
    5: 2,   # SCRIPT_PARAM_STATIC_INT_16BITS
    6: 4,   # SCRIPT_PARAM_STATIC_FLOAT
    7: 6,   # SCRIPT_PARAM_GLOBAL_NUMBER_ARRAY
    8: 6,   # SCRIPT_PARAM_LOCAL_NUMBER_ARRAY
    9: 8,   # SCRIPT_PARAM_STATIC_SHORT_STRING
    10: 2,  # SCRIPT_PARAM_GLOBAL_SHORT_STRING_VARIABLE
    11: 2,  # SCRIPT_PARAM_LOCAL_SHORT_STRING_VARIABLE
    12: 6,  # SCRIPT_PARAM_GLOBAL_SHORT_STRING_ARRAY
    13: 6,  # SCRIPT_PARAM_LOCAL_SHORT_STRING_ARRAY
    15: 16,  # SCRIPT_PARAM_STATIC_LONG_STRING
    16: 2,  # SCRIPT_PARAM_GLOBAL_LONG_STRING_VARIABLE
    17: 2,  # SCRIPT_PARAM_LOCAL_LONG_STRING_VARIABLE
    18: 6,  # SCRIPT_PARAM_GLOBAL_LONG_STRING_ARRAY
    19: 6,  # SCRIPT_PARAM_LOCAL_LONG_STRING_ARRAY
}
PARAM_END_OF_ARGUMENTS = 0
PARAM_STATIC_PASCAL_STRING = 14

MAX_MAIN_HEADER_CHUNKS = 6
#! `m_ChunkIndex` of the header chunks the game walks in `CTheScripts::GetSCMChunk`.
MAIN_HEADER_CHUNK_INDICES = {ord("s"), 0, 1, 2, 3, 4}
IMG_MAGIC = b"VER2"
IMG_SECTOR_SIZE = 2048
IMG_ENTRY_STRIDE = 0x20


class SegmentDesync(Exception):
    """The linear decode cannot continue at `offset`."""

    def __init__(self, offset: int, reason: str) -> None:
        super().__init__(reason)
        self.offset = offset
        self.reason = reason


@dataclass(frozen=True)
class ScmCommand:
    """One SCM command as described by the Sanny Builder library."""

    opcode: int
    name: str
    inputs: tuple[str, ...]
    outputs: int


@dataclass
class SegmentResult:
    name: str
    uses: Counter = field(default_factory=Counter)


# ---------------------------------------------------------------------------------------------
# SCM header / command library
# ---------------------------------------------------------------------------------------------

def ensure_library(path: Path, refresh: bool) -> None:
    """Download the Sanny Builder library database when it is not available yet."""
    if path.is_file() and not refresh:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {LIBRARY_URL} -> {path}")
    try:
        with urllib.request.urlopen(LIBRARY_URL, timeout=60) as response:
            payload = response.read()
    except (urllib.error.URLError, OSError) as error:
        raise SystemExit(f"error: cannot download the command library: {error}") from error
    tmp = path.with_suffix(path.suffix + ".tmp")
    tmp.write_bytes(payload)
    tmp.replace(path)


def load_library(path: Path) -> dict[int, ScmCommand]:
    try:
        document = json.loads(path.read_text(encoding="utf-8"))
        extension = next(e for e in document["extensions"] if e["name"] == "default")
        entries = extension["commands"]
    except (OSError, ValueError, KeyError, StopIteration) as error:
        raise SystemExit(f"error: cannot read the command library {path}: {error}") from error
    commands: dict[int, ScmCommand] = {}
    for entry in entries:
        try:
            opcode = int(entry["id"], 16)
        except (KeyError, TypeError, ValueError) as error:
            raise SystemExit(f"error: malformed command entry {entry!r} in {path}") from error
        commands.setdefault(
            opcode,
            ScmCommand(
                opcode=opcode,
                name=entry.get("name", f"UNKNOWN_0x{opcode:04X}"),
                inputs=tuple(i.get("type", "") for i in entry.get("input") or ()),
                outputs=len(entry.get("output") or ()),
            ),
        )
    return commands


def load_command_names() -> dict[int, str]:
    """Map opcode -> `COMMAND_*` enum name from `eScriptCommands.h`."""
    text = ENUM_FILE.read_text(encoding="utf-8", errors="replace")
    names: dict[int, str] = {}
    for match in re.finditer(r"\b(COMMAND_[A-Za-z0-9_]+)\s*=\s*(0x[0-9A-Fa-f]+)", text):
        names[int(match.group(2), 16)] = match.group(1)
    if not names:
        raise SystemExit(f"error: no COMMAND_* values found in {ENUM_FILE}")
    return names


# ---------------------------------------------------------------------------------------------
# Registered handlers
# ---------------------------------------------------------------------------------------------

def strip_cpp_comments(text: str) -> str:
    """Remove `//` and `/* */` comments, keeping line numbering and string literals intact."""
    out: list[str] = []
    i, length = 0, len(text)
    while i < length:
        ch = text[i]
        if ch == "/" and i + 1 < length:
            nxt = text[i + 1]
            if nxt == "/":
                end = text.find("\n", i)
                if end < 0:
                    break
                out.append("\n")
                i = end + 1
                continue
            if nxt == "*":
                end = text.find("*/", i + 2)
                if end < 0:
                    break
                out.append("\n" * text.count("\n", i, end))
                i = end + 2
                continue
        if ch in "\"'":
            end = i + 1
            while end < length:
                if text[end] == "\\":
                    end += 2
                    continue
                if text[end] == ch:
                    end += 1
                    break
                end += 1
            out.append(text[i:end])
            i = end
            continue
        out.append(ch)
        i += 1
    return "".join(out)


HANDLER_RE = re.compile(r"REGISTER_COMMAND_(?:HANDLER|NOP)\s*\(\s*(COMMAND_[A-Za-z0-9_]+)")
NOT_IMPLEMENTED_RE = re.compile(
    r"REGISTER_(?:COMMAND_UNIMPLEMENTED|UNSUPPORTED_COMMAND_HANDLER)\s*\(\s*(COMMAND_[A-Za-z0-9_]+)"
)


def load_handlers(name_to_opcode: dict[int, str]) -> tuple[set[int], set[int]]:
    """Return (covered, not_implemented) opcode sets from `Scripts/Commands/*.cpp`."""
    # Enum names can be defined twice inside `#if BUILD_XBOX` / `#else` (e.g. DO_DEBUG_STUFF).
    # The first definition belongs to the retail build, so it wins.
    opcode_by_name: dict[str, int] = {}
    for opcode, name in name_to_opcode.items():
        opcode_by_name.setdefault(name, opcode)
    covered: set[int] = set()
    not_implemented: set[int] = set()
    unknown: set[str] = set()

    def resolve(name: str) -> int | None:
        opcode = opcode_by_name.get(name)
        if opcode is None:
            unknown.add(name)
        return opcode

    for path in sorted(COMMANDS_DIR.glob("*.cpp")):
        text = strip_cpp_comments(path.read_text(encoding="utf-8", errors="replace"))
        for match in NOT_IMPLEMENTED_RE.finditer(text):
            opcode = resolve(match.group(1))
            if opcode is not None:
                not_implemented.add(opcode)
        for match in HANDLER_RE.finditer(text):
            opcode = resolve(match.group(1))
            if opcode is not None:
                covered.add(opcode)

    if unknown:
        print(f"warning: {len(unknown)} registered command names are not in eScriptCommands.h: "
              f"{', '.join(sorted(unknown)[:5])}{'...' if len(unknown) > 5 else ''}",
              file=sys.stderr)
    both = covered & not_implemented
    if both:
        print(f"warning: {len(both)} opcodes are both registered and marked unimplemented; "
              f"treating them as covered", file=sys.stderr)
        not_implemented -= both
    return covered, not_implemented


# ---------------------------------------------------------------------------------------------
# Linear decoding
# ---------------------------------------------------------------------------------------------

def read_param(data: bytes, pos: int, end: int) -> tuple[int, int]:
    """Consume one script parameter starting at `pos`, mirroring `CRunningScript`'s reader."""
    if pos >= end:
        raise SegmentDesync(pos, "truncated parameter")
    ptype = data[pos]
    type_offset = pos
    pos += 1
    if ptype == PARAM_END_OF_ARGUMENTS:
        return pos, ptype
    if ptype == PARAM_STATIC_PASCAL_STRING:
        if pos >= end:
            raise SegmentDesync(type_offset, "truncated Pascal string")
        pos += 1 + data[pos]
    elif ptype in PARAM_SIZES:
        pos += PARAM_SIZES[ptype]
    else:
        raise SegmentDesync(type_offset, f"invalid parameter type {ptype} (0x{ptype:02X})")
    if pos > end:
        raise SegmentDesync(type_offset, f"parameter type {ptype} runs past the segment end")
    return pos, ptype


def decode_segment(
    data: bytes,
    start: int,
    end: int,
    commands: dict[int, ScmCommand],
    header_jumps: dict[int, int],
) -> Counter:
    """Linearly decode `[start, end)` and count the opcodes it uses."""
    uses: Counter = Counter()
    end = min(end, len(data))
    pos = start
    while pos + 2 <= end:
        jump = header_jumps.get(pos)
        if jump is not None:
            # The SCM header chunks are a chain of `GOTO <next chunk>`, the last one jumping
            # into the main script. Following them skips the chunk payloads, which are data.
            if data[pos:pos + 3] != b"\x02\x00\x01":
                raise SegmentDesync(pos, "SCM header chunk does not start with GOTO")
            pos = jump
            continue

        word = struct.unpack_from("<H", data, pos)[0]
        if word == 0 and not data[pos:end].strip(b"\x00"):
            break  # Trailing zero padding after the last instruction.

        opcode = word & 0x7FFF  # The high bit is the `NOT` flag.
        if opcode > HIGHEST_VANILLA_OPCODE:
            raise SegmentDesync(pos, f"opcode 0x{opcode:04X} is above the vanilla range")

        command = commands.get(opcode)
        if command is None:
            raise SegmentDesync(pos, f"opcode 0x{opcode:04X} is not in the command library")

        p = pos + 2
        for input_type in command.inputs:
            if input_type == "string128":
                # `SAVE_STRING_TO_DEBUG_FILE` stores a raw 128 byte string without a type byte.
                p += 128
                if p > end:
                    raise SegmentDesync(pos, f"command 0x{opcode:04X} has a truncated string128")
                continue
            if input_type == "arguments":
                while True:
                    p, ptype = read_param(data, p, end)
                    if ptype == PARAM_END_OF_ARGUMENTS:
                        break
                continue
            p, ptype = read_param(data, p, end)
            if ptype == PARAM_END_OF_ARGUMENTS:
                raise SegmentDesync(pos, f"command 0x{opcode:04X} argument list ended early")
        for _ in range(command.outputs):
            # Results are stored through `StoreArg`, which reads a script variable.
            p, ptype = read_param(data, p, end)
            if ptype == PARAM_END_OF_ARGUMENTS:
                raise SegmentDesync(pos, f"command 0x{opcode:04X} output list ended early")

        uses[opcode] += 1
        pos = p
    return uses


# ---------------------------------------------------------------------------------------------
# main.scm / script.img
# ---------------------------------------------------------------------------------------------

@dataclass
class MainScm:
    data: bytes
    header_jumps: dict[int, int]
    main_size: int
    mission_offsets: list[int]


def parse_main_scm(path: Path) -> MainScm:
    data = path.read_bytes()
    header_jumps: dict[int, int] = {}
    main_size: int | None = None
    mission_offsets: list[int] = []
    offset = 0
    for _ in range(MAX_MAIN_HEADER_CHUNKS):
        if offset + 8 > len(data):
            raise SystemExit(f"error: {path} is truncated at header chunk {offset:#x}")
        if data[offset:offset + 3] != b"\x02\x00\x01":
            break  # Not a header chunk: the chain reached the main script.
        chunk_index = data[offset + 7]
        if chunk_index not in MAIN_HEADER_CHUNK_INDICES:
            break
        next_offset = struct.unpack_from("<I", data, offset + 3)[0]
        if next_offset <= offset:
            raise SystemExit(f"error: {path} header chunk chain loops at {offset:#x}")
        header_jumps[offset] = next_offset
        if chunk_index == 1:  # tSCMScriptFileInfoChunk
            main_size, _largest, count, _exclusive, _largest_locals = struct.unpack_from(
                "<IIHHI", data, offset + 8
            )
            mission_offsets = list(struct.unpack_from(f"<{count}I", data, offset + 0x18))
        offset = next_offset
    if main_size is None:
        raise SystemExit(f"error: {path} has no script file info chunk (tSCMScriptFileInfoChunk)")
    return MainScm(data, header_jumps, main_size, mission_offsets)


def read_img_entries(path: Path) -> tuple[bytes, list[tuple[str, int, int]]]:
    """Return the IMG data and its `(name, start, end)` entries (SA IMG v2, 2048 byte sectors)."""
    data = path.read_bytes()
    if data[:4] == IMG_MAGIC:
        count = struct.unpack_from("<I", data, 4)[0]
        base = 8
    else:
        base = 0
        # Plain IMG v2: the directory starts at the file start, and the first entry's data
        # offset (in sectors) marks where the directory ends.
        first_sector = struct.unpack_from("<I", data, 0)[0] if len(data) >= 4 else 0
        count = (first_sector * IMG_SECTOR_SIZE) // IMG_ENTRY_STRIDE

    entries: list[tuple[str, int, int]] = []
    for index in range(count):
        offset = base + index * IMG_ENTRY_STRIDE
        if offset + IMG_ENTRY_STRIDE > len(data):
            break
        sector, size = struct.unpack_from("<II", data, offset)
        raw_name = data[offset + 8:offset + IMG_ENTRY_STRIDE].split(b"\x00", 1)[0]
        name = "".join(chr(byte) for byte in raw_name if 32 <= byte < 127)
        if not name or sector == 0 or size == 0:
            continue
        start = sector * IMG_SECTOR_SIZE
        if start >= len(data):
            continue
        end = min(len(data), start + size * IMG_SECTOR_SIZE)
        entries.append((name, start, end))
    return data, entries


# ---------------------------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------------------------

def collect_segments(
    game_dir: Path, commands: dict[int, ScmCommand]
) -> tuple[list[SegmentResult], list[tuple[str, int, str]]]:
    segments: list[SegmentResult] = []
    desyncs: list[tuple[str, int, str]] = []

    main_path = game_dir / "data" / "script" / "main.scm"
    img_path = game_dir / "data" / "script" / "script.img"
    for path in (main_path, img_path):
        if not path.is_file():
            raise SystemExit(f"error: script file not found: {path}")

    scm = parse_main_scm(main_path)
    mission_starts = sorted(set(scm.mission_offsets))

    def decode_into(name: str, data: bytes, start: int, end: int, jumps: dict[int, int]) -> None:
        try:
            uses = decode_segment(data, start, end, commands, jumps)
        except SegmentDesync as error:
            desyncs.append((name, error.offset, error.reason))
            return
        segments.append(SegmentResult(name, uses))

    decode_into("MAIN", scm.data, 0, scm.main_size, scm.header_jumps)
    for index, mission_offset in enumerate(scm.mission_offsets):
        mission_end = next((start for start in mission_starts if start > mission_offset), len(scm.data))
        decode_into(f"MISSION_{index}", scm.data, mission_offset, mission_end, {})

    img_data, img_entries = read_img_entries(img_path)
    for name, start, end in img_entries:
        decode_into(f"IMG:{name}", img_data, start, end, {})

    return segments, desyncs


def write_missing_csv(path: Path, rows: list[tuple[int, str, str, str, int]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["opcode", "name", "status", "segment", "uses"])
        for opcode, name, status, segment, uses in rows:
            writer.writerow([f"0x{opcode:04X}", name, status, segment, uses])


def write_desync_csv(path: Path, desyncs: list[tuple[str, int, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(["segment", "offset", "reason"])
        for segment, offset, reason in desyncs:
            writer.writerow([segment, f"0x{offset:06X}", reason])


def summarize(
    rows: list[tuple[int, str, str, str, int]],
    desyncs: list[tuple[str, int, str]],
    total_segments: int,
    missing_path: Path,
    desync_path: Path,
) -> None:
    by_status: dict[str, tuple[int, int]] = {}
    for status in ("no_handler", "not_implemented"):
        opcodes = {opcode for opcode, _, st, _, _ in rows if st == status}
        uses = sum(uses for _, _, st, _, uses in rows if st == status)
        by_status[status] = (len(opcodes), uses)

    totals: Counter = Counter()
    for opcode, _, _, _, uses in rows:
        totals[opcode] += uses
    row_by_opcode = {opcode: (name, status) for opcode, name, status, _, _ in rows}

    main_rows = sum(1 for *_, segment, _ in rows if segment == "MAIN")

    print()
    print(f"SCM segments decoded: {total_segments - len(desyncs)}/{total_segments} "
          f"({len(desyncs)} desynced)")
    print(f"Missing-handler rows: {len(rows)} (MAIN: {main_rows}, missions + script.img: "
          f"{len(rows) - main_rows})")
    for status, (opcodes, uses) in by_status.items():
        print(f"  {status:<16} {opcodes:5d} opcodes, {uses:7d} uses")

    print("Top 20 missing commands by uses:")
    for opcode, uses in totals.most_common(20):
        name, status = row_by_opcode[opcode]
        print(f"  0x{opcode:04X} {uses:6d}  {status:<16} {name}")

    if desyncs:
        print(f"Desynced segments ({desync_path.name}):")
        for segment, offset, reason in desyncs:
            print(f"  {segment} at 0x{offset:06X}: {reason}")

    print(f"Wrote {missing_path} ({len(rows)} rows)")
    print(f"Wrote {desync_path} ({len(desyncs)} rows)")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Report SCM commands used by the game scripts that the native build cannot handle."
    )
    parser.add_argument("--game-dir", type=Path, default=DEFAULT_GAME_DIR,
                        help=f"GTA San Andreas directory (default: {DEFAULT_GAME_DIR})")
    parser.add_argument("--out-dir", type=Path, default=LOCAL_DIR,
                        help=f"directory for the generated CSV files (default: {LOCAL_DIR})")
    parser.add_argument("--library", type=Path, default=None,
                        help="path to sa.json (default: <out-dir>/sa.json)")
    parser.add_argument("--refresh-library", action="store_true",
                        help="download sa.json even if it is already present")
    args = parser.parse_args()

    library_path = args.library or (args.out_dir / "sa.json")
    ensure_library(library_path, args.refresh_library)

    commands = load_library(library_path)
    names = load_command_names()
    covered, not_implemented = load_handlers(names)

    segments, desyncs = collect_segments(args.game_dir, commands)

    rows: list[tuple[int, str, str, str, int]] = []
    for segment in segments:
        for opcode, uses in sorted(segment.uses.items()):
            if opcode in not_implemented:
                status = "not_implemented"
            elif opcode in covered:
                continue
            else:
                status = "no_handler"
            name = names.get(opcode) or commands[opcode].name
            rows.append((opcode, name, status, segment.name, uses))

    missing_path = args.out_dir / "scm_missing.csv"
    desync_path = args.out_dir / "scm_desync.csv"
    write_missing_csv(missing_path, rows)
    write_desync_csv(desync_path, desyncs)
    summarize(rows, desyncs, len(segments) + len(desyncs), missing_path, desync_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())

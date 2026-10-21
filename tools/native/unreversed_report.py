#!/usr/bin/env python3
"""Inventory of `plugin::Call*` exe addresses that survive in the native build.

The native build compiles with `NOTSA_NATIVE` defined, so any `plugin::Call*`
inside `#ifndef NOTSA_NATIVE` blocks is not part of the executable and must not
be reported.  A plain text scan cannot tell those apart, so every translation
unit is preprocessed with its own compile command (line markers kept) and only
lines attributed to the scanned file itself are inspected.

Output columns: `file,address,function`
  - `file`     repo-relative path of the source file
  - `address`  `0x%06X` address passed to `plugin::Call*`
  - `function` nearest `// 0xADDR` comment at/above the call plus the enclosing
               signature name, e.g. `0x4212E0 CCarCtrl::Init`

Headers are not translation units; they inherit the compile command of the
first `.cpp` (sorted by path) whose preprocessing includes them.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIR = ROOT / "source"
COMPILE_COMMANDS = ROOT / "build" / "Release" / "compile_commands.json"
OUTPUT_CSV = ROOT / ".local" / "native" / "unreversed.csv"

# Paths (repo-relative) that are never part of the native build's game code.
EXCLUDED_PREFIXES = (
    "game_sa/RenderWare/rw/",
    "game_sa/RenderWare/D3D",
)
EXCLUDED_NAMES = ("PluginBase.h",)
EXCLUDED_PARTS = ("third_party",)

COMPILABLE_SUFFIXES = (".cpp", ".cc", ".cxx", ".c", ".mm", ".m")
HEADER_SUFFIXES = (".h", ".hpp", ".hh", ".hxx", ".inl", ".inc", ".ipp")

CALL_RE = re.compile(r"plugin::Call\w*\s*<")
ADDR_RE = re.compile(r"0x([0-9A-Fa-f]+)")
MARKER_RE = re.compile(r'^#\s+(\d+)\s+"((?:[^"\\]|\\.)*)"(?:\s+(\d+))?\s*$')
COMMENT_RE = re.compile(r"^\s*//\s*0x([0-9A-Fa-f]+)")


def relpath(path: Path | str) -> str:
    """Repo-relative posix path; absolute paths outside the repo are kept as-is."""
    p = Path(path)
    try:
        return p.resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return p.as_posix()


def is_excluded(rel: str) -> bool:
    if any(rel.startswith(prefix) for prefix in EXCLUDED_PREFIXES):
        return True
    if any(rel.endswith(name) or f"/{name}" in rel for name in EXCLUDED_NAMES):
        return True
    return any(part in rel.split("/") for part in EXCLUDED_PARTS)


def discover_candidates() -> list[Path]:
    """Source files under `source/` that still mention `plugin::Call` in text."""
    candidates = []
    for path in sorted(SOURCE_DIR.rglob("*")):
        if not path.is_file():
            continue
        rel = relpath(path)
        if not rel.startswith("source/") or is_excluded(rel[len("source/"):]):
            continue
        if path.suffix.lower() not in COMPILABLE_SUFFIXES + HEADER_SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        if "plugin::Call" in text:
            candidates.append(path)
    return candidates


def load_compile_commands() -> dict[str, dict]:
    with open(COMPILE_COMMANDS, encoding="utf-8") as f:
        entries = json.load(f)
    commands: dict[str, dict] = {}
    for entry in entries:
        commands.setdefault(os.path.realpath(entry["file"]), entry)
    return commands


def build_command(entry: dict, output: str) -> list[str]:
    if "arguments" in entry:
        argv = list(entry["arguments"])
    else:
        argv = shlex.split(entry["command"])
    # Appending another `-o` wins over the earlier `-c ... -o ...o` pair, and
    # `-E` keeps the line markers (never pass `-P`).
    return argv + ["-E", "-o", output]


def preprocess(rel: str, entry: dict, cache_dir: Path) -> tuple[str, Path | None, str]:
    out = cache_dir / (re.sub(r"[^A-Za-z0-9_.-]", "_", rel) + ".i")
    argv = build_command(entry, str(out))
    try:
        proc = subprocess.run(
            argv,
            cwd=entry["directory"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
        )
    except OSError as exc:  # pragma: no cover - compiler missing
        return rel, None, str(exc)
    if proc.returncode != 0:
        first = proc.stderr.strip().splitlines()
        return rel, None, first[-1] if first else f"exit code {proc.returncode}"
    return rel, out, ""


def scan_preprocessed(
    stream_path: Path,
    candidate_rel: set[str],
    directory: str,
) -> dict[str, list[tuple[int, str]]]:
    """Collect lines of candidate files from one preprocessed translation unit.

    Returns `{repo_relative_file: [(original_line_number, line_text), ...]}`.
    """
    found: dict[str, list[tuple[int, str]]] = {}
    path_cache: dict[str, str | None] = {}
    current: str | None = None
    is_candidate = False
    line_no = 0
    with open(stream_path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            if line.startswith("#"):
                m = MARKER_RE.match(line)
                if m:
                    marker_path = m.group(2).replace('\\"', '"')
                    if marker_path in path_cache:
                        current = path_cache[marker_path]
                    else:
                        if not os.path.isabs(marker_path):
                            marker_path = os.path.join(directory, marker_path)
                        normalized = relpath(os.path.normpath(marker_path))
                        current = normalized if normalized in candidate_rel else None
                        path_cache[m.group(2)] = current
                    is_candidate = current is not None
                    line_no = int(m.group(1))
                    continue
            if is_candidate and "plugin::Call" in line:
                found.setdefault(current, []).append((line_no, line))
            line_no += 1
    return found


def extract_addresses(lines: list[tuple[int, str]]) -> list[tuple[int, int]]:
    """Yield `(original_line, address)` for every `plugin::Call*<... 0xADDR`."""
    calls: list[tuple[int, int]] = []
    pending: int | None = None  # line number of a call whose address is on a later line
    for line_no, text in lines:
        if pending is not None:
            m = ADDR_RE.search(text)
            if m:
                calls.append((pending, int(m.group(1), 16)))
                pending = None
                continue
            if "(" in text or ";" in text:
                pending = None
        pos = 0
        while True:
            m = CALL_RE.search(text, pos)
            if not m:
                break
            rest = text[m.end():]
            addr = ADDR_RE.search(rest)
            if addr:
                calls.append((line_no, int(addr.group(1), 16)))
                pos = m.end() + addr.end()
            else:
                pending = line_no
                break
    return calls


def _signature_name(line: str) -> str:
    """Extract the function name from a signature line."""
    prefix = line.split("(", 1)[0].strip()
    tokens = re.findall(r"[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*", prefix)
    if not tokens:
        return ""
    name = tokens[-1]
    if name in ("new", "delete") and "operator" in prefix:
        name = "operator " + name
    return name


def _looks_like_signature(stripped: str) -> bool:
    if not stripped or stripped.startswith(("//", "/*", "*", "#", "}", "@")):
        return False
    if stripped.endswith(";"):
        return False
    if "(" not in stripped:
        return False
    if re.match(r"^(if|for|while|switch|return|else|do|catch|case)\b", stripped):
        return False
    if "=" in stripped.split("(", 1)[0]:
        return False
    return True


def _is_signature_start(stripped: str) -> bool:
    """Whether a signature line is a complete-enough start to stop the scan."""
    return (
        stripped.endswith("{")
        or stripped.endswith(")")
        or stripped.endswith("const")
        or stripped.endswith("(")
        or stripped.endswith(",")
        or stripped.endswith("override")
        or stripped.endswith("final")
    )


def find_function(lines: list[str], call_index: int) -> str:
    """`0xADDR Signature` using the nearest address comment and the enclosing signature."""
    comment_addr = None
    for i in range(call_index - 1, -1, -1):
        m = COMMENT_RE.match(lines[i])
        if m:
            comment_addr = int(m.group(1), 16)
            break

    name = ""
    for i in range(call_index - 1, -1, -1):
        stripped = lines[i].strip()
        # A closing brace at column 0 ends the previous top-level definition.
        if not lines[i][:1].isspace() and stripped.startswith("}"):
            break
        if not _looks_like_signature(stripped):
            continue
        if _is_signature_start(stripped):
            name = _signature_name(lines[i])
            if name:
                break
    if not name:
        # Inline definition on the call line, e.g. `void Foo() override { plugin::Call…`.
        before = lines[call_index].split("plugin::Call", 1)[0]
        if "(" in before and ")" in before:
            name = _signature_name(before)
    if comment_addr is not None:
        return f"0x{comment_addr:06X} {name}".strip()
    return name


def preprocess_and_scan(
    items: list[tuple[str, dict]],
    commands: dict[str, dict],
    candidate_rel: set[str],
    cache_dir: Path,
    jobs: int,
    verbose: bool,
) -> dict[str, dict[str, list[tuple[int, str]]]]:
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        results = list(
            pool.map(lambda item: preprocess(item[0], item[1], cache_dir), items)
        )
    per_unit: dict[str, dict[str, list[tuple[int, str]]]] = {}
    for rel, stream, err in results:
        if stream is None:
            print(f"warning: preprocessing failed for {rel}: {err}", file=sys.stderr)
            continue
        directory = commands[os.path.realpath(ROOT / rel)]["directory"]
        per_unit[rel] = scan_preprocessed(stream, candidate_rel, directory)
        if verbose:
            print(f"preprocessed {rel}", file=sys.stderr)
    return per_unit


def find_includer_units(
    headers: list[Path],
    commands: dict[str, dict],
    already_preprocessed: set[str],
) -> dict[str, str]:
    """First translation unit (sorted by path) that directly includes each header."""
    wanted = {relpath(h): h.name for h in headers}
    found: dict[str, str] = {}
    if not wanted:
        return found
    for real, entry in sorted(commands.items(), key=lambda item: relpath(item[0])):
        rel = relpath(real)
        if rel in already_preprocessed:
            continue
        try:
            text = Path(entry["file"]).read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for header_rel, name in wanted.items():
            if header_rel in found:
                continue
            if re.search(r'#\s*include\s*[<"][^">]*' + re.escape(name) + r'[">]', text):
                found[header_rel] = rel
        if len(found) == len(wanted):
            break
    return found


def collect_rows(
    candidates: list[Path],
    commands: dict[str, dict],
    jobs: int,
    verbose: bool,
) -> list[tuple[str, int, str]]:
    candidate_rel = {relpath(p) for p in candidates}
    units = [p for p in candidates if p.suffix.lower() in COMPILABLE_SUFFIXES]
    headers = [p for p in candidates if p.suffix.lower() in HEADER_SUFFIXES]

    preprocessable: list[tuple[str, dict]] = []
    skipped: list[str] = []
    for path in units:
        entry = commands.get(os.path.realpath(path))
        if entry is None:
            skipped.append(relpath(path))
            continue
        preprocessable.append((relpath(path), entry))

    cache_dir = Path(tempfile.mkdtemp(prefix="unreversed-report-"))
    try:
        per_unit = preprocess_and_scan(
            preprocessable, commands, candidate_rel, cache_dir, jobs, verbose
        )
    finally:
        shutil.rmtree(cache_dir, ignore_errors=True)

    if skipped and verbose:
        for rel in skipped:
            print(f"skipped (no compile command): {rel}", file=sys.stderr)

    # Headers inherit the first translation unit (sorted by path) that includes them.
    source_lines: dict[str, list[tuple[int, str]]] = {}
    for rel in sorted(per_unit):
        for included, lines in per_unit[rel].items():
            if included not in source_lines:
                source_lines[included] = lines
    for rel, _entry in preprocessable:
        if rel in per_unit:
            source_lines.setdefault(rel, per_unit[rel].get(rel, []))

    # A header whose includers are not themselves candidates still needs a command.
    unresolved = [h for h in headers if not source_lines.get(relpath(h))]
    if unresolved:
        includers = find_includer_units(unresolved, commands, set(per_unit))
        extra = [
            (rel, commands[os.path.realpath(ROOT / rel)])
            for rel in sorted(set(includers.values()))
        ]
        extra_units: dict[str, dict[str, list[tuple[int, str]]]] = {}
        if extra:
            cache_dir = Path(tempfile.mkdtemp(prefix="unreversed-report-"))
            try:
                extra_units = preprocess_and_scan(
                    extra, commands, candidate_rel, cache_dir, jobs, verbose
                )
            finally:
                shutil.rmtree(cache_dir, ignore_errors=True)
        unresolved_rel = {relpath(h) for h in unresolved}
        for rel in sorted(extra_units):
            for included, lines in extra_units[rel].items():
                if included in unresolved_rel and not source_lines.get(included):
                    source_lines[included] = lines

    text_cache: dict[str, list[str]] = {}
    rows: list[tuple[str, int, str]] = []
    for rel in sorted(source_lines):
        path = ROOT / rel
        if rel not in text_cache:
            try:
                text_cache[rel] = path.read_text(encoding="utf-8", errors="ignore").splitlines()
            except OSError:
                text_cache[rel] = []
        original = text_cache[rel]
        for call_line, address in extract_addresses(source_lines[rel]):
            function = find_function(original, call_line - 1) if call_line else ""
            rows.append((rel, address, function))

    return rows


def write_csv(rows: list[tuple[str, int, str]]) -> None:
    OUTPUT_CSV.parent.mkdir(parents=True, exist_ok=True)
    with open(OUTPUT_CSV, "w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["file", "address", "function"])
        for rel, address, function in sorted(rows):
            writer.writerow([rel, f"0x{address:06X}", function])


def normalize_check_path(value: str) -> str:
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    rel = relpath(path)
    if not rel.startswith("source/") and (SOURCE_DIR / rel).is_file():
        rel = f"source/{rel}"
    return rel


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Report `plugin::Call*` exe addresses that survive the native build."
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--summary", action="store_true", help="print per-file row counts (descending)"
    )
    mode.add_argument(
        "--check",
        metavar="PATH",
        help="exit 1 when PATH still contains plugin::Call stubs, 0 otherwise",
    )
    mode.add_argument(
        "--list-files", action="store_true", help="print the distinct affected files"
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=min(8, os.cpu_count() or 4),
        help="parallel preprocess jobs (default: %(default)s)",
    )
    parser.add_argument("--verbose", action="store_true", help="log per-file progress")
    args = parser.parse_args(argv)

    if not COMPILE_COMMANDS.is_file():
        parser.error(f"missing {COMPILE_COMMANDS}; configure build/Release first")

    rows = collect_rows(discover_candidates(), load_compile_commands(), args.jobs, args.verbose)

    if args.summary:
        counts: dict[str, int] = {}
        for rel, _, _ in rows:
            counts[rel] = counts.get(rel, 0) + 1
        for rel, count in sorted(counts.items(), key=lambda item: (-item[1], item[0])):
            print(f"{count:5d}  {rel}")
        return 0

    if args.list_files:
        for rel in sorted({rel for rel, _, _ in rows}):
            print(rel)
        return 0

    if args.check is not None:
        rel = normalize_check_path(args.check)
        count = sum(1 for row in rows if row[0] == rel)
        print(f"{count} plugin::Call row(s) in {rel}")
        return 1 if count else 0

    write_csv(rows)
    print(f"Wrote {len(rows)} rows ({len({r[0] for r in rows})} files) to {relpath(OUTPUT_CSV)}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except BrokenPipeError:
        # `--summary | head` is a normal usage; don't dump a traceback.
        sys.stdout = open(os.devnull, "w")
        sys.exit(0)

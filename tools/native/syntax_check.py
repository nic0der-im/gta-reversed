#!/usr/bin/env python3
"""Syntax-only compile of a source file using build/Release/compile_commands.json.

Drops the stale `-include-pch` group (and the CMake `-include` precompiled header)
so the file can be checked without rebuilding the PCH.
"""
import json
import os
import shlex
import subprocess
import sys

CC_JSON = 'build/Release/compile_commands.json'


def build_cmd(entry):
    toks = shlex.split(entry['command'])
    out = []
    skip = 0
    for tok in toks:
        if skip:
            skip -= 1
            continue
        if tok in ('-Xclang',):
            skip = 1
            continue
        if tok in ('-include-pch', '-include', '-o'):
            skip = 1
            continue
        if tok in ('-Winvalid-pch', '-c'):
            continue
        if tok.endswith('.o') or tok.endswith('.pch'):
            continue
        out.append(tok)
    return out


def main(paths):
    entries = json.load(open(CC_JSON))
    by_file = {e['file']: e for e in entries}
    rc = 0
    for path in paths:
        entry = by_file.get(os.path.abspath(path))
        if entry is None:
            print(f'{path}: no compile command')
            rc = 1
            continue
        cmd = build_cmd(entry) + ['-fsyntax-only', entry['file']]
        res = subprocess.run(cmd, capture_output=True, text=True)
        print(f'== {path}: exit {res.returncode}')
        if res.returncode != 0:
            rc = res.returncode
            print(res.stderr)
    return rc


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))

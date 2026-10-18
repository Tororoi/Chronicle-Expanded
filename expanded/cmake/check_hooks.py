#!/usr/bin/env python3
"""Fail when an Expanded hook sits in ps2/src code the port build leaves out.

    check_hooks.py --originals <ps2/src> --copies <build>/ps2_src -o <stamp> <unit.cpp>...

The build compiles each ps2/src unit as a copy without the definitions port/src or expanded/src replace
(PORT_OVERRIDE). A hook (an expanded:: call) inside one of those definitions never runs: its place is the
replacement. Each unit's hooks are counted in the original and in the copy; the copy must keep them all.
"""
import argparse
import os
import re
import sys

HOOK = re.compile(r'\bexpanded::\w+\s*\(')


def hooks(path):
    with open(path, encoding='utf-8', errors='replace') as f:
        lines = f.read().splitlines()
    found = {}
    for number, line in enumerate(lines, 1):
        for match in HOOK.finditer(line):
            found.setdefault(match.group(0).rstrip('( \t'), []).append(number)
    return found


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--originals', required=True)
    ap.add_argument('--copies', required=True)
    ap.add_argument('-o', '--output', required=True)
    ap.add_argument('units', nargs='*')
    a = ap.parse_args()
    dead = []
    for unit in a.units:
        original = hooks(os.path.join(a.originals, unit))
        copy = hooks(os.path.join(a.copies, unit))
        for name, lines in original.items():
            if len(copy.get(name, [])) < len(lines):
                dead.append(f'ps2/src/{unit}:{lines[0]}: {name} is in a definition the port replaces; hook the replacement')
    if dead:
        print('\n'.join(dead), file=sys.stderr)
        sys.exit(1)
    with open(a.output, 'w') as f:
        f.write('ok\n')


if __name__ == '__main__':
    main()

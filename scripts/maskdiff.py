#!/usr/bin/env python3
"""Relocation-masked comparison of two linked-image address ranges.

A raw byte diff of two GameServer builds is dominated by relocations: any
layout difference makes every rel32 call/jmp and every absolute data
reference resolve elsewhere, so the diff shows ~95% difference even when the
code is canonically identical. This masks those operands and diffs the
instruction streams instead, so the real per-function differences show up.

Usage:
    scripts/maskdiff.py --a GameServer.exe:0x4012c8-0x407928 \
                        --b build/GameServer.exe:0x401370-0x40779c \
                        [--context N] [--limit N]

Both ranges are disassembled with objdump, each instruction is canonicalised
(relative branches keep their condition, call/jmp targets and absolute
addresses become ADDR, hex immediates are compared numerically), and the two
streams are aligned with difflib. Output is the aligned diff plus a summary:
instruction counts and the number of differing instructions.

PROGRESS/DIAGNOSTIC instrument: a canonical match here is necessary but not
sufficient for byte identity — final acceptance is the whole-file MD5.
"""
import argparse
import difflib
import re
import subprocess
import sys

ADDR_RE = re.compile(r'0x4[0-9a-f]{5}|0x5[0-9a-f]{5}')
INSN_RE = re.compile(r'^\s*([0-9a-f]+):\s+((?:[0-9a-f]{2} )+)\s*(\S+)\s*(.*)$')


def disasm(path, start, end):
    out = subprocess.run(
        ['objdump', '-d', f'--start-address={start}', f'--stop-address={end}', path],
        capture_output=True, text=True).stdout
    return out.splitlines()


def canon(line):
    m = INSN_RE.match(line)
    if not m:
        return None
    mnem, ops = m.group(3), m.group(4)
    ops = ADDR_RE.sub('ADDR', ops)
    ops = re.sub(r'<[^>]*>', '', ops).strip()
    ops = re.sub(r'\s+', ' ', ops)
    if mnem in ('call', 'calll') and not ops.startswith('*'):
        ops = 'ADDR'
    if mnem in ('jmp', 'jmpl', 'jmpq', 'jmp*'):
        ops = 'ADDR'
    return f'{mnem} {ops}'.strip()


def stream(path, start, end):
    out = []
    for line in disasm(path, start, end):
        c = canon(line)
        if c:
            out.append(c)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--a', required=True, help='ref: FILE:START-END')
    ap.add_argument('--b', required=True, help='ours: FILE:START-END')
    ap.add_argument('--context', type=int, default=2)
    ap.add_argument('--limit', type=int, default=40)
    args = ap.parse_args()

    def spec(s):
        f, rng = s.split(':')
        lo, hi = rng.split('-')
        return f, int(lo, 16), int(hi, 16)

    fa, sa, ea = spec(args.a)
    fb, sb, eb = spec(args.b)
    A = stream(fa, sa, ea)
    B = stream(fb, sb, eb)
    print(f'A {fa} {hex(sa)}-{hex(ea)}: {len(A)} instructions')
    print(f'B {fb} {hex(sb)}-{hex(eb)}: {len(B)} instructions')
    print(f'net B-A = {len(B)-len(A):+d}')

    sm = difflib.SequenceMatcher(None, A, B, autojunk=False)
    ops = [o for o in sm.get_opcodes() if o[0] != 'equal']
    print(f'differing regions: {len(ops)}')
    shown = 0
    for tag, i1, i2, j1, j2 in ops:
        if shown >= args.limit:
            print('...')
            break
        print(f'\n== {tag}  A[{i1}:{i2}] B[{j1}:{j2}]  (A {i2-i1}, B {j2-j1})')
        lo = max(0, i1 - args.context)
        for k in range(lo, min(i1 + args.context, len(A))):
            print(f'   A {A[k]}')
        for k in range(i1, i2):
            print(f'  -A {A[k]}')
        for k in range(j1, j2):
            print(f'  +B {B[k]}')
        shown += 1


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Generate the static-initializer data of `gta_sa.exe` for the native build.

The retail executable's globals live in the `.data` section, whose file-backed part ends at
`0x8E4000`: everything the initializers write is in the zero-fill tail, so those initial values
exist *only as code*. This script reads the CRT C++ initializer table (`__xi_a..__xi_z`, walked by
`_cinit` at `0x823B76`), decodes every initializer body, and emits the equivalent writes as a
table that `notsa::native::ApplyExeData()` replays into `ExeGlobals`.

Every entry is classified into exactly one kind:

    STORE_BYTES(addr, bytes)            mov dword/word/byte [imm], imm (and the equivalent
                                        register forms after constant folding)
    STORE_FLOAT(addr, bits)             x87 chain ending in `fstp dword [imm]`
    ZERO_FILL(addr, len)                `rep stosd` of zero
    FILL_PATTERN(addr, stride, n, data) constant stores repeated with a constant stride (or a
                                        non-zero `rep stosd`), and constant element constructors
                                        of `_eh_vector_constructor_iterator` arrays
    VECTOR_CTOR(base, ctor, elem, n)    `_eh_vector_constructor_iterator` whose element
                                        constructor is not a trivial constant initializer
    OBJECT_CTOR(this, ctor)             `mov ecx,<global>` + `call`/`jmp <ctor>`
    SKIP                                no effect (`_atexit`-only bodies, writes that are
                                        already zero in the native build)
    UNHANDLED(entry)                    anything not decoded with certainty; reported, never guessed

Table order is preserved: the emitted table is applied in the same order the executable's `_cinit`
would have applied the initializers.

Usage:
    python3 tools/native/gen_exedata.py [--exe PATH] [--check] [--report PATH] [-v]

`--check` regenerates in memory and fails if the checked-in files differ (used by CI/review).
"""

from __future__ import annotations

import argparse
import bisect
import os
import re
import struct
import sys
from dataclasses import dataclass, field
from fractions import Fraction

try:
    import capstone
except ImportError:  # pragma: no cover - fallback for machines without capstone
    capstone = None


# ---------------------------------------------------------------------------------------------
# Disassembler back ends
# ---------------------------------------------------------------------------------------------
#
# `capstone` is the primary decoder. When it is not installed the same `disasm(bytes, address)`
# interface is provided on top of `llvm-objdump -d --x86-asm-syntax=intel`, whose text is parsed
# back into `mnemonic`/operands. Anything the text parser does not understand raises
# `Unhandled` for that entry, so the fallback can only lose entries, never invent values.

if capstone is not None:
    CS_OP_REG, CS_OP_IMM, CS_OP_MEM = capstone.CS_OP_REG, capstone.CS_OP_IMM, capstone.CS_OP_MEM
else:  # pragma: no cover - only used when capstone is missing
    CS_OP_INVALID, CS_OP_REG, CS_OP_IMM, CS_OP_MEM = 'invalid', 'reg', 'imm', 'mem'


def _find_objdump() -> str:
    import shutil

    for name in ('llvm-objdump',):
        found = shutil.which(name)
        if found:
            return found
    for candidate in (
        '/Library/Developer/CommandLineTools/usr/bin/llvm-objdump',
        '/opt/homebrew/opt/llvm/bin/llvm-objdump',
        '/usr/local/opt/llvm/bin/llvm-objdump',
    ):
        if os.path.exists(candidate):
            return candidate
    raise SystemExit('neither `capstone` nor `llvm-objdump` is available')


REGISTER_SIZES = {
    'al': 1, 'ah': 1, 'bl': 1, 'bh': 1, 'cl': 1, 'ch': 1, 'dl': 1, 'dh': 1,
    'ax': 2, 'bx': 2, 'cx': 2, 'dx': 2, 'si': 2, 'di': 2, 'bp': 2, 'sp': 2,
}


class TextOperand:
    """`capstone`-shaped operand built from objdump text."""

    __slots__ = ('type', 'size', 'reg', 'imm', 'mem')

    class Memory:
        __slots__ = ('base', 'index', 'scale', 'disp')

        def __init__(self, base, index, scale, disp):
            self.base, self.index, self.scale, self.disp = base, index, scale, disp

    def __init__(self, type_, size=4, reg=None, imm=None, mem=None):
        self.type, self.size, self.reg, self.imm = type_, size, reg, imm
        self.mem = mem


class TextInsn:
    """`capstone`-shaped instruction built from objdump text."""

    __slots__ = ('address', 'mnemonic', 'op_str', 'operands')

    def __init__(self, address, mnemonic, op_str, operands):
        self.address, self.mnemonic, self.op_str, self.operands = address, mnemonic, op_str, operands

    @staticmethod
    def reg_name(reg):
        return reg  # Text operands carry register names already


SIZE_PREFIXES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8, 'tbyte': 10}


def _parse_int(text: str) -> int | None:
    try:
        return int(text, 0)
    except ValueError:
        return None


def _parse_operand(text: str) -> TextOperand | None:
    text = re.sub(r'\s*<[^>]*>', '', text).strip()  # drop `0x... <.text+0x...>` annotations
    text = re.sub(r'\b(?:es|ds|ss|cs|fs|gs):', '', text)  # drop segment overrides

    if '[' in text:
        size = 4
        head, _, rest = text.partition('[')
        head = head.strip()
        if head:
            if not head.endswith('ptr'):
                return None
            size = SIZE_PREFIXES.get(head[:-3].strip(), 0)
            if not size:
                return None
        rest = rest.strip()
        if not rest.endswith(']'):
            return None
        base = index = None
        scale = 1
        disp = 0
        for token in (t.strip() for t in re.split(r'(?=[+-])', rest[:-1])):
            if not token:
                continue
            sign = 1
            if token[0] in '+-':
                sign = -1 if token[0] == '-' else 1
                token = token[1:].strip()
            if '*' in token:
                # llvm-objdump prints the scale first (`4*eax`), capstone's model wants it last.
                left, _, right = token.partition('*')
                left, right = _parse_int(left), _parse_int(right)
                if left is not None and right is None:
                    scale, index = left, token.partition('*')[2].strip()
                elif right is not None and left is None:
                    index, scale = token.partition('*')[0].strip(), right
                else:
                    return None
                continue
            value = _parse_int(token)
            if value is not None:
                disp += sign * value
            elif base is None:
                base = token
            else:
                return None
        return TextOperand(CS_OP_MEM, size, mem=TextOperand.Memory(base, index, scale, disp))

    value = _parse_int(text)
    if value is not None:
        return TextOperand(CS_OP_IMM, 4, imm=value)
    if re.fullmatch(r'[a-z]{2,4}', text):
        return TextOperand(CS_OP_REG, REGISTER_SIZES.get(text, 4), reg=text)
    return None


class ObjdumpDisassembler:
    """`md.disasm(bytes, address)` on top of `llvm-objdump` (used when capstone is missing)."""

    def __init__(self, image: Image):
        self.image = image
        self.objdump = _find_objdump()
        self.detail = True  # Accepted for interface compatibility

    def disasm(self, code: bytes, address: int):
        import subprocess

        section = self.image.section_of(address)
        stop = address + len(code)
        if section is not None:
            stop = min(stop, section.end)
        if stop <= address:
            return
        output = subprocess.run(
            [self.objdump, '-d', '--x86-asm-syntax=intel', '--no-show-raw-insn',
             f'--start-address={address:#x}', f'--stop-address={stop:#x}', self.image.path],
            capture_output=True, text=True, check=True
        ).stdout

        for line in output.splitlines():
            match = re.match(r'\s+([0-9a-f]+):(.*)$', line)
            if not match:
                continue
            payload = match.group(2)
            if payload.lstrip(' ').startswith('<'):
                continue  # Section header (`00401000 <.text>:`)
            fields = [f.strip() for f in payload.split('\t') if f.strip()]
            if not fields:
                continue
            insn_address = int(match.group(1), 16)
            mnemonic = fields[0]
            # Strip objdump's `0xADDR <.text+0xADDR>` annotations so comments match capstone.
            operand_text = re.sub(r'\s*<[^>]*>', '', ', '.join(fields[1:]))
            if mnemonic in ('rep', 'repe', 'repne', 'lock'):
                # `rep` is printed as its own column; fold it back into the mnemonic.
                sub = operand_text.split(None, 1)
                if not sub:
                    raise Unhandled('unparsable `rep` prefix')
                mnemonic = f'{mnemonic} {sub[0].rstrip(",")}'
                operand_text = sub[1] if len(sub) > 1 else ''
            operands = []
            if operand_text:
                for part in operand_text.split(','):
                    operand = _parse_operand(part)
                    if operand is None:
                        raise Unhandled(f'unparsable operand `{part.strip()}`')
                    operands.append(operand)
            yield TextInsn(insn_address, mnemonic, operand_text, operands)

# ---------------------------------------------------------------------------------------------
# Layout of the retail executable
# ---------------------------------------------------------------------------------------------

#: `__xi_a` .. `__xi_z`: the C++ initializer table walked by `_cinit` (`0x823BC7`..`0x823BD4`).
XI_TABLE_START = 0x008A4000
XI_TABLE_END = 0x008A5A10

#: `_atexit` (`0x821D1E`), `_eh_vector_constructor_iterator` (`0x821D30`).
FN_ATEXIT = 0x00821D1E
FN_VECTOR_CTOR = 0x00821D30
#: `_ftol`-like helper: `fistp qword` + overflow checks, returns the integer in `eax`.
FN_FTOL = 0x00821B40

#: The executable's `.data` section (VA 0x8A4000, VirtualSize 0x3FA000). `ExeGlobals` backs this
#: whole range with one zero-filled arena, which is where `ApplyExeData()` writes.
DATA_START = 0x008A4000
DATA_END = 0x00C9E000

#: Every 4-byte value that points inside the executable is a pointer; such stores cannot be
#: represented as constants (the native build has no executable image), so their entries stay
#: UNHANDLED on purpose.
POINTER_START = 0x00401000

MAX_PAYLOAD = 8  # payload bytes carried by one row (`ExeDataEntry::Value`)


def u16(data: bytes, off: int) -> int:
    return struct.unpack_from('<H', data, off)[0]


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from('<I', data, off)[0]


@dataclass(frozen=True)
class Section:
    name: str
    va: int
    vsize: int
    rawsize: int
    rawptr: int

    @property
    def end(self) -> int:
        return self.va + max(self.vsize, self.rawsize)


class Image:
    """Read-only view of the executable, addressed by link-time virtual address."""

    def __init__(self, path: str):
        with open(path, 'rb') as f:
            self.data = f.read()
        self.path = path
        pe = u32(self.data, 0x3C)
        if self.data[pe:pe + 4] != b'PE\0\0':
            raise SystemExit(f'{path}: not a PE image')
        nsec, optsz = u16(self.data, pe + 6), u16(self.data, pe + 20)
        opt = pe + 24
        self.image_base = u32(self.data, opt + 28)
        self.sections: list[Section] = []
        for i in range(nsec):
            o = opt + optsz + i * 40
            name = self.data[o:o + 8].rstrip(b'\0').decode('ascii', 'replace')
            vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', self.data, o + 8)
            self.sections.append(Section(name, self.image_base + va, vsize, rawsize, rawptr))
        self.sections.sort(key=lambda s: s.va)
        self._starts = [s.va for s in self.sections]
        self.data_end = max(s.end for s in self.sections)

    def section_of(self, va: int) -> Section | None:
        i = bisect.bisect_right(self._starts, va) - 1
        if i < 0:
            return None
        s = self.sections[i]
        return s if va < s.end else None

    def byte(self, va: int) -> int | None:
        s = self.section_of(va)
        if s is None:
            return None
        off = va - s.va
        if off < s.rawsize:
            return self.data[s.rawptr + off]
        return 0  # `.bss`-like tail: zero until the initializers write it

    def read(self, va: int, size: int) -> bytes:
        out = bytearray()
        for i in range(size):
            b = self.byte(va + i)
            if b is None:
                raise SystemExit(f'read outside the image at 0x{va + i:X}')
            out.append(b)
        return bytes(out)


class Memory:
    """The executable's memory while its initializers run, with undo support."""

    def __init__(self, image: Image):
        self.image = image
        self.overlay: dict[int, int | None] = {}
        self.journal: list[tuple[int, bool, int | None]] = []

    def mark(self) -> int:
        return len(self.journal)

    def rollback(self, mark: int) -> None:
        while len(self.journal) > mark:
            addr, had, old = self.journal.pop()
            if had:
                self.overlay[addr] = old
            else:
                self.overlay.pop(addr, None)

    def _byte(self, addr: int) -> int | None:
        if addr in self.overlay:
            return self.overlay[addr]
        return self.image.byte(addr)

    def read(self, addr: int, size: int) -> bytes | None:
        out = bytearray()
        for i in range(size):
            b = self._byte(addr + i)
            if b is None:
                return None
            out.append(b)
        return bytes(out)

    def write(self, addr: int, values: bytes | list[int | None]) -> None:
        for i, v in enumerate(values):
            a = addr + i
            self.journal.append((a, a in self.overlay, self.overlay.get(a)))
            self.overlay[a] = v


# ---------------------------------------------------------------------------------------------
# x87 / float32 helpers
# ---------------------------------------------------------------------------------------------

def f32_to_fraction(bits: int) -> Fraction | None:
    val = struct.unpack('<f', struct.pack('<I', bits))[0]
    if val != val or val in (float('inf'), float('-inf')):
        return None
    return Fraction(val)


def f64_to_fraction(bits: int) -> Fraction | None:
    val = struct.unpack('<d', struct.pack('<Q', bits))[0]
    if val != val or val in (float('inf'), float('-inf')):
        return None
    return Fraction(val)


def fraction_to_f32(value: Fraction) -> int | None:
    """Round exactly to float32 (round-half-to-even), like `fstp dword` does with the default CW."""
    if value == 0:
        return 0
    sign = 0
    if value < 0:
        sign, value = 0x80000000, -value

    # exponent: 2**e <= value < 2**(e+1)
    e = value.numerator.bit_length() - value.denominator.bit_length()
    while Fraction(2) ** e > value:
        e -= 1
    while Fraction(2) ** (e + 1) <= value:
        e += 1

    def scaled_round(exp: int) -> int:
        scaled = value / Fraction(2) ** exp
        q, r = divmod(scaled.numerator, scaled.denominator)
        twice = 2 * r
        if twice > scaled.denominator or (twice == scaled.denominator and q & 1):
            q += 1
        return q

    if e < -126:  # subnormal (or zero) result
        mant = scaled_round(-149)
        if mant == 0:
            return sign
        if mant >= (1 << 23):  # rounded up into the smallest normal
            return sign | (1 << 23)
        return sign | mant

    if e > 127:
        return None  # overflow: `fstp` would produce infinity, which no initializer needs

    mant = scaled_round(e - 23)
    if mant == (1 << 24):
        mant >>= 1
        e += 1
        if e > 127:
            return None
    return sign | ((e + 127) << 23) | (mant & 0x7FFFFF)


def f32_bits(value: Fraction) -> int | None:
    return fraction_to_f32(value)


class X87:
    """x87 stack restricted to the operations the initializers use, with exact arithmetic."""

    def __init__(self):
        self.stack: list[Fraction] = []

    def push(self, v: Fraction) -> None:
        self.stack.append(v)

    def pop(self) -> Fraction:
        return self.stack.pop()

    @property
    def st0(self) -> Fraction:
        return self.stack[-1]

    def binary(self, op: str, rhs: Fraction) -> None:
        lhs = self.pop()
        if op == 'mul':
            self.push(lhs * rhs)
        elif op == 'div':
            self.push(lhs / rhs)
        elif op == 'divr':
            self.push(rhs / lhs)
        elif op == 'add':
            self.push(lhs + rhs)
        elif op == 'sub':
            self.push(lhs - rhs)
        elif op == 'subr':
            self.push(rhs - lhs)
        else:  # pragma: no cover
            raise AssertionError(op)


# ---------------------------------------------------------------------------------------------
# Classification results
# ---------------------------------------------------------------------------------------------

@dataclass
class Row:
    kind: str
    addr: int
    aux: int
    size: int
    count: int
    value: int
    comment: str


@dataclass
class Entry:
    slot: int
    fn: int
    kind: str = 'Unhandled'
    rows: list[Row] = field(default_factory=list)
    detail: str = ''


class Unhandled(Exception):
    pass


# ---------------------------------------------------------------------------------------------
# Interpreter
# ---------------------------------------------------------------------------------------------

REGS = ('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi')

#! 8/16-bit register halves, tracked separately so a partially written register stays unknown.
SUBREGS = {
    'ax': ('eax', 16), 'al': ('eax', 8), 'ah': ('eax', 8),
    'bx': ('ebx', 16), 'bl': ('ebx', 8), 'bh': ('ebx', 8),
    'cx': ('ecx', 16), 'cl': ('ecx', 8), 'ch': ('ecx', 8),
    'dx': ('edx', 16), 'dl': ('edx', 8), 'dh': ('edx', 8),
}
STACK_BASE = 0x70000000
UNK = None


@dataclass
class StoreEvent:
    addr: int
    size: int
    data: bytes | None    # None for block events
    is_float: bool = False
    block_value: int = 0  # for `rep stosd`
    stride: int = 0
    count: int = 0


class Interp:
    """Executes one initializer body concretely.

    Only the instruction subset the table actually uses is modelled; every deviation raises
    `Unhandled`, which rolls the writes back so a partially understood body never leaks values.
    """

    def __init__(self, image: Image, mem: Memory, md):
        self.image = image
        self.mem = mem
        self.md = md

    # -- run ---------------------------------------------------------------------------------
    def run(self, body) -> list[StoreEvent]:
        self.regs: dict[str, int | None] = {r: UNK for r in REGS}
        self.subregs: dict[str, int | None] = {}
        self.regs['esp'] = STACK_BASE
        self.flags = {'zf': False, 'sf': False, 'of': False, 'cf': False}
        self.x87 = X87()
        self.events: list[StoreEvent] = []
        self.stores = 0

        pc = 0
        steps = 0
        while pc < len(body):
            ins = body[pc]
            steps += 1
            if steps > 200_000:
                raise Unhandled('step budget exceeded')
            target = self.step(ins)
            if target is None:
                pc += 1
            elif target == 'ret':
                return self.events
            else:
                pc = target
        raise Unhandled('body has no terminator')

    # -- helpers -----------------------------------------------------------------------------
    def reg(self, name: str) -> int | None:
        if name in SUBREGS:
            return self.subregs.get(name)
        return self.regs.get(name, UNK)

    def set_reg(self, name: str, value: int | None) -> None:
        if name in SUBREGS:
            self.subregs[name] = value if value is None else (value & ((1 << SUBREGS[name][1]) - 1))
            return
        if name not in self.regs:
            raise Unhandled(f'register {name}')
        self.regs[name] = None if value is None else (value & 0xFFFFFFFF)
        # A full-register write also defines its halves (they inherit the low bits).
        for sub, (parent, bits) in SUBREGS.items():
            if parent == name:
                self.subregs[sub] = None if value is None else (value & ((1 << bits) - 1))

    def ea(self, ins, op) -> int:
        m = op.mem
        addr = m.disp
        if m.base:
            base = self.reg(ins.reg_name(m.base))
            if base is None:
                raise Unhandled('address from unknown base register')
            addr += base
        if m.index:
            index = self.reg(ins.reg_name(m.index))
            if index is None:
                raise Unhandled('address from unknown index register')
            addr += index * (m.scale or 1)
        return addr & 0xFFFFFFFF

    def operand_value(self, ins, op) -> int | None:
        if op.type == capstone.CS_OP_IMM:
            return op.imm & 0xFFFFFFFF
        if op.type == capstone.CS_OP_REG:
            return self.reg(ins.reg_name(op.reg))
        raise Unhandled(f'operand {ins.op_str}')

    def load(self, addr: int, size: int) -> bytes:
        data = self.mem.read(addr, size)
        if data is None:
            raise Unhandled(f'read of unknown memory at 0x{addr:X}')
        return data

    def store(self, addr: int, data: bytes, is_float: bool = False) -> None:
        if not (DATA_START <= addr and addr + len(data) <= DATA_END):
            if STACK_BASE - 0x10000 <= addr < STACK_BASE + 0x10000:
                # Local scratch (`mov [esp], eax` before a `fild`): modelled, but not a global.
                self.mem.write(addr, data)
                return
            raise Unhandled(f'store outside .data at 0x{addr:X}')
        self.mem.write(addr, data)
        # A dword inside the executable image is an address, which has no meaning natively.
        for i in range(0, len(data) - 3, 4):
            v = int.from_bytes(data[i:i + 4], 'little')
            if POINTER_START <= v < self.image.data_end and v % 4 == 0:
                raise Unhandled(f'stores the address 0x{v:X} at 0x{addr + i:X}')
        self.events.append(StoreEvent(addr, len(data), data, is_float))
        self.stores += 1

    def set_flags(self, result: int, bits: int = 32) -> None:
        mask = (1 << bits) - 1
        sign = (result >> (bits - 1)) & 1
        result &= mask
        self.flags['zf'] = result == 0
        self.flags['sf'] = bool(sign)

    def cond(self, mnemonic: str) -> bool:
        f = self.flags
        table = {
            'je': f['zf'], 'jz': f['zf'],
            'jne': not f['zf'], 'jnz': not f['zf'],
            'js': f['sf'], 'jns': not f['sf'],
            'jl': f['sf'] != f['of'], 'jnge': f['sf'] != f['of'],
            'jge': f['sf'] == f['of'], 'jnl': f['sf'] == f['of'],
            'jle': f['zf'] or f['sf'] != f['of'],
            'jg': not f['zf'] and f['sf'] == f['of'],
            'jb': f['cf'], 'jae': not f['cf'],
        }
        if mnemonic not in table:
            raise Unhandled(f'branch {mnemonic}')
        return table[mnemonic]

    # -- one instruction ---------------------------------------------------------------------
    def step(self, ins):  # noqa: C901 - a dispatch table reads worse here
        m = ins.mnemonic
        ops = ins.operands

        if m == 'ret':
            return 'ret'
        if m == 'nop' or m == 'leave' or m == 'int3':
            if m == 'leave':
                self.regs['esp'] = self.regs['ebp']
                self.regs['ebp'] = UNK
            return None

        if m == 'jmp':
            if ops[0].type != capstone.CS_OP_IMM:
                raise Unhandled('indirect jump')
            return self.index_of(ops[0].imm)

        if m.startswith('j'):
            if self.cond(m):
                return self.index_of(ops[0].imm)
            return None

        if m in ('mov', 'movzx', 'movsx'):
            dst, src = ops[0], ops[1]
            if dst.type == capstone.CS_OP_REG:
                if src.type == capstone.CS_OP_MEM:
                    addr = self.ea(ins, src)
                    raw = self.load(addr, src.size)
                    if src.size != 4 and m == 'mov':
                        name = ins.reg_name(dst.reg)
                        if name not in SUBREGS or (1 << src.size) != (1 << SUBREGS[name][1]) // 1 and \
                                src.size * 8 != SUBREGS[name][1]:
                            raise Unhandled('partial register load')
                        self.set_reg(name, int.from_bytes(raw, 'little'))
                        return None
                    if m == 'movzx':
                        self.set_reg(ins.reg_name(dst.reg), int.from_bytes(raw, 'little'))
                    elif m == 'movsx':
                        self.set_reg(ins.reg_name(dst.reg), int.from_bytes(raw, 'little', signed=True))
                    else:
                        self.set_reg(ins.reg_name(dst.reg), int.from_bytes(raw, 'little'))
                else:
                    self.set_reg(ins.reg_name(dst.reg), self.operand_value(ins, src))
            elif dst.type == capstone.CS_OP_MEM:
                addr = self.ea(ins, dst)
                val = self.operand_value(ins, src)
                if val is None:
                    raise Unhandled('store of an unknown register')
                self.store(addr, int(val).to_bytes(dst.size, 'little'))
            else:
                raise Unhandled(f'mov {ins.op_str}')
            return None

        if m == 'lea':
            dst, src = ops[0], ops[1]
            name = ins.reg_name(dst.reg)
            mem = src.mem
            if (not mem.index and mem.disp == 0 and mem.base
                    and ins.reg_name(mem.base) == name and self.reg(name) is None):
                return None  # `lea eax, [eax]`: 5-byte padding, a no-op
            self.set_reg(name, self.ea(ins, src))
            return None

        if m in ('xor', 'add', 'sub', 'and', 'or'):
            dst, src = ops[0], ops[1]
            if dst.type != capstone.CS_OP_REG:
                raise Unhandled(f'{m} to memory')
            a = self.reg(ins.reg_name(dst.reg))
            b = self.operand_value(ins, src)
            if src.type == capstone.CS_OP_REG and dst.reg == src.reg and m == 'xor':
                self.set_reg(ins.reg_name(dst.reg), 0)
                self.set_flags(0)
                return None
            if a is None or b is None:
                self.set_reg(ins.reg_name(dst.reg), UNK)
                raise Unhandled(f'{m} with an unknown operand')
            if m == 'xor':
                r = a ^ b
            elif m == 'add':
                r = a + b
            elif m == 'sub':
                r = a - b
            elif m == 'and':
                r = a & b
            else:
                r = a | b
            self.set_flags(r)
            self.set_reg(ins.reg_name(dst.reg), r)
            return None

        if m in ('dec', 'inc'):
            dst = ops[0]
            a = self.reg(ins.reg_name(dst.reg))
            if a is None:
                raise Unhandled(f'{m} of an unknown register')
            r = a - 1 if m == 'dec' else a + 1
            self.set_flags(r)
            self.set_reg(ins.reg_name(dst.reg), r)
            return None

        if m == 'cmp':
            a = self.operand_value(ins, ops[0])
            b = self.operand_value(ins, ops[1])
            if a is None or b is None:
                raise Unhandled('cmp with an unknown operand')
            self.set_flags(a - b)
            return None

        if m == 'test':
            a = self.operand_value(ins, ops[0])
            b = self.operand_value(ins, ops[1])
            if a is None or b is None:
                raise Unhandled('test with an unknown operand')
            self.set_flags(a & b)
            return None

        if m == 'push':
            src = ops[0]
            val = self.operand_value(ins, src)
            esp = self.regs['esp']
            if esp is None:
                raise Unhandled('push with an unknown stack pointer')
            self.set_reg('esp', esp - 4)
            self.mem.write(esp - 4, [None] * 4 if val is None else list(int(val).to_bytes(4, 'little')))
            return None

        if m == 'pop':
            dst = ops[0]
            esp = self.regs['esp']
            if esp is None:
                raise Unhandled('pop with an unknown stack pointer')
            raw = self.mem.read(esp, 4)
            self.set_reg('esp', esp + 4)
            self.set_reg(ins.reg_name(dst.reg), None if raw is None else int.from_bytes(raw, 'little'))
            return None

        if m == 'rep stosd':
            count = self.regs['ecx']
            dest = self.regs['edi']
            value = self.regs['eax']
            if count is None or dest is None or value is None:
                raise Unhandled('rep stosd with unknown operands')
            if count == 0:
                return None
            data = int(value).to_bytes(4, 'little') * count
            if not (DATA_START <= dest and dest + len(data) <= DATA_END):
                raise Unhandled(f'rep stosd outside .data at 0x{dest:X}')
            self.mem.write(dest, data)
            if value != 0:
                for i in range(0, len(data), 4):
                    v = int.from_bytes(data[i:i + 4], 'little')
                    if POINTER_START <= v < self.image.data_end and v % 4 == 0:
                        raise Unhandled(f'rep stosd of the address 0x{v:X}')
            self.events.append(StoreEvent(dest, len(data), None, block_value=value, stride=4, count=count))
            self.stores += count
            self.set_reg('edi', dest + len(data))
            self.set_reg('ecx', 0)
            return None

        if m == 'call':
            target = ops[0]
            if target.type != capstone.CS_OP_IMM:
                raise Unhandled('indirect call')
            return self.call(target.imm)

        # -- x87 -----------------------------------------------------------------------------
        if m == 'fld':
            self.x87.push(self.x87_operand(ins, ops[0]))
            return None
        if m == 'fild':
            op = ops[0]
            if op.type != capstone.CS_OP_MEM:
                raise Unhandled('fild of a register')
            raw = self.load(self.ea(ins, op), op.size)
            self.x87.push(Fraction(int.from_bytes(raw, 'little', signed=True)))
            return None
        if m in ('fmul', 'fdiv', 'fdivr', 'fadd', 'fsub', 'fsubr'):
            if ops[0].type == capstone.CS_OP_REG:
                raise Unhandled(f'{m} of a register')
            self.x87.binary({'fmul': 'mul', 'fdiv': 'div', 'fdivr': 'divr',
                             'fadd': 'add', 'fsub': 'sub', 'fsubr': 'subr'}[m],
                            self.x87_operand(ins, ops[0]))
            return None
        if m == 'fchs':
            self.x87.push(-self.x87.pop())
            return None
        if m == 'fabs':
            self.x87.push(abs(self.x87.pop()))
            return None
        if m == 'fcos':
            v = self.x87.st0
            if v != 0:
                # The x87 `fcos` approximation is not the correctly rounded cosine; do not guess.
                raise Unhandled('fcos of a non-zero value')
            self.x87.pop()
            self.x87.push(Fraction(1))
            return None
        if m == 'fstp':
            op = ops[0]
            if op.type != capstone.CS_OP_MEM:
                raise Unhandled('fstp to a register')
            bits = self.float_bits(self.x87.pop(), op.size)
            self.store(self.ea(ins, op), bits, is_float=op.size == 4)
            return None
        if m == 'fst':
            op = ops[0]
            if op.type != capstone.CS_OP_MEM:
                raise Unhandled('fst to a register')
            bits = self.float_bits(self.x87.st0, op.size)
            self.store(self.ea(ins, op), bits)
            return None

        raise Unhandled(f'instruction {m} {ins.op_str}')

    def x87_operand(self, ins, op) -> Fraction:
        if op.type != capstone.CS_OP_MEM:
            raise Unhandled('x87 register operand')
        raw = self.load(self.ea(ins, op), op.size)
        if op.size == 4:
            val = f32_to_fraction(int.from_bytes(raw, 'little'))
        elif op.size == 8:
            val = f64_to_fraction(int.from_bytes(raw, 'little'))
        else:
            raise Unhandled(f'x87 operand of {op.size} bytes')
        if val is None:
            raise Unhandled('x87 non-finite operand')
        return val

    def float_bits(self, value: Fraction, size: int) -> bytes:
        if size == 4:
            bits = f32_bits(value)
            if bits is None:
                raise Unhandled('float32 overflow')
            return bits.to_bytes(4, 'little')
        if size == 8:
            try:
                return struct.pack('<d', float(value))
            except OverflowError:
                raise Unhandled('float64 overflow') from None
        raise Unhandled(f'float store of {size} bytes')

    def call(self, target: int):
        if target == FN_ATEXIT:
            self.regs['eax'] = UNK
            return None
        if target == FN_FTOL:
            value = self.x87.pop()
            if value.denominator != 1:
                raise Unhandled(f'_ftol of the non-integer {value}')
            value = int(value)
            if not (-(1 << 31) <= value < (1 << 31)):
                raise Unhandled('_ftol overflow')
            self.regs['eax'] = value & 0xFFFFFFFF
            return None
        raise Unhandled(f'call to 0x{target:X}')

    def index_of(self, addr: int) -> int:
        pos = self.body_index.get(addr)
        if pos is None:
            raise Unhandled(f'branch to 0x{addr:X} outside the body')
        return pos

    def prepare(self, body, base: int) -> None:
        self.body_base = base
        self.body_index = {ins.address: i for i, ins in enumerate(body)}


# ---------------------------------------------------------------------------------------------
# Decoding helpers
# ---------------------------------------------------------------------------------------------

def make_md(image: Image, backend: str = 'auto'):
    """Decoder for initializer bodies: `capstone` when available, else `llvm-objdump`."""
    if backend in ('auto', 'capstone') and capstone is not None:
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        return md
    if backend == 'capstone':
        raise SystemExit('`--disassembler capstone` needs the `capstone` module')
    return ObjdumpDisassembler(image)


def function_body(image: Image, md, entry: int, max_bytes: int = 4096):
    """Decode an initializer body: linear, stopping at `ret` or an unconditional `jmp`."""
    code = image.read(entry, max_bytes)
    body = []
    for ins in md.disasm(code, entry):
        body.append(ins)
        if ins.mnemonic in ('ret', 'retf', 'jmp'):
            break
    return body


def mnemonic(ins) -> str:
    return f'{ins.mnemonic} {ins.op_str}'.strip()


def immediate(ins, index: int = 0) -> int | None:
    op = ins.operands[index]
    return op.imm if op.type == capstone.CS_OP_IMM else None


# ---------------------------------------------------------------------------------------------
# Structural classification
# ---------------------------------------------------------------------------------------------

PUSHABLE = ('push', 'mov')
CLEANUP = ('push', 'call', 'pop', 'add', 'ret', 'leave', 'nop')


def as_object_ctor(image, md, body) -> tuple[int, int, str] | None:
    """`mov ecx,<global>` (+ pushed arguments) followed by `call`/`jmp <ctor>`."""
    targets = []
    for i, ins in enumerate(body):
        if ins.mnemonic in ('call', 'jmp'):
            imm = immediate(ins)
            if imm is None:
                return None
            if imm in (FN_ATEXIT, FN_VECTOR_CTOR, FN_FTOL):
                continue
            targets.append(i)
    if len(targets) != 1:
        return None
    i = targets[0]
    if i == 0 or body[i - 1].mnemonic != 'mov' or not body[i - 1].op_str.startswith('ecx, '):
        return None
    this_ptr = immediate(body[i - 1], 1)
    if this_ptr is None:
        return None
    args = []
    for ins in body[:i - 1]:
        if ins.mnemonic == 'push':
            val = immediate(ins)
            if val is None:
                return None
            args.append(val)
        elif ins.mnemonic == 'mov' and immediate(ins, 1) is not None:
            continue
        else:
            return None
    for ins in body[i + 1:]:
        if ins.mnemonic not in CLEANUP:
            return None
        if ins.mnemonic == 'call' and immediate(ins) != FN_ATEXIT:
            return None
    detail = f'{mnemonic(body[i - 1])} ; {mnemonic(body[i])}'
    if args:
        detail += ' ; args ' + ', '.join(f'0x{a:X}' for a in args)
    return this_ptr, immediate(body[i]), detail


def as_vector_ctor(body) -> tuple[int, int, int, int, str] | None:
    """The `_eh_vector_constructor_iterator(base, elem, count, ctor, dtor)` call shape."""
    call_index = None
    for i, ins in enumerate(body):
        if ins.mnemonic == 'call' and immediate(ins) == FN_VECTOR_CTOR:
            call_index = i
            break
    if call_index is None:
        return None
    pushes = []
    for ins in body[:call_index]:
        if ins.mnemonic == 'push':
            val = immediate(ins)
            if val is None:
                return None
            pushes.append(val)
        elif ins.mnemonic in ('mov', 'nop', 'lea'):
            continue
        else:
            return None
    if len(pushes) != 5:
        return None
    for ins in body[call_index + 1:]:
        if ins.mnemonic not in CLEANUP:
            return None
        if ins.mnemonic == 'call' and immediate(ins) != FN_ATEXIT:
            return None
    dtor, ctor, count, elem, base = pushes
    return base, ctor, elem, count, f'ctor 0x{ctor:X} dtor 0x{dtor:X} {count} x 0x{elem:X}'


def trivial_element_ctor(image, md, ctor: int, elem_size: int) -> list[tuple[int, int, bytes]] | None:
    """Constant initialiser for one array element, or None when anything else happens.

    Returns `[(offset, size, bytes)]` for the non-zero constant fields the constructor writes.
    """
    body = function_body(image, md, ctor)
    if not body or body[-1].mnemonic != 'ret':
        return None
    base_regs: set[str] = set()
    known: dict[str, int | None] = {}
    writes: list[tuple[int, int, bytes]] = []
    for ins in body[:-1]:
        if ins.mnemonic == 'push' and ins.operands[0].type == capstone.CS_OP_REG:
            continue
        if ins.mnemonic == 'pop':
            continue
        if ins.mnemonic == 'mov' and ins.operands[0].type == capstone.CS_OP_REG:
            dst = ins.reg_name(ins.operands[0].reg)
            src = ins.operands[1]
            if src.type == capstone.CS_OP_REG and ins.reg_name(src.reg) == 'ecx':
                base_regs.add(dst)
                continue
            if src.type == capstone.CS_OP_IMM:
                known[dst] = src.imm & 0xFFFFFFFF
                continue
            return None
        if ins.mnemonic == 'xor' and ins.operands[0].type == capstone.CS_OP_REG and \
                ins.operands[0].type == ins.operands[1].type and ins.operands[0].reg == ins.operands[1].reg:
            known[ins.reg_name(ins.operands[0].reg)] = 0
            continue
        if ins.mnemonic == 'mov' and ins.operands[0].type == capstone.CS_OP_MEM:
            mem = ins.operands[0].mem
            if not mem.base or ins.reg_name(mem.base) not in base_regs or mem.index:
                return None
            disp = mem.disp
            if disp < 0 or disp + ins.operands[0].size > elem_size:
                return None
            src = ins.operands[1]
            if src.type == capstone.CS_OP_IMM:
                value = src.imm & 0xFFFFFFFF
            elif src.type == capstone.CS_OP_REG:
                name = ins.reg_name(src.reg)
                if name not in known or known[name] is None:
                    return None
                value = known[name]
            else:
                return None
            size = ins.operands[0].size
            data = int(value).to_bytes(4, 'little')[:size]
            if int.from_bytes(data, 'little') != 0:
                v4 = int.from_bytes(data.ljust(4, b'\0'), 'little')
                if POINTER_START <= v4 < image.data_end and size == 4 and v4 % 4 == 0:
                    return None
                writes.append((disp, size, data))
            continue
        return None
    return writes


# ---------------------------------------------------------------------------------------------
# One table entry
# ---------------------------------------------------------------------------------------------

def compress(events: list[StoreEvent]) -> tuple[list[Row], int]:
    """Turn the concrete store sequence into table rows.

    A write whose bytes already equal the state the native build starts from (an all-zero
    arena) is dropped: that is what makes the `memset`-style zero initialization of the retail
    executable collapse to nothing. Runs of equal payloads with a constant stride become one
    `FillPattern` row.
    """
    rows: list[Row] = []
    shadow: dict[int, int] = {}  # state the emitted rows produce, starting from zero
    dropped = 0

    def differs(addr: int, data: bytes) -> bool:
        return any(shadow.get(addr + i, 0) != b for i, b in enumerate(data))

    def commit(addr: int, data: bytes) -> None:
        for i, b in enumerate(data):
            shadow[addr + i] = b

    pending: list[StoreEvent] = []

    def flush() -> None:
        nonlocal pending
        i = 0
        while i < len(pending):
            ev = pending[i]
            j = i + 1
            stride = None
            while j < len(pending):
                step = pending[j].addr - pending[j - 1].addr
                if pending[j].data != ev.data or pending[j].size != ev.size or step <= 0:
                    break
                if stride is None:
                    stride = step
                elif step != stride:
                    break
                j += 1
            if stride is not None and j - i >= 2:
                rows.append(Row('FillPattern', ev.addr, stride, ev.size, j - i,
                                int.from_bytes(ev.data, 'little'),
                                f'pattern x{j - i} stride 0x{stride:X}'))
                for k in range(j - i):
                    commit(ev.addr + k * stride, ev.data)
                i = j
            else:
                rows.append(Row('StoreFloat' if ev.is_float else 'StoreBytes', ev.addr, 0, ev.size, 1,
                                int.from_bytes(ev.data, 'little'), ''))
                commit(ev.addr, ev.data)
                i += 1
        pending = []

    for ev in events:
        if ev.data is None:  # `rep stosd`
            flush()
            if ev.block_value == 0:
                rows.append(Row('ZeroFill', ev.addr, 0, ev.size, 1, 0, f'rep stosd {ev.count}'))
                commit(ev.addr, bytes(ev.size))
            else:
                pattern = int(ev.block_value).to_bytes(4, 'little')
                rows.append(Row('FillPattern', ev.addr, 4, 4, ev.count, ev.block_value,
                                f'rep stosd {ev.count} x 0x{ev.block_value:08X}'))
                for k in range(ev.count):
                    commit(ev.addr + k * 4, pattern)
            continue
        if not differs(ev.addr, ev.data):
            dropped += 1
            continue
        pending.append(ev)
    flush()
    return rows, dropped


def classify_entry(image: Image, md, mem: Memory, interp: Interp, entry: Entry) -> None:
    body = function_body(image, md, entry.fn)

    vector = as_vector_ctor(body)
    if vector is not None:
        base, ctor, elem, count, detail = vector
        writes = trivial_element_ctor(image, md, ctor, elem)
        entry.detail = f'_eh_vector_constructor_iterator({detail})'
        if writes is not None:
            entry.kind = 'FinalizedVector'
            for offset, size, data in writes:
                for chunk in range(0, size, MAX_PAYLOAD):
                    part = data[chunk:chunk + MAX_PAYLOAD]
                    if len(part) > MAX_PAYLOAD:
                        raise AssertionError
                    entry.rows.append(Row('FillPattern', base + offset + chunk, elem, len(part), count,
                                          int.from_bytes(part, 'little'),
                                          f'element ctor 0x{ctor:X}'))
            if not entry.rows:
                entry.kind = 'Skip'
                entry.detail += ' (element ctor only clears)'
                entry.rows = [Row('Skip', entry.fn, 0, 0, 0, 0, entry.detail)]
            return
        entry.kind = 'VectorCtor'
        entry.rows.append(Row('VectorCtor', base, ctor, elem, count, 0, entry.detail))
        return

    obj = as_object_ctor(image, md, body)
    if obj is not None:
        this_ptr, ctor, detail = obj
        entry.kind = 'ObjectCtor'
        entry.detail = detail
        entry.rows.append(Row('ObjectCtor', this_ptr, ctor, 0, 0, 0, detail))
        return

    mark = mem.mark()
    interp.prepare(body, entry.fn)
    try:
        events = interp.run(body)
    except Unhandled as e:
        mem.rollback(mark)
        entry.kind = 'Unhandled'
        entry.detail = str(e)
        entry.rows = [Row('Unhandled', entry.fn, 0, 0, 0, 0, str(e))]
        return

    rows, dropped = compress(events)
    entry.rows = rows
    if not rows:
        entry.kind = 'Skip'
        entry.detail = 'no effect (destructor registration and/or writes that are already zero)'
        if dropped:
            entry.detail = f'{dropped} write(s) already zero in the native build'
        entry.rows = [Row('Skip', entry.fn, 0, 0, 0, 0, entry.detail)]
        entry.rows = [Row('Skip', entry.fn, 0, 0, 0, 0, entry.detail)]
    else:
        entry.kind = 'Stores'
        entry.detail = f'{len(rows)} row(s)'


# ---------------------------------------------------------------------------------------------
# Emitting
# ---------------------------------------------------------------------------------------------

HEADER = r"""//! @file ExeData.generated.h
//! The executable's CRT static-initializer effects, ready to replay into `ExeGlobals`.
//!
//! GENERATED FILE - DO NOT EDIT. Regenerate with `python3 tools/native/gen_exedata.py`.
//! Source: the `__xi_a..__xi_z` table (`0x%%START%%..0x%%END%%`, %%COUNT%% entries) of `gta_sa.exe`.

#pragma once

#ifdef NOTSA_NATIVE

#include <cstdint>
#include <span>

namespace notsa::native {

//! What one row of the generated table does. The meaning of the fields depends on the kind; see
//! the comment on each enumerator.
enum class ExeDataKind : std::uint8_t {
    StoreBytes,   //!< Write the low `Size` (1..8) bytes of `Value` (little endian) at `Addr`.
    StoreFloat,   //!< Write the 32-bit float `Value` at `Addr` (result of an x87 constant chain).
    ZeroFill,     //!< Write `Size` zero bytes at `Addr`.
    FillPattern,  //!< Write the low `Size` bytes of `Value` at `Addr + i * Aux`, for `i` < `Count`.
    VectorCtor,   //!< `_eh_vector_constructor_iterator`: `Count` elements of `Size` bytes at
                  //!< `Addr` constructed by `Aux` (a constructor function in gta_sa.exe).
    ObjectCtor,   //!< Construct the object at `Addr` with `Aux` (a constructor function in
                  //!< gta_sa.exe). The pushed arguments are recorded in the table comment.
    Skip,         //!< No effect in the native build (`_atexit`-only, or the writes are already
                  //!< zero). `Addr` is the initializer's entry point, for traceability.
    Unhandled,    //!< Not applied. `Addr` is the initializer's entry point. Either
                  //!< `ApplyManualExeDataEntry` handles it, or it is a known gap listed in
                  //!< `docs/NativePort.md` (pointer stores, CRT internals, non-constant maths).
};

//! One effect of the executable's static initialization.
struct ExeDataEntry {
    ExeDataKind Kind;     //!< What the row does.
    std::uintptr_t Addr;  //!< Destination address in the executable's `.data`.
    std::uintptr_t Aux;   //!< `FillPattern` stride / `VectorCtor`+`ObjectCtor` constructor.
    std::uint32_t Size;   //!< Payload size, element size (`VectorCtor`) or byte count (`ZeroFill`).
    std::uint32_t Count;  //!< Element repetitions (`FillPattern`, `VectorCtor`).
    std::uint64_t Value;  //!< Payload bytes, little endian.
};

//! Every row, in the order the executable's `_cinit` would have run the initializers.
std::span<const ExeDataEntry> GetExeDataEntries();

//! Replay the table into `ExeGlobals`, in table order. Called once from `main`.
void ApplyExeData();

} // namespace notsa::native

#endif // NOTSA_NATIVE
"""

SOURCE = r"""//! @file ExeData.generated.cpp
//! The executable's CRT static-initializer effects, ready to replay into `ExeGlobals`.
//!
//! GENERATED FILE - DO NOT EDIT. Regenerate with `python3 tools/native/gen_exedata.py`.
//! Source: the `__xi_a..__xi_z` table (`0x%%START%%..0x%%END%%`, %%COUNT%% entries) of `gta_sa.exe`.
//! Row kinds and their fields are documented in `ExeData.generated.h`.

#ifdef NOTSA_NATIVE

#include "app/platform/native/ExeData.generated.h"

#include <array>
#include <cstddef>

#include <spdlog/spdlog.h>

#include "app/platform/native/ExeData.manual.h"
#include "app/platform/native/ExeGlobals.h"

namespace notsa::native {
namespace {

//! One table row: `kind, addr, aux, size, count, value`.
#define EXEDATA_ROW(kind, addr, aux, size, count, value) {ExeDataKind::kind, addr, aux, size, count, value}

constexpr ExeDataEntry kEntries[] = {
%%ROWS%%
};

#undef EXEDATA_ROW

//! Little-endian payload bytes of a row, independent of the host's byte order.
std::array<std::uint8_t, 8> PayloadOf(const ExeDataEntry& e) {
    std::array<std::uint8_t, 8> bytes{};
    for (std::size_t i = 0; i < bytes.size(); i++) {
        bytes[i] = static_cast<std::uint8_t>(e.Value >> (i * 8));
    }
    return bytes;
}
} // namespace

std::span<const ExeDataEntry> GetExeDataEntries() {
    return kEntries;
}

void ApplyExeData() {
    std::size_t applied{}, bytesWritten{}, skipped{}, manual{};
    std::size_t unhandled{};    //!< Unhandled rows no manual handler claimed
    std::size_t constructions{};  //!< `ObjectCtor`/`VectorCtor` rows no manual handler claimed

    for (const auto& e : GetExeDataEntries()) {
        switch (e.Kind) {
        case ExeDataKind::StoreBytes:
        case ExeDataKind::StoreFloat: {
            const auto payload = PayloadOf(e);
            ExeGlobals::Write(e.Addr, payload.data(), e.Size);
            bytesWritten += e.Size;
            applied++;
            break;
        }
        case ExeDataKind::ZeroFill:
            ExeGlobals::ZeroFill(e.Addr, e.Size);
            bytesWritten += e.Size;
            applied++;
            break;
        case ExeDataKind::FillPattern:
            for (std::uint32_t i = 0; i < e.Count; i++) {
                const auto payload = PayloadOf(e);
                ExeGlobals::Write(e.Addr + i * e.Aux, payload.data(), e.Size);
            }
            bytesWritten += static_cast<std::size_t>(e.Count) * e.Size;
            applied++;
            break;
        case ExeDataKind::ObjectCtor:
        case ExeDataKind::VectorCtor:
        case ExeDataKind::Unhandled:
            if (ApplyManualExeDataEntry(e)) {
                manual++;
            } else if (e.Kind == ExeDataKind::Unhandled) {
                unhandled++;
                spdlog::warn("ExeData: initializer 0x{:X} left unhandled", e.Addr);
            } else {
                constructions++;
            }
            break;
        case ExeDataKind::Skip:
            skipped++;
            break;
        }
    }

    spdlog::info(
        "ExeData: applied {} rows ({} bytes), {} manual, {} skipped, {} constructors not applied, {} unhandled",
        applied, bytesWritten, manual, skipped, constructions, unhandled
    );
}
} // namespace notsa::native

#endif // NOTSA_NATIVE
"""


def emit_rows(entries: list[Entry]) -> str:
    lines: list[str] = []
    for entry in entries:
        head = f'    /* 0x{entry.fn:X}'
        detail = entry.detail.replace('*/', '* /').replace('\n', ' ')
        if len(detail) > 110:
            detail = detail[:107] + '...'
        for i, row in enumerate(entry.rows):
            prefix = head + (' ' + detail if detail else '') + ' */' if i == 0 else '    /* cont. */'
            comment = ''
            if row.comment and row.kind in ('FillPattern', 'VectorCtor', 'ObjectCtor', 'Unhandled', 'Skip'):
                comment = ' // ' + row.comment.replace('\n', ' ')
                if len(comment) > 110:
                    comment = comment[:107] + '...'
            lines.append(
                f'{prefix}\n'
                f'    EXEDATA_ROW({row.kind}, 0x{row.addr:X}, 0x{row.aux:X}, {row.size}, {row.count}, 0x{row.value:X}),'
                f'{comment}'
            )
    return '\n'.join(lines)


SUMMARY_KINDS = ('StoreBytes', 'StoreFloat', 'ZeroFill', 'FillPattern', 'VectorCtor', 'ObjectCtor', 'Skip', 'Unhandled')


def generate(image: Image, out_dir: str, backend: str = 'auto') -> tuple[dict[str, str], list[Entry]]:
    md = make_md(image, backend)
    mem = Memory(image)
    interp = Interp(image, mem, md)

    count = (XI_TABLE_END - XI_TABLE_START) // 4
    slots = struct.unpack('<' + 'I' * count, image.read(XI_TABLE_START, count * 4))
    entries: list[Entry] = []
    for index, fn in enumerate(slots):
        if fn == 0:
            continue
        entry = Entry(XI_TABLE_START + index * 4, fn)
        classify_entry(image, md, mem, interp, entry)
        entries.append(entry)

    # One row per entry minimum, so the table stays traceable to the executable's table order.
    assert all(e.rows for e in entries), 'entry without rows'

    # Every write must land in the arena `ExeGlobals` owns (the executable's `.data`).
    for e in entries:
        for row in e.rows:
            if row.kind in ('StoreBytes', 'StoreFloat', 'FillPattern'):
                assert row.size <= MAX_PAYLOAD, f'payload too large in 0x{e.fn:X}'
            if row.kind in ('StoreBytes', 'StoreFloat', 'ZeroFill', 'FillPattern', 'VectorCtor'):
                assert DATA_START <= row.addr, f'store below the arena in 0x{e.fn:X}: 0x{row.addr:X}'
                assert row.addr + max(row.size, 1) <= DATA_END, f'store above the arena in 0x{e.fn:X}: 0x{row.addr:X}'
            if row.kind == 'FillPattern':
                assert row.addr + (row.count - 1) * row.aux + row.size <= DATA_END, f'pattern outside arena in 0x{e.fn:X}'

    substitutions = {
        '%%START%%': f'{XI_TABLE_START:X}',
        '%%END%%': f'{XI_TABLE_END:X}',
        '%%COUNT%%': str(len(entries)),
        '%%ROWS%%': emit_rows(entries),
    }

    def expand(template: str) -> str:
        for key, value in substitutions.items():
            template = template.replace(key, value)
        return template

    header = expand(HEADER)
    source = expand(SOURCE)

    files = {
        os.path.join(out_dir, 'ExeData.generated.h'): header,
        os.path.join(out_dir, 'ExeData.generated.cpp'): source,
    }
    return files, entries


def summarize(entries: list[Entry]) -> str:
    by_kind: dict[str, int] = {}
    rows: dict[str, int] = {}
    for e in entries:
        by_kind[e.kind] = by_kind.get(e.kind, 0) + 1
        for r in e.rows:
            rows[r.kind] = rows.get(r.kind, 0) + 1
    total_rows = sum(rows.values())
    total_bytes = sum(
        row.size * (row.count if row.kind == 'FillPattern' else 1)
        for e in entries for row in e.rows
        if row.kind in ('StoreBytes', 'StoreFloat', 'ZeroFill', 'FillPattern')
    )
    lines = [f'entries: {len(entries)}   rows: {total_rows}   bytes written: {total_bytes}']
    lines.append('per entry kind:')
    for kind in ('Stores', 'FinalizedVector', 'VectorCtor', 'ObjectCtor', 'Skip', 'Unhandled'):
        if by_kind.get(kind):
            lines.append(f'  {kind:<16} {by_kind[kind]:>5}')
    lines.append('rows per kind:')
    for kind in SUMMARY_KINDS:
        if rows.get(kind):
            lines.append(f'  {kind:<16} {rows[kind]:>5}')
    unhandled = [e for e in entries if e.kind == 'Unhandled']
    if unhandled:
        lines.append(f'unhandled ({len(unhandled)}):')
        for e in unhandled:
            lines.append(f'  0x{e.fn:X}  {e.detail}')
    vectors = [e for e in entries if e.kind == 'VectorCtor']
    if vectors:
        lines.append(f'array constructors left to manual code ({len(vectors)}):')
        for e in vectors:
            lines.append(f'  0x{e.fn:X}  base 0x{e.rows[0].addr:X}  {e.detail}')
    objects = [e for e in entries if e.kind == 'ObjectCtor']
    if objects:
        lines.append(f'object constructors ({len(objects)}):')
        for e in objects:
            lines.append(f'  0x{e.fn:X}  this 0x{e.rows[0].addr:X}  {e.detail}')
    return '\n'.join(lines)


def main() -> int:
    repo = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--exe', default=os.environ.get('GTASA_EXE'),
                        help='Retail executable (default: $GTASA_EXE)')
    parser.add_argument('--out-dir', default=os.path.join(repo, 'source', 'app', 'platform', 'native'),
                        help='Where to write ExeData.generated.{h,cpp}')
    parser.add_argument('--check', action='store_true', help='Fail if the generated files are out of date')
    parser.add_argument('--report', help='Write the per-entry classification to this file')
    parser.add_argument('--disassembler', choices=('auto', 'capstone', 'objdump'), default='auto',
                        help='Decoder to use (default: capstone if installed, else llvm-objdump)')
    parser.add_argument('-v', '--verbose', action='store_true', help='Print every entry')
    args = parser.parse_args()

    exe = args.exe
    if not exe:
        candidate = os.path.join(repo, '.local', 'gta_sa.exe')
        if os.path.exists(candidate):
            exe = candidate
    if not exe or not os.path.exists(exe):
        parser.error('no executable: pass --exe or set GTASA_EXE')

    image = Image(exe)
    files, entries = generate(image, args.out_dir, args.disassembler)

    stale = []
    for path, content in files.items():
        try:
            with open(path, 'r', encoding='utf-8') as f:
                current = f.read()
        except FileNotFoundError:
            current = None
        if current != content:
            stale.append(path)
            if not args.check:
                with open(path, 'w', encoding='utf-8', newline='\n') as f:
                    f.write(content)

    if args.report:
        with open(args.report, 'w', encoding='utf-8', newline='\n') as f:
            for e in entries:
                f.write(f'0x{e.fn:X}\t{e.kind}\t{e.detail}\n')

    if args.verbose:
        for e in entries:
            for row in e.rows:
                print(f'0x{e.fn:X} {e.kind:<16} {row.kind:<12} addr=0x{row.addr:X} aux=0x{row.aux:X} '
                      f'size={row.size} count={row.count} value=0x{row.value:X}' + (f'  # {row.comment}' if row.comment else ''))

    print(summarize(entries))
    if args.check:
        if stale:
            print('out of date: ' + ', '.join(os.path.relpath(p, repo) for p in stale), file=sys.stderr)
            return 1
        print('generated files are up to date')
    else:
        print('wrote ' + ', '.join(os.path.relpath(p, repo) for p in files))
    return 0


if __name__ == '__main__':
    sys.exit(main())

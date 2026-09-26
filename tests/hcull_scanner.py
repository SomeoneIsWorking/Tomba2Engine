#!/usr/bin/env python3
"""hcull_scanner -- the Tomba! 2 horizontal-visibility CULL SCANNER, over the authenticated images.

Split out of tests/horizontal_cull_census.py, which is the command line and the reporting. This half is
the part that knows about the binary: which images exist and are authentic, which words of them are
code, where a frame-derived constant is built or compared, and whether any bound is read from the
guest's own draw environment. It PRINTS NOTHING -- the caller reports -- so it can be imported by a
diagnostic that wants the same answers in a different shape.

The reasoning that is not obvious from the code, and that a reader should not have to re-derive:

  * FOUR IMAGE FAMILIES AT THREE RAM BASES. The resident MAIN.EXE, the 23 MODE overlays (SOP/A00..A0L)
    at 0x80108F9C, the two AREA-slot overlays (OPN/CRD) at 0x8018A000, and the three stage overlays
    (START/DEMO/GAME) at 0x80106228. A resident-only scan is incomplete by construction, and because
    every MODE overlay shares one base a bare address cannot name its image -- the CLI refuses to guess
    and asks for `--image`.

  * THE RESIDENT TEXT IS AT FILE OFFSET 0x800. The PS-X EXE header states t_addr=0x80010000 and
    t_size=0xAE800 and the file is exactly 0x800 + 0xAE800 bytes. Mapping the whole file at 0x80010000
    shifts every resident address by 0x800, and MIPS is dense enough that the shifted stream still decodes
    into plausible prologues, calls and compares -- so the mistake is invisible in the output. The three
    checks in RESIDENT_MAPPING_CHECKS pin it against addresses recorded independently of this code.

  * COVERAGE IS REACHABILITY, NOT A LINEAR SWEEP. A code image carries jump tables, filename strings and
    node tables between its functions, and a linear sweep disassembles those into false hits. The walk
    goes forward from each `addiu $sp,$sp,-N` prologue, attributes each word to the FIRST function that
    reached it, and never enqueues a branch target that is itself a prologue -- which is what keeps the
    count from exceeding the image. An unbounded walk reported 1418% coverage, which is not a number.

  * CONSTANTS ARE PROPAGATED, NOT MATCHED. `lui $t7,0xf0` reads as 240<<16 to an immediate matcher, and in
    this title that instruction is the first half of `lui $t7,0xf0; ori $t7,$t7,0xf0f0` -- a colour
    constant. 96 of the first version's 1,442 hits were that one pair. So a `lui` is only a 32-bit
    constant once the `ori`/`addiu` completing it is seen, and a report separates a value BUILT into a
    register from a value COMPARED against one, because the second is the culling predicate.

  * AN UNRESOLVABLE LOAD IS COUNTED, NOT GUESSED. A register-relative load's effective address is
    base+imm; when the base is not a constant the address is unknown, and the first version read it as
    `imm` anyway and reported a draw-environment read that did not exist. Those loads now have their own
    reported count.
"""
from __future__ import annotations

import hashlib
import json
import struct
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from capstone import CS_ARCH_MIPS, CS_MODE_LITTLE_ENDIAN, CS_MODE_MIPS32, Cs

ROOT = Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "config" / "tomba2-images.json"
OVERLAYS = ROOT / "scratch" / "bin" / "overlays"
RESIDENT = ROOT / "scratch" / "bin" / "tomba2" / "MAIN.EXE"

# The four image families and the RAM base each is LOADED at. `activateModeOverlay`
# (game/core/native_override_catalog.cpp) loads SOP/A00..A0L at kModeSlot; `activateAreaSlotOverlay`
# loads OPN/CRD at kAreaSlot; tools/disasm_overlay.py records the stage overlays at the third base.
RESIDENT_BASE = 0x80010000
# The resident image is a PS-X EXE: a 0x800-byte header, then the text. Its header states
# t_addr=0x80010000 t_size=0xAE800 and the file is exactly 0x800 + 0xAE800 bytes, so guest address A
# lives at file offset A - 0x80010000 + 0x800. Getting this wrong is SILENT: MIPS is dense enough that
# an 0x800-byte shift still decodes into plausible prologues, calls and compares, so every resident
# address in the first version of this census was wrong by 0x800 and the listings looked fine.
# `verify_resident_mapping` pins it against three independently recorded addresses.
RESIDENT_TEXT_FILE_OFFSET = 0x800
MODE_BASE = 0x80108F9C
AREA_BASE = 0x8018A000
STAGE_BASE = 0x80106228
MODE_NAMES = ["SOP"] + ["A0" + (chr(ord("0") + a) if a < 10 else chr(ord("A") + a - 10)) for a in range(22)]

# The DRAWENV the guest keeps as "current" (PutDrawEnv memcpy's 92 bytes to 0x800A59B0).
DRAW_ENV_CACHE = 0x800A59B0
DRAW_ENV_LEN = 92

# 4:3-frame-derived. 320/240 are the frame's right and bottom edges; 160/120 are the projection
# centre (OFX/OFY) and therefore the half-extents.
FRAME_CONSTANTS = (320, 240, 160, 120)

RETURN_TO_CALLER = 0x03E00008  # jr $ra
SP = 29
PROLOGUE_DECREASE = 0x8000  # sign bit of the immediate: the frame is smaller, not larger
# A Tomba! 2 body is at most a few hundred instructions; the largest recorded (FUN_80060c60) is 466
# lines. 4096 words is an order of magnitude of headroom, and crossing it means the walk escaped into
# data, which the census reports rather than silently truncating.
MAX_FUNCTION_WORDS = 4096

# Instruction-word constants. Control flow is decoded from the WORD, never from the assembler's text:
# this is both exact (a `lui` operand is printed in a form that is not its encoding field) and the
# reason the census runs in seconds instead of minutes -- no disassembler call in the walk at all.
OP_SPECIAL = 0x00
OP_REGIMM = 0x01
OP_J = 0x02
OP_JAL = 0x03
OP_BEQ = 0x04
OP_BNE = 0x05
OP_BLEZ = 0x06
OP_BGTZ = 0x07
OP_ADDI = 0x08
OP_ADDIU = 0x09
OP_SLTI = 0x0A
OP_SLTIU = 0x0B
OP_ANDI = 0x0C
OP_ORI = 0x0D
OP_XORI = 0x0E
OP_LUI = 0x0F
OP_LB = 0x20
OP_LH = 0x21
OP_LWL = 0x22
OP_LW = 0x23
OP_LBU = 0x24
OP_LHU = 0x25
OP_LWR = 0x26
OP_SB = 0x28
OP_SH = 0x29
OP_SWL = 0x2A
OP_SW = 0x2B
OP_SWR = 0x2E
FUNCT_JR = 0x08
FUNCT_JALR = 0x09
FUNCT_SLL = 0x00
FUNCT_SLT = 0x2A
FUNCT_SLTU = 0x2B
STORES = (OP_SB, OP_SH, OP_SWL, OP_SW, OP_SWR)
CONTROL_TRANSFERS = (OP_J, OP_JAL, OP_BEQ, OP_BNE, OP_BLEZ, OP_BGTZ, OP_REGIMM)
SIGNED_OPS = (OP_ADDI, OP_ADDIU, OP_SLTI, OP_SLTIU, OP_XORI)
UNSIGNED_OPS = (OP_ANDI, OP_ORI)
LOAD_OPS = (OP_LB, OP_LH, OP_LW, OP_LBU, OP_LHU)
BRANCH_OPS = (OP_BEQ, OP_BNE, OP_BLEZ, OP_BGTZ)
COND_BRANCH_REGIMM = (0x00, 0x01)  # beqz, bnez

# A MIPS body OPENS by allocating stack and CLOSES on `jr $ra` plus its delay slot. Both ends are
# required, and "allocates" means a DECREASE: `addiu $sp,$sp,-N`. The opcode/register/immediate-shape
# alone is not enough, because `addiu $sp,$sp,+N` -- the matching epilogue -- passes the same test, and
# matching on it made the census read every function's epilogue as a second function entry.
# tools/overlay_owner_map.py keeps the top-half test and survives it only because it then requires a
# complete `jr $ra` walk, which this reachability walk does not do.

# The four image families and the RAM base each is LOADED at. `activateModeOverlay`
# (game/core/native_override_catalog.cpp) loads SOP/A00..A0L at kModeSlot; `activateAreaSlotOverlay`
# loads OPN/CRD at kAreaSlot; tools/disasm_overlay.py records the stage overlays at the third base.
RESIDENT_BASE = 0x80010000
# The resident image is a PS-X EXE: a 0x800-byte header, then the text. Its header states
# t_addr=0x80010000 t_size=0xAE800 and the file is exactly 0x800 + 0xAE800 bytes, so guest address A
# lives at file offset A - 0x80010000 + 0x800. Getting this wrong is SILENT: MIPS is dense enough that
# an 0x800-byte shift still decodes into plausible prologues, calls and compares, so every resident
# address in the first version of this census was wrong by 0x800 and the listings looked fine.
# `verify_resident_mapping` pins it against three independently recorded addresses.
RESIDENT_TEXT_FILE_OFFSET = 0x800
MODE_BASE = 0x80108F9C
AREA_BASE = 0x8018A000
STAGE_BASE = 0x80106228
MODE_NAMES = ["SOP"] + ["A0" + (chr(ord("0") + a) if a < 10 else chr(ord("A") + a - 10)) for a in range(22)]

# The DRAWENV the guest keeps as "current" (PutDrawEnv memcpy's 92 bytes to 0x800A59B0).
DRAW_ENV_CACHE = 0x800A59B0
DRAW_ENV_LEN = 92

# 4:3-frame-derived. 320/240 are the frame's right and bottom edges; 160/120 are the projection
# centre (OFX/OFY) and therefore the half-extents.
FRAME_CONSTANTS = (320, 240, 160, 120)

RETURN_TO_CALLER = 0x03E00008  # jr $ra
SP = 29
PROLOGUE_DECREASE = 0x8000  # sign bit of the immediate: the frame is smaller, not larger
# A Tomba! 2 body is at most a few hundred instructions; the largest recorded (FUN_80060c60) is 466
# lines. 4096 words is an order of magnitude of headroom, and crossing it means the walk escaped into
# data, which the census reports rather than silently truncating.
MAX_FUNCTION_WORDS = 4096

# Instruction-word constants. Control flow is decoded from the WORD, never from the assembler's text:
# this is both exact (a `lui` operand is printed in a form that is not its encoding field) and the
# reason the census runs in seconds instead of minutes -- no disassembler call in the walk at all.
OP_SPECIAL = 0x00
OP_REGIMM = 0x01
OP_J = 0x02
OP_JAL = 0x03
OP_BEQ = 0x04
OP_BNE = 0x05
OP_BLEZ = 0x06
OP_BGTZ = 0x07
OP_ADDI = 0x08
OP_ADDIU = 0x09
OP_SLTI = 0x0A
OP_SLTIU = 0x0B
OP_ANDI = 0x0C
OP_ORI = 0x0D
OP_XORI = 0x0E
OP_LUI = 0x0F
OP_LB = 0x20
OP_LH = 0x21
OP_LWL = 0x22
OP_LW = 0x23
OP_LBU = 0x24
OP_LHU = 0x25
OP_LWR = 0x26
OP_SB = 0x28
OP_SH = 0x29
OP_SWL = 0x2A
OP_SW = 0x2B
OP_SWR = 0x2E
FUNCT_JR = 0x08
FUNCT_JALR = 0x09
FUNCT_SLL = 0x00
FUNCT_SLT = 0x2A
FUNCT_SLTU = 0x2B
STORES = (OP_SB, OP_SH, OP_SWL, OP_SW, OP_SWR)
CONTROL_TRANSFERS = (OP_J, OP_JAL, OP_BEQ, OP_BNE, OP_BLEZ, OP_BGTZ, OP_REGIMM)
SIGNED_OPS = (OP_ADDI, OP_ADDIU, OP_SLTI, OP_SLTIU, OP_XORI)
UNSIGNED_OPS = (OP_ANDI, OP_ORI)
LOAD_OPS = (OP_LB, OP_LH, OP_LW, OP_LBU, OP_LHU)
BRANCH_OPS = (OP_BEQ, OP_BNE, OP_BLEZ, OP_BGTZ)
COND_BRANCH_REGIMM = (0x00, 0x01)  # beqz, bnez

# A MIPS body OPENS by allocating stack and CLOSES on `jr $ra` plus its delay slot. Both ends are
# required, and "allocates" means a DECREASE: `addiu $sp,$sp,-N`. The opcode/register/immediate-shape
# alone is not enough, because `addiu $sp,$sp,+N` -- the matching epilogue -- passes the same test, and
# matching on it made the census read every function's epilogue as a second function entry.
# tools/overlay_owner_map.py keeps the top-half test and survives it only because it then requires a
# complete `jr $ra` walk, which this reachability walk does not do.
class Refusal(Exception):
    """A condition that must stop the census rather than shrink its answer."""


@dataclass(frozen=True)
class Image:
    name: str
    family: str
    base: int
    data: bytes

    @property
    def end(self) -> int:
        return self.base + len(self.data)

    @property
    def word_count(self) -> int:
        return len(self.data) // 4

    def contains(self, addr: int) -> bool:
        return self.base <= addr < self.end

    def word_at(self, addr: int) -> int:
        off = addr - self.base
        return int.from_bytes(self.data[off : off + 4], "little")


@dataclass
class Function:
    image: Image
    start: int
    offsets: set[int] = field(default_factory=set)
    ran_off_end: bool = False

    @property
    def size(self) -> int:
        return len(self.offsets)


class Index:
    """One image's words, prologues, and a bounded control-flow walk.

    Built once per image and shared by every scan, so a four-scan census decodes nothing twice.
    """

    def __init__(self, image: Image, md: Cs) -> None:
        self.image = image
        self.md = md
        self.words = [image.word_at(image.base + 4 * off) for off in range(image.word_count)]
        self.prologues = {
            off
            for off, w in enumerate(self.words)
            if (w >> 26) == OP_ADDIU
            and ((w >> 21) & 0x1F) == SP
            and ((w >> 16) & 0x1F) == SP
            and (w & 0xFFFF) >= PROLOGUE_DECREASE
        }
        self.functions: list[Function] = []
        self.attributed: set[int] = set()
        self.escaped: list[Function] = []

    # -- control flow, from the word ---------------------------------------------------------------
    # `off` is a WORD index throughout, and every address that leaves this class is a guest ADDRESS.
    def _branch_target(self, off: int, word: int) -> int | None:
        """The absolute guest address of a jump/branch at word-index `off`, or None."""
        opcode = word >> 26
        pc = self.image.base + 4 * off
        if opcode in (OP_J, OP_JAL):
            return ((word & 0x03FFFFFF) << 2) | ((pc + 4) & 0xF0000000)
        if opcode in BRANCH_OPS or (
            opcode == OP_REGIMM and ((word >> 16) & 0x1F) in COND_BRANCH_REGIMM
        ):
            disp = word & 0xFFFF
            disp -= 0x10000 if disp & 0x8000 else 0
            return pc + 4 + (disp << 2)
        return None

    def _is_terminator(self, word: int) -> bool:
        if word == RETURN_TO_CALLER:
            return True
        if word >> 26 == OP_SPECIAL and (word & 0x3F) in (FUNCT_JR, FUNCT_JALR):
            return True  # jr $ra ends a body; a register jump is an indirect tail we cannot follow
        return False

    def walk(self) -> list[Function]:
        """Reachability from every prologue, attributing each word to the first function reaching it.

        THE WALK IS BOUNDED, and that matters more than it sounds. A walk that follows every branch
        target escapes through tail calls into the NEXT function, so the same words get counted once
        per function that reached them: an unbounded version of this census reported 13,559,203 words
        "reached" out of 956,407 words in the images -- 1418% coverage, which is not a coverage
        number at all. Two rules make the number mean something:
          * a branch target that is ITSELF a prologue is not enqueued; that is another function's
            entry and its body is that function's to report;
          * a word is attributed to the FIRST function that reached it, and later walks stop at
            already-attributed words.
        Every word is then counted at most once, so reached + unreached == the image's word count
        exactly, and coverage can be stated as a fraction without hiding overlap.

        DELAY SLOTS ARE ENQUEUED EXPLICITLY. Every MIPS control transfer executes the following word
        before transferring, so the walk enqueues the delay slot itself. A `j`/`jal` then continues
        at the target; a CONDITIONAL branch continues at BOTH the target and the word after the delay
        slot; and an unconditional `j` continues at NEITHER, because nothing falls through it.
        Getting the unconditional case wrong is silent -- the walk simply reads the next function's
        body as part of this one.
        """
        for start in sorted(self.prologues):
            if start in self.attributed:
                continue  # a prologue inside a body already walked is that body's code
            fn = Function(self.image, self.image.base + 4 * start)
            stack = [start]
            while stack:
                off = stack.pop()
                if off in fn.offsets or off in self.attributed or not 0 <= off < len(self.words):
                    continue
                word = self.words[off]
                fn.offsets.add(off)
                if self._is_terminator(word):
                    continue  # jr $ra: the delay slot follows, and nothing else does
                opcode = word >> 26
                if opcode in (OP_J, OP_JAL):
                    target = self._branch_target(off, word)
                    if target is not None:
                        target_off = (target - self.image.base) // 4
                        if 0 <= target_off < len(self.words) and target_off not in self.prologues:
                            stack.append(target_off)
                    stack.append(off + 1)  # the delay slot, then nowhere else
                elif opcode in BRANCH_OPS or (
                    opcode == OP_REGIMM and ((word >> 16) & 0x1F) in COND_BRANCH_REGIMM
                ):
                    target = self._branch_target(off, word)
                    if target is not None:
                        target_off = (target - self.image.base) // 4
                        if 0 <= target_off < len(self.words) and target_off not in self.prologues:
                            stack.append(target_off)
                    stack.append(off + 1)  # delay slot
                    stack.append(off + 2)  # the not-taken path
                else:
                    stack.append(off + 1)
                if len(fn.offsets) > MAX_FUNCTION_WORDS:
                    fn.ran_off_end = True
                    break
            self.functions.append(fn)
            if fn.ran_off_end:
                self.escaped.append(fn)
            self.attributed |= fn.offsets
        return self.functions

    def mnemonic(self, off: int):
        """The disassembled instruction at word-index `off`, for display only."""
        chunk = self.image.data[off * 4 : off * 4 + 4]
        for insn in self.md.disasm(chunk, self.image.base + 4 * off, 1):
            return insn
        return None

    def text_at(self, off: int) -> str:
        insn = self.mnemonic(off)
        if insn is None:
            return f".word 0x{self.words[off]:08x}"
        return f"{insn.mnemonic} {insn.op_str}"

    def owner_of(self, addr: int) -> Function | None:
        off = (addr - self.image.base) // 4
        for fn in self.functions:
            if off in fn.offsets:
                return fn
        return None
# -- scan A: 4:3-frame-derived immediates --------------------------------------------------------
def decode_immediate(word: int) -> tuple[int, str] | None:
    """(value, form) for an instruction whose constant is an immediate, else None.

    `slt`/`sltu` (SPECIAL funct 2A/2B) compare two REGISTERS and carry no immediate. They matter
    here: a screen-edge test that loads its bound into a register and then does `sltu` is a DERIVED
    bound, and to anything reading the instruction text it is indistinguishable from a compare against
    a literal. Decoding the word is what tells the two apart.
    """
    opcode = word >> 26
    imm = word & 0xFFFF
    if opcode == OP_LUI:
        return (imm << 16, "lui (16.16 high half)")
    if opcode in SIGNED_OPS:
        value = imm - 0x10000 if imm & 0x8000 else imm
        return (value, "signed immediate")
    if opcode in UNSIGNED_OPS:
        return (imm, "zero-extended immediate")
    return None


def match_frame_constants(word: int) -> list[tuple[int, str]]:
    """The 4:3-frame-derived constants this word carries, and how it carries them.

    Every 16.16 form of a frame constant has a zero low half (320<<16 = 0x01400000 and so on), so a
    bare `lui` materialises the whole value and no paired `ori` is needed to see it.
    """
    decoded = decode_immediate(word)
    if decoded is None:
        return []
    value, form = decoded
    found = []
    for frame in FRAME_CONSTANTS:
        if value == frame:
            found.append((frame, f"{form} == {frame}"))
        elif value == frame << 16:
            found.append((value, f"{form} == {frame}<<16 (16.16 screen coordinate)"))
    return found


# -- scan A: 4:3-frame-derived constants, with the immediate decoded as a CONSTANT ----------------
# Scan A does not match immediates; it propagates CONSTANTS through a function body and reports where
# a frame-derived value is built or compared. The distinction is not academic. A matcher on raw
# immediates reports `lui $t7, 0xf0` as "240<<16" -- and in Tomba! 2 that instruction is the first
# half of `lui $t7,0xf0; ori $t7,$t7,0xf0f0`, which is the vertex colour mask 0xF0F0F0F0, not a screen
# edge. 96 of the 1442 raw-immediate hits were that one mask. An immediate matcher would have put
# every area's vertex-colour masking in a cull census, and the census would have looked thorough.
#
# So: within a walked body, track each register's known constant forward from its definition,
# invalidate it on any write, and pair `lui` with the `ori`/`addiu` that completes it. Only then is
# "this body compares a vertex against the right edge" a claim the binary supports.
#
# The two things reported:
#   BUILD  a frame-derived value is materialised into a register.
#   TEST   a frame-derived value is COMPARED against something -- the culling predicate.
# A BUILD with no TEST is the interesting anomaly: the bound is computed and then never used to cull.

FRAME_VALUES = frozenset(FRAME_CONSTANTS) | frozenset(v << 16 for v in FRAME_CONSTANTS)


def _signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def _destination(word: int) -> int | None:
    """The register an instruction WRITES, or None (stores, branches, jr)."""
    opcode = word >> 26
    if opcode == OP_SPECIAL:
        funct = word & 0x3F
        if funct in (FUNCT_JR, FUNCT_JALR):
            return None
        if funct == 0x00:  # sll (rd=0 is a nop)
            rd = (word >> 11) & 0x1F
            return rd or None
        return (word >> 11) & 0x1F  # the usual jalr-through-`rd` forms write rd
    if opcode in STORES or opcode in (OP_SW, OP_SWL, OP_SWR, OP_SB, OP_SH):
        return None
    if opcode in CONTROL_TRANSFERS:
        return None
    return (word >> 16) & 0x1F


def _constant_outcome(word: int, regs: dict[int, int]) -> tuple[int, int, int] | None:
    """(destination, value, base_register) for an instruction that computes a known constant.

    `base_register` is the register the constant was carried in, for the TEST report; it is the
    instruction's own destination for a BUILD.
    """
    opcode = word >> 26
    rs = (word >> 21) & 0x1F
    rt = (word >> 16) & 0x1F
    imm = word & 0xFFFF
    if opcode == OP_LUI:
        return (rt, imm << 16, rs)
    if opcode == OP_ADDIU or opcode == OP_ADDI:
        signed = _signed16(imm)
        if rs == 0:
            return (rt, signed, rs)
        if rs in regs:
            return (rt, (regs[rs] + signed) & 0xFFFFFFFF, rs)
        return None
    if opcode == OP_ORI:
        if rs == 0:
            return (rt, imm, rs)
        if rs in regs:
            return (rt, regs[rs] | imm, rs)
        return None
    if opcode == OP_XORI:
        if rs in regs:
            return (rt, regs[rs] ^ imm, rs)
        return None
    return None


def _compared_values(word: int, regs: dict[int, int]) -> list[tuple[str, int]]:
    """(which, value) for the operands a comparison instruction tests against a bound."""
    opcode = word >> 26
    rs = (word >> 21) & 0x1F
    rt = (word >> 16) & 0x1F
    imm = word & 0xFFFF
    if opcode in (OP_SLTI, OP_SLTIU):
        return [("immediate", _signed16(imm))]
    if opcode == OP_SPECIAL and (word & 0x3F) in (FUNCT_SLT, FUNCT_SLTU):
        # register-register: the bound is whatever the two registers hold
        out = []
        for name, reg in (("rs", rs), ("rt", rt)):
            if reg in regs:
                out.append((f"register {name}", regs[reg]))
        return out
    return []


def _form(word: int) -> str:
    opcode = word >> 26
    if opcode in (OP_SLTI, OP_SLTIU):
        return "signed" if opcode == OP_SLTI else "zero-extended"
    if opcode == OP_SPECIAL:
        return "register-register"
    return "opaque"


def scan_frame_constants(indexes: list[Index]) -> list[dict]:
    """Every place a frame-derived constant is BUILT into a register or TESTed against one.

    EXACTLY ONCE per (instruction, value). Three reporting paths used to overlap -- the immediate
    decode, the constant propagation and the `lui` pair -- and each reported the same `addiu $v0,$zero,
    320` independently, so every `li 320` came out as two sites and the census's denominator was
    wrong by a factor of two. One instruction, one site.
    """
    hits: list[dict] = []
    for index in indexes:
        for fn in index.functions:
            regs: dict[int, int] = {}
            ordered = sorted(fn.offsets)
            position = 0
            while position < len(ordered):
                off = ordered[position]
                word = index.words[off]
                addr = index.image.base + 4 * off

                # A `lui` is only a 32-bit constant once the `ori`/`addiu` that completes it is seen.
                # Reading `lui` alone is how the colour constant 0x00F0F0F0 arrives looking like 240<<16.
                if (word >> 26) == OP_LUI:
                    destination = (word >> 16) & 0x1F
                    regs.pop(destination, None)
                    pair = _lui_pair(index, ordered, position)
                    if pair is not None:
                        combined, next_position, form = pair
                        for value, how in _frame_matches(combined):
                            hits.append(
                                _hit(index, fn, addr, value, f"BUILD {how} via {form}",
                                     index.text_at(off) + " / " + index.text_at(ordered[next_position]))
                            )
                        regs[destination] = combined
                        position = next_position + 1
                        continue
                    high = (word & 0xFFFF) << 16
                    for value, how in _frame_matches(high):
                        hits.append(_hit(index, fn, addr, value, f"BUILD {how} via lone lui",
                                         index.text_at(off)))
                    regs[destination] = high
                    position += 1
                    continue

                destination = _destination(word)
                if destination is not None:
                    regs.pop(destination, None)

                outcome = _constant_outcome(word, regs)
                if outcome is not None:
                    dest, value, base = outcome
                    if value in FRAME_VALUES:
                        hits.append(
                            _hit(index, fn, addr, value,
                                 f"BUILD {value} (0x{value:X})"
                                 + (" from a register" if base else " as a literal"),
                                 index.text_at(off))
                        )
                    regs[dest] = value

                for which, value in _compared_values(word, regs):
                    if value in FRAME_VALUES:
                        hits.append(
                            _hit(index, fn, addr, value,
                                 f"TEST against {which} = {value} (0x{value:X}), "
                                 f"{_form(word)} compare",
                                 index.text_at(off))
                        )
                position += 1
    return hits


def _is_lui(word: int) -> bool:
    return (word >> 26) == OP_LUI


def _lui_pair(index: Index, ordered: list[int], position: int):
    """The `lui rt,hi` at `position` plus the `ori/addiu rt,rt,lo` that completes it, if adjacent."""
    word = index.words[ordered[position]]
    rt = (word >> 16) & 0x1F
    high = (word & 0xFFFF) << 16
    follower = ordered[position + 1] if position + 1 < len(ordered) else None
    if follower is None:
        return None
    nxt = index.words[follower]
    opcode = nxt >> 26
    if (nxt >> 16) & 0x1F != rt or (nxt >> 21) & 0x1F != rt:
        return None
    low = nxt & 0xFFFF
    if opcode == OP_ORI:
        return (high | low, position + 1, "lui+ori")
    if opcode == OP_ADDIU:
        return ((high + _signed16(low)) & 0xFFFFFFFF, position + 1, "lui+addiu")
    return None


def _frame_matches(value: int) -> list[tuple[int, str]]:
    out = []
    for frame in FRAME_CONSTANTS:
        if value == frame:
            out.append((frame, f"== {frame}"))
        elif value == frame << 16:
            out.append((value, f"== {frame}<<16 (16.16 screen coordinate)"))
    return out


def _hit(index: Index, fn: Function, addr: int, value: int, how: str, text: str) -> dict:
    return {
        "image": index.image,
        "addr": addr,
        "fn": fn.start,
        "value": value,
        "how": how,
        "text": text,
    }


# -- scan B: bounds read from the guest's own draw environment ------------------------------------
# An owner that reads its bound from the DRAWENV the guest itself published DERIVES that bound, and a
# derived bound already follows the widened window. An owner that compares against a literal cannot.
# Separating those two classes is the deliverable, so this scan has to be honest about what it can
# and cannot resolve.
#
# A MIPS load is `lw rt, imm(rs)`. When `rs` is $zero the effective address IS imm and the load is
# absolute. When `rs` holds a known constant (propagated the same way scan A propagates) the address
# is that constant plus imm. When `rs` is unknown the address CANNOT be resolved -- and the first
# version of this scan assumed it was imm anyway, so a register-relative `lw $v0,-0x39c8($v0)` whose
# base happened to be 0x800A93C8 was reported as a read of the draw environment. It is not. Such loads
# are counted in their own bucket, with the number of them, so the census can say how much of the
# question it could not answer.

def scan_draw_env_loads(indexes: list[Index]) -> list[dict]:
    """Hits (definite) plus unresolved candidates, so the scan reports its own blindness."""
    hits: list[dict] = []
    unresolved = 0
    for index in indexes:
        for fn in index.functions:
            regs = _constant_registers(index, fn)
            for off in sorted(fn.offsets):
                word = index.words[off]
                if (word >> 26) not in LOAD_OPS:
                    continue
                rs = (word >> 21) & 0x1F
                imm = word & 0xFFFF
                if rs == 0:
                    target = index.image.base + 4 * off + imm
                elif rs in regs:
                    target = (regs[rs] + imm) & 0xFFFFFFFF
                else:
                    unresolved += 1
                    continue
                if not DRAW_ENV_CACHE <= target < DRAW_ENV_CACHE + DRAW_ENV_LEN:
                    continue
                hits.append(
                    {
                        "image": index.image,
                        "addr": index.image.base + 4 * off,
                        "fn": fn.start,
                        "value": target,
                        "how": f"draw-env cache +{target - DRAW_ENV_CACHE} (absolute)"
                        if rs == 0
                        else f"draw-env cache +{target - DRAW_ENV_CACHE} (from a built base)",
                        "text": index.text_at(off),
                    }
                )
    scan_draw_env_loads.unresolved = unresolved  # type: ignore[attr-defined]
    return hits


def _constant_registers(index: Index, fn: Function) -> dict[int, int]:
    """The register constants known along one body's address order, with `lui` pairs resolved."""
    regs: dict[int, int] = {}
    ordered = sorted(fn.offsets)
    position = 0
    while position < len(ordered):
        off = ordered[position]
        word = index.words[off]
        if (word >> 26) == OP_LUI:
            destination = (word >> 16) & 0x1F
            regs.pop(destination, None)
            pair = _lui_pair(index, ordered, position)
            if pair is not None:
                combined, next_position, _ = pair
                regs[destination] = combined
                position = next_position + 1
                continue
            regs[destination] = (word & 0xFFFF) << 16
            position += 1
            continue
        destination = _destination(word)
        if destination is not None:
            regs.pop(destination, None)
        outcome = _constant_outcome(word, regs)
        if outcome is not None:
            dest, value, _ = outcome
            regs[dest] = value
        position += 1
    return regs


# -- scan C: the in-image call graph, so a candidate can be named by its callers ------------------
def scan_callers(indexes: list[Index]) -> dict[int, list[str]]:
    ranges = [(i.image.base, i.image.end, i) for i in indexes]
    callers: dict[int, list[str]] = defaultdict(list)
    for index in indexes:
        for fn in index.functions:
            for off in fn.offsets:
                word = index.words[off]
                if (word >> 26) != OP_JAL:
                    continue
                target = index._branch_target(off, word)
                if target is None:
                    continue
                for lo, hi, _ in ranges:
                    if lo <= target < hi:
                        callers[target].append(
                            f"{index.image.name}@{index.image.base + 4 * off:08X}"
                        )
                        break
    return callers


# -- scan D: who PUBLISHES the projection ---------------------------------------------------------
# The GTE screen centre (OFX CR24 / OFY CR25) and the projection plane (H CR26) are written through
# the framework's `gte_write_ctrl`, and the guest reaches it as COP2 MFC2 of a control register into
# a store, or through a libgpu/libgte leaf. This scan names the functions that STORE to those CR
# numbers so the census can point at the projection publication rather than assume it is the one
# `docs/engine_re.md` recorded.
CR_OFX, CR_OFY, CR_H = 24, 25, 26


def scan_projection_writers(indexes: list[Index]) -> list[dict]:
    """Stores whose value could be a projection control register: `sw`/`sh` of a GTE-CR read.

    Detected as a MFC2 (COP2 move control-register to GPR) whose destination register is stored
    within the next few instructions. MFC2 is opcode 0x12 with rt=rd.
    """
    hits: list[dict] = []
    for index in indexes:
        for fn in index.functions:
            ordered = sorted(fn.offsets)
            for position, off in enumerate(ordered):
                word = index.words[off]
                if (word >> 26) != 0x12 or ((word >> 21) & 0x1F) != ((word >> 11) & 0x1F):
                    continue  # not MFC2 (the rt==rd shape is what distinguishes it from CFC2)
                register = (word >> 16) & 0x1F
                cr = (word >> 11) & 0x1F
                if cr not in (CR_OFX, CR_OFY, CR_H):
                    continue
                window = ordered[position + 1 : position + 4]
                for other in window:
                    other_word = index.words[other]
                    if (other_word >> 26) not in (OP_SW, OP_SH):
                        continue
                    if ((other_word >> 16) & 0x1F) != register:
                        continue
                    hits.append(
                        {
                            "image": index.image,
                            "addr": index.image.base + 4 * off,
                            "fn": fn.start,
                            "value": cr,
                            "how": f"MFC2 CR{cr} stored at {index.image.base + 4 * other:08X}",
                            "text": index.text_at(off) + "  /  " + index.text_at(other),
                        }
                    )
                    break
    return hits


def report_bodies(indexes: list[Index], hits: list[dict]) -> None:
    """Group the TEST sites by the body that contains them, and describe each body's shape.

    A site count is not a census. 417 sites against the right edge is either four culling owners or
    eighty layout routines, and the difference is the whole deliverable. So the sites are grouped by
    the walked body they sit in, and each group is described by the shape a reader can classify
    without trusting this tool:
      * how many X (320) and Y (240) compares it makes -- a screen-edge cull tests one per corner;
      * whether the compares are unsigned (`sltiu`/`sltu`, so a negative coordinate fails the test
        and the shape is an explicit on-screen test) or signed (`slti`/`slt`, so a negative
        coordinate passes, which is what a LAYOUT bound looks like);
      * whether the bound is a literal (`sltiu $v0,$v0,0x140`) or was built into a register first,
        which is the difference between a hardcoded edge and one the game can move.
    """
    by_body: dict[tuple[str, int], list[dict]] = defaultdict(list)
    for hit in hits:
        if hit["how"].startswith("TEST"):
            by_body[(hit["image"].name, hit["fn"])].append(hit)

    print(f"== bodies containing a frame-constant TEST: {len(by_body)}")
    print("   image   body        X320  Y240  16.16  form              site(s)")
    for (name, body), sites in sorted(by_body.items(), key=lambda kv: (kv[0][0], kv[0][1])):
        x = sum(1 for s in sites if s["value"] in (320, 320 << 16))
        y = sum(1 for s in sites if s["value"] in (240, 240 << 16))
        wide = any(s["value"] > 0xFFFF for s in sites)
        forms = sorted({s["how"].split(",")[-1].strip() for s in sites})
        print(
            f"   {name:8s} {body:08X}  {x:4d}  {y:4d}  {'yes' if wide else ' no':5s}  "
            f"{'/'.join(forms):18s} {len(sites):5d}"
        )
def report_bodies(indexes: list[Index], hits: list[dict]) -> None:
    """Group the TEST sites by the body that contains them, and describe each body's shape.

    A site count is not a census. 417 sites against the right edge is either four culling owners or
    eighty layout routines, and the difference is the whole deliverable. So the sites are grouped by
    the walked body they sit in, and each group is described by the shape a reader can classify
    without trusting this tool:
      * how many X (320) and Y (240) compares it makes -- a screen-edge cull tests one per corner;
      * whether the compares are unsigned (`sltiu`/`sltu`, so a negative coordinate fails the test
        and the shape is an explicit on-screen test) or signed (`slti`/`slt`, so a negative
        coordinate passes, which is what a LAYOUT bound looks like);
      * whether the bound is a literal (`sltiu $v0,$v0,0x140`) or was built into a register first,
        which is the difference between a hardcoded edge and one the game can move.
    """
    by_body: dict[tuple[str, int], list[dict]] = defaultdict(list)
    for hit in hits:
        if hit["how"].startswith("TEST"):
            by_body[(hit["image"].name, hit["fn"])].append(hit)

    print(f"== bodies containing a frame-constant TEST: {len(by_body)}")
    print("   image   body        X320  Y240  16.16  form              site(s)")
    for (name, body), sites in sorted(by_body.items(), key=lambda kv: (kv[0][0], kv[0][1])):
        x = sum(1 for s in sites if s["value"] in (320, 320 << 16))
        y = sum(1 for s in sites if s["value"] in (240, 240 << 16))
        wide = any(s["value"] > 0xFFFF for s in sites)
        forms = sorted({s["how"].split(",")[-1].strip() for s in sites})
        print(
            f"   {name:8s} {body:08X}  {x:4d}  {y:4d}  {'yes' if wide else ' no':5s}  "
            f"{'/'.join(forms):18s} {len(sites):5d}"
        )
# -- images --------------------------------------------------------------------------------------
# Three addresses the RESIDENT image must decode to a specific instruction, each recorded
# independently of this tool, used to pin the file-offset mapping. An address that is merely "some
# code" proves nothing -- MIPS decodes at almost any offset -- so each entry names the instruction and
# why it is the expected one. `verify_resident_mapping` checks all three and reports which failed.
RESIDENT_MAPPING_CHECKS: tuple[tuple[int, str, str], ...] = (
    (
        0x800509B4,
        "addiu $sp, $sp, -0x18",
        "Engine::initDisplay's own frame. 0x18 is the frame game/scene/startup.cpp mirrors, and the "
        "projection publication lives inside it -- 0x800509CC is the InitGeom call and 0x800509D8 "
        "the SetGeomOffset one, both checked below",
    ),
    (
        0x800509D8,
        "jal 0x800846d0",
        "the same body's SetGeomOffset call, with a0=160 (0xa0) and a1=120 (0x78) loaded in its "
        "delay slot -- the screen centre the ofx/h rule is stated against",
    ),
    (
        0x8003B320,
        "addiu $sp, $sp, -0x10",
        "the quad submitter's own -16-byte frame (game/render/quad_rtpt_submit.cpp says 'ascend the "
        "real 16-byte frame')",
    ),
)


def _resident_text(path: Path, expected_size: int) -> bytes:
    """The EXE's TEXT bytes, sliced past the header, or a refusal naming what is wrong.

    The slice is not cosmetic. The first version of this census fed the WHOLE file at base
    0x80010000, so every resident address was 0x800 lower than it should be. Nothing complained:
    MIPS is dense, so the shifted stream still yielded prologues, `jal`s and `sltiu`s, and the
    listings read like code. Three checks now pin the mapping, and the loader refuses an EXE whose
    own header disagrees with the constant this file assumes.
    """
    raw = path.read_bytes()
    if raw[:8] != b"PS-X EXE":
        raise Refusal(f"{path.name} does not start with the PS-X EXE signature")
    t_addr, t_size = struct.unpack_from("<II", raw, 0x18)
    if t_addr != RESIDENT_BASE:
        raise Refusal(
            f"{path.name} header t_addr is 0x{t_addr:08X}, this census assumes "
            f"0x{RESIDENT_BASE:08X}"
        )
    if t_size + RESIDENT_TEXT_FILE_OFFSET != len(raw):
        raise Refusal(
            f"{path.name} is {len(raw)} bytes; header text 0x{t_size:X} + header "
            f"0x{RESIDENT_TEXT_FILE_OFFSET:X} = 0x{t_size + RESIDENT_TEXT_FILE_OFFSET:X}"
        )
    return raw[RESIDENT_TEXT_FILE_OFFSET:]


def load_images(directory: Path) -> list[Image]:
    """Every code image, authenticated against the manifest. A missing or altered one refuses."""
    if not MANIFEST.is_file():
        raise Refusal(f"no image manifest at {MANIFEST}; cannot state what was scanned")
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))["images"]
    wanted: list[tuple[str, str, int, Path]] = []
    wanted += [
        (f"{n}.BIN", "mode", MODE_BASE, directory / f"{n}.BIN") for n in MODE_NAMES
    ]
    wanted += [
        (f"{n}.BIN", "area", AREA_BASE, directory / f"{n}.BIN") for n in ("OPN", "CRD")
    ]
    wanted += [
        (f"{n}.BIN", "stage", STAGE_BASE, directory / f"{n}.BIN") for n in ("START", "DEMO", "GAME")
    ]

    images: list[Image] = []
    for name, family, base, path in wanted:
        key = f"BIN/{name}"
        if key not in manifest:
            raise Refusal(f"manifest has no entry for {key}")
        expected = manifest[key]
        if not path.is_file():
            raise Refusal(f"no image at {path}: re-provision with tools/tomba2_provision.py")
        data = path.read_bytes()
        if len(data) != expected["size"]:
            raise Refusal(f"{name} is {len(data)} bytes, manifest says {expected['size']}")
        if hashlib.sha256(data).hexdigest() != expected["sha256"]:
            raise Refusal(f"{name} does not match the manifest digest")
        images.append(Image(name, family, base, data))

    # The resident EXE is authenticated over the WHOLE file, then sliced to its text, so the manifest
    # still describes the file the user provisioned.
    if "MAIN.EXE" not in manifest:
        raise Refusal("manifest has no entry for MAIN.EXE")
    expected = manifest["MAIN.EXE"]
    if not RESIDENT.is_file():
        raise Refusal(f"no image at {RESIDENT}: re-provision with tools/tomba2_provision.py")
    raw = RESIDENT.read_bytes()
    if len(raw) != expected["size"]:
        raise Refusal(f"MAIN.EXE is {len(raw)} bytes, manifest says {expected['size']}")
    if hashlib.sha256(raw).hexdigest() != expected["sha256"]:
        raise Refusal("MAIN.EXE does not match the manifest digest")
    images.insert(0, Image("MAIN.EXE", "resident", RESIDENT_BASE, _resident_text(RESIDENT, len(raw))))
    return images


def verify_resident_mapping(index: Index) -> int:
    """Check the resident file-offset mapping against three recorded addresses. Returns failures."""
    failures = 0
    for addr, want, why in RESIDENT_MAPPING_CHECKS:
        if not index.image.contains(addr):
            print(f"  [FAIL] {addr:08X} is outside the resident text")
            failures += 1
            continue
        got = index.text_at((addr - index.image.base) // 4)
        ok = want in got
        failures += 0 if ok else 1
        print(f"  [{'PASS' if ok else 'FAIL'}] {addr:08X}: got {got!r}, want {want!r} -- {why}")
    return failures


def build_indexes(images: list[Image], md: Cs) -> list[Index]:
    indexes = []
    for image in images:
        index = Index(image, md)
        index.walk()
        indexes.append(index)
    return indexes

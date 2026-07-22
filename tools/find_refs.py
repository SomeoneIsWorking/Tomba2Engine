#!/usr/bin/env python3
"""find_refs.py — "who touches guest address X?" over a 2 MB PSX RAM dump (KSEG0 @ 0x80000000).

Ghidra's Reference DB misses the base+offset loads this codebase lives on (`lui rX,0x800f` +
`lbu rY,-0x810c(rX)`), and `decomp.sh` has no xref mode — so every "who writes 0x800E7EF4" question
has been answered by hand-scanning. This does it directly off the instruction stream.

    tools/find_refs.py <dump.bin> <addr> [--rw r|w|both] [--range LO HI]

Prints every load/store whose effective address resolves to <addr>, with the access width, the
direction, and the enclosing function (nearest preceding `addiu sp,sp,-N` prologue).

Register tracking is intentionally simple — `lui` and `addiu/ori` off a lui — which is exactly the
shape MIPS uses for absolute globals. It is a SCANNER, not a decompiler: confirm a hit in Ghidra
(`decomp.sh`) before acting on it.
"""
import struct
import sys

LOADS  = {0x20: ("lb", 1), 0x21: ("lh", 2), 0x23: ("lw", 4), 0x24: ("lbu", 1), 0x25: ("lhu", 2)}
STORES = {0x28: ("sb", 1), 0x29: ("sh", 2), 0x2b: ("sw", 4)}


def main() -> int:
    args = sys.argv[1:]
    if len(args) < 2:
        print(__doc__)
        return 1
    dump, target = args[0], int(args[1], 16)
    rw = "both"
    lo, hi = 0x80000000, 0x80200000
    i = 2
    while i < len(args):
        if args[i] == "--rw":
            rw = args[i + 1]; i += 2
        elif args[i] == "--range":
            lo, hi = int(args[i + 1], 16), int(args[i + 2], 16); i += 3
        else:
            i += 1

    data = open(dump, "rb").read()
    base = 0x80000000

    # Pass 1: function starts, so a hit can be attributed to a caller-visible symbol.
    starts = []
    for off in range(0, len(data) - 4, 4):
        w = struct.unpack_from("<I", data, off)[0]
        # addiu sp, sp, -N   (opcode 0x09, rs=rt=29, negative immediate)
        if (w >> 26) == 0x09 and ((w >> 21) & 31) == 29 and ((w >> 16) & 31) == 29 and (w & 0x8000):
            starts.append(base + off)

    def owner(addr: int) -> int:
        best = 0
        loidx, hiidx = 0, len(starts) - 1
        while loidx <= hiidx:
            mid = (loidx + hiidx) // 2
            if starts[mid] <= addr:
                best = starts[mid]; loidx = mid + 1
            else:
                hiidx = mid - 1
        return best

    reg = [None] * 32
    hits = 0
    for off in range(0, len(data) - 4, 4):
        addr = base + off
        if addr < lo or addr >= hi:
            continue
        w = struct.unpack_from("<I", data, off)[0]
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm

        if op == 0x0F:                                  # lui
            reg[rt] = (imm << 16) & 0xFFFFFFFF
            continue
        if op == 0x09 and reg[rs] is not None and rs != 29:   # addiu off a lui
            reg[rt] = (reg[rs] + simm) & 0xFFFFFFFF
            continue

        table = LOADS if op in LOADS else (STORES if op in STORES else None)
        if table is None:
            if op == 0x09 or op == 0x0D:
                reg[rt] = None
            continue
        name, width = table[op]
        is_store = op in STORES
        if rw == "r" and is_store:
            continue
        if rw == "w" and not is_store:
            continue
        if reg[rs] is None:
            continue
        ea = (reg[rs] + simm) & 0xFFFFFFFF
        if ea <= target < ea + width:
            fn = owner(addr)
            print(f"  {addr:08x}  {name:3s} {'W' if is_store else 'R'}{width}  ea=0x{ea:08x}"
                  f"   in FUN_{fn:08X}")
            hits += 1
    print(f"[find_refs] {hits} reference(s) to 0x{target:08X} in {dump}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

#!/usr/bin/env python3
"""width_audit.py — flag native guest-memory accesses whose WIDTH/SIGNEDNESS disagrees with the guest.

WHY
---
Kanban #8 and #1 were one defect: `Behaviors::areaSeasidePerframe` read the ride/attach pointer as
`mem_r16s(0x800E7FD8)` where the guest does `lw` + `sltiu` — a 32-bit UNSIGNED test. Unattached the
slot is 0 and both readings agree, so the bug was invisible until something was held; held, the sign
flipped the branch and two separate gameplay bugs fell out. Nothing in the build catches that: it
compiles, it runs, it is byte-identical in the common case.

This audit reads every LITERAL guest address in the native tree, finds how the GUEST accesses that
same address in the real instruction stream (a RAM dump), and reports where the widths disagree.

    tools/width_audit.py [--dump scratch/bin/grab_prefix_6600.bin] [--path game/] [--quiet-ok]

Guest load/store widths are taken from the MIPS opcode, so this is ground truth, not a decompiler's
rendering — the decompiler is exactly what hid the original bug (it printed the operand as a pointer
comparison, which shows neither width nor signedness).

★ NEVER "FIX" A HIT WITHOUT CHECKING generated/ FIRST. ★
This scanner resolves an address only when a `lui` that establishes the base register is close enough
to the access. When the base was set up further back, real accesses are INVISIBLE to it — so it will
happily report "guest uses: lbu" for an address the guest also reads `lhu` two instructions later.
That is not hypothetical: it reported exactly that for the area id 0x800BF870, and acting on it
produced a WRONG edit (mem_r8 where gen does mem_r16). The corpus in generated/ has the widths
explicitly (`c->mem_r16((c->r[4] + (uint32_t)-1936))`) and is authoritative; the audit only tells you
WHERE to look.

OTHER LIMITS: an address the guest only ever forms dynamically (base register from a struct field,
$gp-relative, or computed) is reported "guest-unseen" rather than OK. A mismatch is not automatically
a bug either — a narrow read of a wider field is correct when only the low bits are wanted. This tool
ranks suspects; it does not judge.
"""
import argparse, os, re, struct, sys

# native side: mem_rN / mem_wN against a literal guest address
NATIVE_RE = re.compile(r'mem_(r|w)(8|16|16s|32)\s*\(\s*(0x8[0-9A-Fa-f]{7})u?', re.I)

# MIPS load/store opcodes -> (bits, signed, kind)
OPS = {0x20: (8, True, 'lb'), 0x24: (8, False, 'lbu'), 0x21: (16, True, 'lh'),
       0x25: (16, False, 'lhu'), 0x23: (32, False, 'lw'),
       0x28: (8, False, 'sb'), 0x29: (16, False, 'sh'), 0x2B: (32, False, 'sw')}


def s16(x):
    return struct.unpack('<h', struct.pack('<H', x))[0]


def guest_access_widths(ram, base):
    """Every (kind,bits,signed) the guest uses to touch each absolute address it forms via lui[+addiu]."""
    acc = {}
    n = len(ram)
    for off in range(0, n - 3, 4):
        w = struct.unpack_from('<I', ram, off)[0]
        if (w >> 26) != 0x0F:                      # LUI rX, hi
            continue
        regs = {(w >> 16) & 0x1F: (w & 0xFFFF) << 16}
        for k in range(1, 24):
            o2 = off + 4 * k
            if o2 + 3 >= n:
                break
            w2 = struct.unpack_from('<I', ram, o2)[0]
            op = w2 >> 26
            rs, rt, imm = (w2 >> 21) & 0x1F, (w2 >> 16) & 0x1F, s16(w2 & 0xFFFF)
            if op == 0x09 and rs in regs:                     # ADDIU rt, rs, imm -> pointer form
                regs[rt] = (regs[rs] + imm) & 0xFFFFFFFF
                continue
            if op in OPS and rs in regs:
                addr = (regs[rs] + imm) & 0xFFFFFFFF
                acc.setdefault(addr, set()).add(OPS[op])
                # DO NOT stop here. A single lui-established base is reused for MANY accesses in a row
                # (`lui v0,0x800C` then lbu/lhu/lw at several offsets). Stopping at the first one made
                # the audit MISS real accesses and report false WIDTH mismatches — it claimed the guest
                # only ever read the area id with lbu when the very next instruction pair reads it lhu.
                continue
            if op == 0x0F:                      # a new lui redefines a base; keep scanning others
                regs.pop((w2 >> 16) & 0x1F, None)
                regs[(w2 >> 16) & 0x1F] = (w2 & 0xFFFF) << 16
                continue
            # a jump/branch ends the straight-line window this base is trustworthy in
            if op in (0x02, 0x03) or (op == 0 and (w2 & 0x3F) in (0x08, 0x09)):
                break
    return acc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--dump', default='scratch/bin/grab_prefix_6600.bin')
    ap.add_argument('--path', default='game')
    ap.add_argument('--quiet-ok', action='store_true', help='only print mismatches')
    a = ap.parse_args()

    if not os.path.exists(a.dump):
        print("width_audit: no RAM dump at %s (make one with PSXPORT_PAD_DUMP_AT)" % a.dump)
        return 2
    ram = open(a.dump, 'rb').read()
    print("[width_audit] scanning guest instruction stream (%s) ..." % a.dump)
    guest = guest_access_widths(ram, 0x80000000)
    print("[width_audit] guest touches %d distinct absolute addresses via lui/addiu forms" % len(guest))

    mismatches, unseen, ok = [], 0, 0
    for root, _dirs, files in os.walk(a.path):
        for fn in files:
            if not fn.endswith(('.cpp', '.h')):
                continue
            p = os.path.join(root, fn)
            for i, line in enumerate(open(p, errors='ignore'), 1):
                # Strip line comments first: prose ABOUT an access ("the check is mem_r16(0x800BF870)")
                # is not an access, and counting it produces phantom suspects in exactly the files where
                # someone documented the correct width.
                code = line.split('//', 1)[0]
                if not code.strip():
                    continue
                for rw, wid, addr_s in NATIVE_RE.findall(code):
                    addr = int(addr_s, 16)
                    nbits = 32 if wid == '32' else (8 if wid == '8' else 16)
                    nsigned = wid.endswith('s')
                    g = guest.get(addr)
                    if not g:
                        unseen += 1
                        continue
                    # compare against LOADS for reads, STORES for writes
                    want_store = (rw == 'w')
                    rel = [(b, s, k) for (b, s, k) in g if k.startswith('s') == want_store]
                    if not rel:
                        unseen += 1
                        continue
                    kinds = sorted({k for _b, _s, k in rel})
                    if not any(b == nbits for (b, s, k) in rel):
                        # WIDTH disagreement — the class that produced kanban #8/#1. Always report.
                        mismatches.append((p, i, addr, rw, wid, kinds, 'WIDTH'))
                    elif rw == 'r':
                        same = [(b, s) for (b, s, k) in rel if b == nbits]
                        # Only flag SIGN when the guest is UNANIMOUS at this width and disagrees.
                        # An address the guest reads both ways (lh AND lhu) cannot indict either choice.
                        if same and all(s != nsigned for (b, s) in same):
                            mismatches.append((p, i, addr, rw, wid, kinds, 'SIGN'))
                        else:
                            ok += 1
                    else:
                        ok += 1

    print("\n[width_audit] %d agree, %d guest-unseen (address never formed statically), %d SUSPECT\n"
          % (ok, unseen, len(mismatches)))
    order = {'WIDTH': 0, 'SIGN': 1}
    for p, i, addr, rw, wid, kinds, why in sorted(mismatches, key=lambda x: (order[x[6]], x[0], x[1])):
        print("  %-6s %s:%d  mem_%s%s(0x%08X)  guest uses: %s" % (why, p, i, rw, wid, addr, ','.join(kinds)))
    return 1 if mismatches else 0


if __name__ == '__main__':
    sys.exit(main())

#!/usr/bin/env python3
"""genaddrs.py — resolve every ABSOLUTE guest address a recompiled body touches.

WHY THIS EXISTS. The recompiler emits MIPS lui/addiu pairs literally:

    c->r[2] = (uint32_t)32779u << 16;                 // lui  v0, 0x800B
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)-14844)); // lw   v0, -14844(v0)

so the address you actually need — 0x800AC604 — appears NOWHERE in the source. Every port has to
recover it by hand, and hand arithmetic on a negative 16-bit offset against a shifted base is exactly
the kind of thing that goes wrong quietly. It went wrong for me on 0x800998E4: I wrote 0x800AC5C4 for
0x800AC604 and 0x800AC550 for 0x800AC590, both off by 0x40, so the port read the WRONG TABLE.

The dangerous part is what caught it. `port_check` PASSED — correctly, because its store axis is
address-AGNOSTIC by design (it compares widths and order, not destinations), and every width and call
site was right. Only the SBS byte-compare found it. So a static gate cannot protect you here and this
tool is the cheap prevention: resolve the addresses mechanically instead of by hand.

    tools/genaddrs.py 800998E4              # every resolved absolute address, with its access
    tools/genaddrs.py 800998E4 --known      # also say what each one is already called in the tree

Output is one line per resolved access: the register, the base, the offset, the ABSOLUTE address, and
the access width/direction. Unresolvable bases (a register whose lui this scan did not see) are listed
SEPARATELY and counted — a base this tool could not follow is reported, never silently dropped, so a
short list can never read as a complete one.
"""
import argparse, os, re, subprocess, sys, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LUI_RE = re.compile(r'c->r\[(\d+)\] = \(uint32_t\)(\d+)u << 16;')
# c->mem_rNN((c->r[R] + (uint32_t)OFF))  /  c->mem_wNN((c->r[R] + (uint32_t)OFF), ...)
ACC_RE = re.compile(r'c->mem_([rw])(8|16|32)s?\(\(c->r\[(\d+)\] \+ \(uint32_t\)(-?\d+)\)')
# a register reassigned from something that is NOT a lui kills our knowledge of it
ASSIGN_RE = re.compile(r'c->r\[(\d+)\] = ')


def find_body(addr):
    pats = [rf'void (?:gen_func_{addr})\(Core\* c\) \{{', rf'void (ov_\w+_gen_{addr})\(Core\* c\) \{{']
    for f in glob.glob(os.path.join(ROOT, 'generated/*.c')):
        txt = open(f, errors='replace').read()
        for p in pats:
            m = re.search(p + r'(.*?)\n\}', txt, re.S)
            if m:
                return os.path.basename(f), m.group(m.lastindex)
    return None, None


def known_name(addr):
    """What does the tree already call this address? Cheap grep — a NAME here is a hint, not proof."""
    try:
        out = subprocess.run(['grep', '-rhoE', rf'k[A-Za-z0-9_]*\s*=\s*{addr}u?', '--include=*.h',
                              '--include=*.cpp', 'game/'], capture_output=True, text=True,
                             cwd=ROOT, timeout=60).stdout.strip().splitlines()
        if out:
            return out[0].split('=')[0].strip()
    except Exception:
        pass
    return ""


def main():
    ap = argparse.ArgumentParser(description="resolve absolute guest addresses in a recompiled body")
    ap.add_argument("addr")
    ap.add_argument("--known", action="store_true", help="also show the tree's existing name, if any")
    a = ap.parse_args()
    addr = a.addr.upper().replace("0X", "")

    src, body = find_body(addr)
    if body is None:
        sys.exit(f"genaddrs: no recompiled body for {addr} in generated/. Refusing rather than "
                 f"printing an empty list — check the address.")

    base = {}          # reg -> lui value, when we have seen it and it has not been clobbered
    resolved, unresolved = [], []
    for line in body.splitlines():
        s = line.strip()
        m = LUI_RE.search(s)
        if m:
            base[int(m.group(1))] = int(m.group(2)) << 16
            continue
        for am in ACC_RE.finditer(s):
            rw, width, reg, off = am.group(1), int(am.group(2)), int(am.group(3)), int(am.group(4))
            if reg in base:
                resolved.append((reg, base[reg], off, base[reg] + off, rw, width))
            else:
                unresolved.append((reg, off, rw, width, s[:70]))
        # any OTHER assignment to a register kills the base we were tracking for it
        for asm in ASSIGN_RE.finditer(s):
            r = int(asm.group(1))
            if not LUI_RE.search(s) and r in base and f'c->r[{r}] = (uint32_t)' not in s:
                del base[r]

    print(f"{addr} ({src}) — {len(resolved)} resolved absolute address(es)")
    seen = set()
    for reg, b, off, abs_, rw, width in resolved:
        key = (abs_, rw, width)
        if key in seen:
            continue
        seen.add(key)
        kind = ("read " if rw == 'r' else "WRITE") + f" u{width}"
        name = f"   {known_name('0x%08X' % abs_)}" if a.known else ""
        print(f"  r{reg:<2} 0x{b:08X} {off:>+8} ->  0x{abs_:08X}   {kind}{name}")

    if unresolved:
        print(f"\n{len(unresolved)} access(es) whose BASE this scan could not follow — these are NOT "
              f"resolved and you must derive them yourself:")
        for reg, off, rw, width, ctx in unresolved[:12]:
            print(f"  r{reg:<2} {off:>+8}  ({'read' if rw=='r' else 'WRITE'} u{width})  {ctx}")
        print("  (a base is unfollowable when it comes from an argument, a load, or arithmetic — "
              "that is normal for object fields, which are reached through a0/s0 rather than a lui.)")
    else:
        print("\nEvery access in this body had a followable base. Note that object-field accesses "
              "through a0/s0 are not lui-based and so do not appear here at all — this lists GLOBALS.")


if __name__ == "__main__":
    main()

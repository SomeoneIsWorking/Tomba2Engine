

## `tools/genaddrs.py` — resolve a body's ABSOLUTE guest addresses instead of doing it by hand

The recompiler emits MIPS lui/addiu pairs literally, so the address a port actually needs appears
NOWHERE in the source:

    c->r[2] = (uint32_t)32779u << 16;                     // lui  v0, 0x800B
    c->r[2] = c->mem_r32((c->r[2] + (uint32_t)-14844));   // lw   v0, -14844(v0)   -> 0x800AC604

Every port has to recover that by hand, and hand arithmetic on a negative 16-bit offset against a
shifted base fails quietly. It failed on 0x800998E4: 0x800AC5C4 written for 0x800AC604 and 0x800AC550
for 0x800AC590, both off by 0x40, so the port classified from the WRONG TABLE.

**What makes this worth a tool rather than more care:** `port_check` PASSED that port, and correctly so
— its store axis is address-AGNOSTIC by design, comparing widths and order rather than destinations,
and every width and call site was right. A static gate structurally cannot catch a wrong address. Only
the SBS byte-compare found it, diverging in the 24 bytes of the output buffer.

    tools/genaddrs.py 800998E4            # every resolved absolute address, with access + width
    tools/genaddrs.py 800998E4 --known    # also show what the tree already calls each one

Run it BEFORE writing the named-constant block, and take the addresses from it rather than from
arithmetic. Accesses whose base it cannot follow (object fields via a0/s0, sp-relative spills) are
listed separately and COUNTED — a short list can never read as a complete one.

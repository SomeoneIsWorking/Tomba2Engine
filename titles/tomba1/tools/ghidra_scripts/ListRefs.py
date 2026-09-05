#@runtime Jython
"""List code/data references to exact Tomba! 1 guest addresses.

Targets are supplied as a comma- or whitespace-separated TOMBA1_REF_TARGETS value.  The script
prints the enclosing function for each reference and the callers of that function, when Ghidra has
materialized them.  It deliberately reports a denominator so an empty reference database cannot
look like a successful search.
"""

import os


def parse_targets(value):
    words = value.replace(",", " ").split()
    return [int(word, 16) for word in words]


def address(value):
    return currentProgram.getAddressFactory().getDefaultAddressSpace().getAddress(value)


targets = parse_targets(os.environ.get("TOMBA1_REF_TARGETS", ""))
if not targets:
    raise RuntimeError("TOMBA1_REF_TARGETS names no addresses")

manager = currentProgram.getReferenceManager()
functions = currentProgram.getFunctionManager()
total = 0
for target in targets:
    refs = list(manager.getReferencesTo(address(target)))
    print("TARGET 0x%08X refs=%d" % (target, len(refs)))
    total += len(refs)
    for ref in refs:
        source = ref.getFromAddress()
        owner = functions.getFunctionContaining(source)
        if owner is None:
            print("  FROM %s type=%s owner=<none>" % (source, ref.getReferenceType()))
            continue
        callers = list(manager.getReferencesTo(owner.getEntryPoint()))
        call_sites = []
        for caller in callers:
            caller_owner = functions.getFunctionContaining(caller.getFromAddress())
            if caller_owner is not None:
                call_sites.append("%s:%s" % (caller.getFromAddress(), caller_owner.getEntryPoint()))
        print(
            "  FROM %s type=%s owner=%s callers=%d [%s]"
            % (
                source,
                ref.getReferenceType(),
                owner.getEntryPoint(),
                len(call_sites),
                ", ".join(call_sites),
            )
        )
print("TOTAL refs=%d targets=%d" % (total, len(targets)))

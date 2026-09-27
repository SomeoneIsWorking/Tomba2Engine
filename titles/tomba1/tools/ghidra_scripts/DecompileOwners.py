#@runtime Jython
"""Decompile the FUNCTION OWNING each supplied instruction, and report the owner entry.

Why this exists: a reachability census reports instruction addresses, but only a
function body says whether a comparison is a screen-space cull or a data bound. Creating
a function at the instruction (what DecompileFunctions.py does for a bare entry) would
invent a body, so this resolves the CONTAINING function from the analysis instead and
decompiles each distinct owner exactly once.

Script args: OUT ADDR [ADDR ...]
"""

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor


args = getScriptArgs()
if len(args) < 2:
    raise RuntimeError("usage: DecompileOwners.py OUT ADDR [ADDR ...]")

output = args[0]
sites = [int(value, 16) for value in args[1:]]
space = currentProgram.getAddressFactory().getDefaultAddressSpace()
functions = currentProgram.getFunctionManager()
decompiler = DecompInterface()
decompiler.toggleCCode(True)
decompiler.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

owners = {}
orphan = []
for site in sites:
    at = space.getAddress(site)
    owner = functions.getFunctionContaining(at)
    if owner is None:
        orphan.append(site)
        continue
    entry = owner.getEntryPoint().getOffset()
    owners.setdefault(entry, []).append(site)

handle = open(output, "w")
handle.write("// sites: %d, resolved owners: %d, unresolved: %d\n"
             % (len(sites), len(owners), len(orphan)))
for entry in sorted(owners):
    function = functions.getFunctionAt(space.getAddress(entry))
    handle.write("// ==================== owner %08X (sites %s) ====================\n"
                 % (entry, " ".join("%08X" % s for s in owners[entry])))
    if function is None:
        handle.write("// NO FUNCTION\n\n")
        continue
    function.setNoReturn(False)
    result = decompiler.decompileFunction(function, 90, monitor)
    if result is not None and result.decompileCompleted():
        handle.write(result.getDecompiledFunction().getC())
        handle.write("\n")
    else:
        message = result.getErrorMessage() if result is not None else "no result"
        handle.write("// DECOMPILE FAILED: %s\n\n" % message)
handle.close()
print("OWNERS %d/%d sites in %d functions, %d unresolved -> %s"
      % (len(sites) - len(orphan), len(sites), len(owners), len(orphan), output))
if orphan:
    print("UNRESOLVED %s" % " ".join("%08X" % s for s in orphan))

#@runtime Jython
"""Decompile exact Tomba! 1 function entries to a caller-selected scratch file."""

from ghidra.app.decompiler import DecompInterface
from ghidra.util.task import ConsoleTaskMonitor


args = getScriptArgs()
if len(args) < 2:
    raise RuntimeError("usage: DecompileFunctions.py OUT ADDR [ADDR ...]")

output = args[0]
targets = [int(value, 16) for value in args[1:]]
functions = currentProgram.getFunctionManager()
space = currentProgram.getAddressFactory().getDefaultAddressSpace()
decompiler = DecompInterface()
decompiler.toggleCCode(True)
decompiler.openProgram(currentProgram)
monitor = ConsoleTaskMonitor()

count = 0
missing = []
handle = open(output, "w")
for target in sorted(targets):
    at = space.getAddress(target)
    function = functions.getFunctionAt(at)
    if function is None:
        function = createFunction(at, None)
    handle.write("// ==================== %08X ====================\n" % target)
    if function is None:
        missing.append(target)
        handle.write("// NO FUNCTION\n\n")
        continue
    function.setNoReturn(False)
    result = decompiler.decompileFunction(function, 90, monitor)
    if result is not None and result.decompileCompleted():
        handle.write(result.getDecompiledFunction().getC())
        handle.write("\n")
        count += 1
    else:
        message = result.getErrorMessage() if result is not None else "no result"
        handle.write("// DECOMPILE FAILED: %s\n\n" % message)
handle.close()
print("DECOMPILED %d/%d -> %s" % (count, len(targets), output))
if missing:
    print("MISSING %s" % " ".join("%08X" % value for value in missing))

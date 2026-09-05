#@runtime Jython
"""Write exact instructions in a half-open Tomba! 1 guest-address range."""

from ghidra.program.model.listing import CodeUnit


args = getScriptArgs()
if len(args) != 3:
    raise RuntimeError("usage: DisassembleRange.py OUT START END")

output = args[0]
start = int(args[1], 16)
end = int(args[2], 16)
if end <= start:
    raise RuntimeError("END must be greater than START")

space = currentProgram.getAddressFactory().getDefaultAddressSpace()
listing = currentProgram.getListing()
address = space.getAddress(start)
limit = space.getAddress(end)
count = 0
handle = open(output, "w")
while address.compareTo(limit) < 0:
    instruction = listing.getInstructionAt(address)
    if instruction is None:
        disassemble(address)
        instruction = listing.getInstructionAt(address)
        if instruction is None:
            handle.write("%s <not-an-instruction>\n" % address)
            address = address.add(4)
            continue
    handle.write(
        "%s  %-12s %s\n"
        % (
            address,
            instruction.getMnemonicString(),
            instruction.toString().split(None, 1)[1] if " " in instruction.toString() else "",
        )
    )
    count += 1
    address = instruction.getNext().getAddress() if instruction.getNext() is not None else address.add(4)
handle.close()
print("DISASSEMBLED %d instruction(s) in [0x%08X,0x%08X) -> %s" % (count, start, end, output))

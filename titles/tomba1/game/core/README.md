# Core engine seam

This directory owns Tomba! 1's direct `GameRuntime`, immutable executable facts, generated
`RecompRegistry` adapter, native boot prefix, three-record cooperative scheduler, and finite frame
driver. It must not inherit or include the
repository-level Tomba! 2 engine, its legacy configuration, addresses, overlays, or hook tables.

`crt0_port_trace.cpp` keeps the generated crt0 body independently comparable. The product boot path
stops before retail main's non-returning outer loop, while `Tomba1FrameDriver` resumes only the
measured task records until their cooperative yield, saves one R3000 register context per slot,
initializes each task from its measured stack top, delivers the measured VBlank event from the host,
and advances one presentation fence. `cd_native_startup.*` and `sync_native.*` bind the measured
public libcd command/read entries to the framework's synchronous disc owner. `stream_field_turn.*`
publishes the title's call-coherent field service for a continuous stream whose guest poll cannot
return to the frame driver. It advances logic-only SPU/XA exactly once before pumping the paced CD
controller; each ordinary native field cancels the equivalent pending timer turn. The
measured `CdReady` wrapper is title-owned so it consumes an already-due controller response without
entering the linked routine's `VSync(-1)` timeout clock. The linked `DMACallback` entry is likewise
title-owned: it exchanges one typed per-`Game` callback entry, returns the prior pointer, and applies
the measured DICR channel/master enable write so the shared IRQ owner can deliver completions without
inventing a guest callback table. The exact
linked libgte projection leaves bind to the shared projection recorder. The exact linked libetc
VSync remains a fatal trap.

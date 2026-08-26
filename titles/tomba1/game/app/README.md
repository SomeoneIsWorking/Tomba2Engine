# Application composition owner

The future `main.cpp` belongs here. It will install the Tomba! 1 runtime and generated registry,
construct the shared psxport machine, provision the selected executable, and enter the verified boot
path. It must not implement engine, renderer, input, audio, configuration, or RE logic.

This follows Dusklight's host-entry shape: lifecycle composition is separate from the subsystem owners
it connects. No executable is published until the generated/oracle boundary exists.

The entry point must not parse, register, or forward a 60fps or native-rendering option. Tomba! 1 has
no such compatibility modes; its only title enhancement control is the future widescreen policy.

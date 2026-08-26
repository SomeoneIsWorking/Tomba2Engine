# Tests

Hermetic title tests live here once production C++ seams exist. For the current scaffold, the shipping
Python identity verifier owns its own positive, mismatch, and refusal cases, and the isolation checker
mechanically enforces the engine boundary, source-size cap, and widescreen-only capability contract.
Its selftest drives the production scope and token predicates to the opposite answer for excluded
60fps, native-rendering, and lerp inputs.

Runtime tests must eventually exercise the production generated/oracle boundary and cite the real
`SCUS_942.36`; a test-only MIPS or widescreen reimplementation would prove nothing.

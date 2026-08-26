# Core engine seam

This directory will own Tomba! 1's direct `GameRuntime`, immutable executable facts, generated
`RecompRegistry` adapter, and native overrides as they are proven. It must not inherit or include the
repository-level Tomba! 2 engine, its legacy configuration, addresses, overlays, or hook tables.

The first source arrives only after `T1-03`/`T1-04` establish a deterministic independent-oracle and
generated boundary. Until then, the executable identity verifier is the only implemented title fact.

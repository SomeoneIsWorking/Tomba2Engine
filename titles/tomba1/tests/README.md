# Tests

Hermetic title tests complement the production C++ seams. The shipping Python identity verifier owns
its own positive, mismatch, and refusal cases, and the isolation checker
mechanically enforces the engine boundary, source-size cap, and widescreen-only capability contract.
Its selftest drives the production scope and token predicates to the opposite answer for excluded
60fps, native-rendering, and lerp inputs.

`test_provision.py` drives the shipping title-local provisioner through CLI/environment/`.env`/drop-in
precedence, CHD ambiguity, `SYSTEM.CNF` parsing, a wrong boot target, and an altered executable. Its
publication checks assert that no executable reaches `scratch/bin/tomba1/` until both disc boot
selection and all 15 identity facts agree.

`test_stream_field_turn.cpp` proves that one title stream field advances logic-only SPU/XA exactly
once before it pumps the controller, then drives the shipping callback through both refusal gates:
no direct-runtime callback layout and no active continuous stream. Both leave sector delivery
unchanged; real-disc execution remains the evidence that an active stream advances guest code.

The two `tomba1_help_*` contracts launch the real product executable with `-h` and `--help` from the
build directory, where its relative provisioned asset path is absent. Both must print usage and exit
zero, proving executable help is resolved before asset or disc discovery. The root
`tomba_launcher_selftest` additionally exercises top-level and selected `tomba1`/`tomba2` help with
an empty `PATH` and unrelated working directory, proving the launcher returns before all host and
product discovery.

`tomba1_crt0_boundary_check` exercises the production authenticated executable/overlay evidence against two independent-oracle
runs, requires 35/35 compared fields, and forces the opposite answer. These checks do not substitute
for the required bounded product launch, visible output, input response, or widescreen evidence.

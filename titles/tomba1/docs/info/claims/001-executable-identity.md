---
id: C001
kind: claim
status: holds
created: 2026-08-26
tags: executable,identity,tomba1
depends: titles/tomba1/tools/verify_executable.py#verify_bytes,titles/tomba1/executable.json
---

## Claim

The measured USA Tomba! executable named `SCUS_942.36` is 559,104 bytes with SHA-1
`81cbc79f0230aeb4252e058039f47ac95a777f5a`, entry `0x8006B58C`, text
`[0x80010000,0x80098000)`, initial SP `0x801FFFF0`, and the North America PS-X region marker.

## Evidence

`tools/verify_executable.py --selftest --executable <private SCUS_942.36>` drove the shipping verifier
over the measured file and compared 15 filename, whole-file, region, and PS-X header facts. The same
run changed one byte in a scratch copy and required the verifier to report the opposite SHA-1 answer.

## What would falsify it

The shipping verifier reports any disagreement on a fresh extraction from the selected USA disc, or
disc provenance proves that `SYSTEM.CNF` selects a different executable/build.

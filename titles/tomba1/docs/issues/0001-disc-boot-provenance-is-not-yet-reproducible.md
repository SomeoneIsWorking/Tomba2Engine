---
id: 1
title: Tomba! 1 disc boot provenance is not yet reproducible
status: open
symptom: The measured SCUS_942.36 identity is known, but no title-local tool proves which executable the selected disc boots
tags: tomba1,provisioning,identity,frontier
state_items: S001
created: 2026-08-26
updated: 2026-08-26
---

## Current evidence

C001/I001 prove 15 facts about one real extracted `SCUS_942.36`, including its whole-file SHA-1 and
PS-X header. They do not prove which disc supplied it or what that disc's `SYSTEM.CNF` selects.

## Required resolution

Implement T1-01's title-local provisioner with CLI > `PSXPORT_TOMBA1_DISC` > `.env` > one root CHD
resolution. It must extract `SYSTEM.CNF`, require its boot target to be `SCUS_942.36`, publish the
executable only after the shipping identity verifier passes, and include positive, mismatch,
ambiguity, malformed-input, and refusal cases. A real selected-disc run must then cite the boot path,
file size, and identity denominator.

## Rejected shortcut

Treating the existing private extraction or its filename as disc provenance would collapse T1-00 and
T1-01 and make a wrong-region disc indistinguishable from the selected build.

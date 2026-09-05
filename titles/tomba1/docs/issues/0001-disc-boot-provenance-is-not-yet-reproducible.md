---
id: 1
title: Tomba! 1 disc boot provenance is reproducible
status: resolved
symptom: The measured SCUS_942.36 identity is known, but no title-local tool proves which executable the selected disc boots
tags: tomba1,provisioning,identity,frontier
state_items: S001
created: 2026-08-26
updated: 2026-08-27
---

## Current evidence

C001/I001 prove 15 facts about one real extracted `SCUS_942.36`, including its whole-file SHA-1 and
PS-X header. They do not prove which disc supplied it or what that disc's `SYSTEM.CNF` selects.

C002/I002 prove the shipping publication policy hermetically. `tools/provision.py` resolves CLI,
`PSXPORT_TOMBA1_DISC`, `.env`, then exactly one root CHD; extracts and parses `SYSTEM.CNF`; refuses a
wrong boot target; and publishes only after the existing 15-fact verifier agrees. Eight cases include
both publication and no-publication outcomes.

## Resolution

On 2026-08-27 the title-local provisioner ran against the selected USA CHD. Its real `SYSTEM.CNF`
selected root `SCUS_942.36`; the extracted file was 559,104 bytes and the shipping verifier reported
15/15 agreement before publication. C003 records the falsifiable disc-provenance result.

## Rejected shortcut

Treating the existing private extraction or its filename as disc provenance would collapse T1-00 and
T1-01 and make a wrong-region disc indistinguishable from the selected build.

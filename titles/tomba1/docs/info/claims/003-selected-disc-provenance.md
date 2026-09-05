---
id: C003
kind: claim
status: holds
created: 2026-08-27
tags: tomba1,provisioning,identity
depends: tools/provision.py#provision
---

## Claim

The selected Tomba! USA disc boots root `SCUS_942.36`, and its fresh extraction agrees with all 15
tracked executable identity facts.

## Evidence

`tools/provision.py` ran against the user-supplied `Tomba! (USA).chd`. Its real `SYSTEM.CNF` selected
`SCUS_942.36`; the provisioner extracted that path and the shipping verifier reported `MATCH: 15/15`
before publishing it below `scratch/bin/tomba1/`.

## What would falsify it

A fresh run against the selected USA disc names a different `SYSTEM.CNF` boot target or disagrees on
any tracked executable fact.

---
id: C002
kind: claim
status: holds
created: 2026-08-27
tags: tomba1,provisioning,identity
depends: tools/provision.py#resolve_disc, tools/provision.py#normalize_boot_path, tools/provision.py#provision
---

## Claim

Tomba! 1's title-local provisioner publishes no executable until one selected disc declares SCUS_942.36 in SYSTEM.CNF and all 15 tracked executable facts agree

## Evidence

tests/test_provision.py drove the shipping resolve_disc/normalize_boot_path/provision functions through 8 hermetic cases: all four precedence levels, ambiguous CHDs, missing explicit input, valid and malformed SYSTEM.CNF, wrong boot target with an existing-output sentinel, agreeing publication, and altered-byte identity mismatch without publication.

## What would falsify it

a missing or ambiguous disc, malformed/wrong SYSTEM.CNF, or identity mismatch creates or replaces the published executable; or the agreeing path fails to publish the verified bytes

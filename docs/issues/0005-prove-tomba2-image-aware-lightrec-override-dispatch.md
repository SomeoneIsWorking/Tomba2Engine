---
id: 5
title: Prove Tomba! 2 image-aware Lightrec override and original-call dispatch
status: open
symptom: The planned Lightrec product has no shipping proof that resident and colliding-overlay calls select the correct native owner and can call the original guest body
state_items: S001
tags: tomba2,dynarec,lightrec,overrides,overlays
created: 2026-09-04
updated: 2026-09-05
---

## Required discriminator

After the shared per-`Core` Lightrec executor is available, exercise one reached resident native
override and one address reused by two authenticated overlays. `0x801113B4` is a grounded candidate:
binary evidence identifies different entry shapes at that numeric address in A03 and A0B.

For the resident and both overlay-image cases, prove that normal dispatch reaches the intended native
owner, a scoped original call executes the corresponding guest body through Lightrec and returns, and
a mismatched image cannot reuse either the override decision or a stale translated block. Report
call/block denominators and include a forced-negative case so silence cannot pass.

This issue is the first title discriminator, not the completion gate. The generated/static path is
already absent. Tomba! 2 still must regain its recorded free-roam frontier and pass representative
interactive gameplay without a fallback.

## Current synthetic coverage

`tomba_native_override_catalog` exercises the production declaration/binding
owner and the shipping Lightrec dispatcher: a first-image native call, a stale
generation negative that executes guest code, idempotent rebinding to the final
generation, a different image at the same address that remains on the JIT path,
and a scoped original call with nonzero translated execution and zero interpreter
fallback. Binding requires an explicit resident image token and the canonical
resident text range; only the two resident-load lifecycle points acquire that
token. It cannot adopt another active image merely because an address matches.

This preserves the boot reload contract but uses a synthetic resident image.
Exact-content authentication remains missing: the provisioner currently checks
image sizes, not a content manifest, and an image-generation token establishes
residency rather than authenticity. Overlay-specific image activation and native
declaration ownership are not wired; after generated-body removal, 55 of 254
declarations remain inactive and 199 bind to the resident image. Real resident
original-call evidence and two authenticated colliding
overlays remain required.

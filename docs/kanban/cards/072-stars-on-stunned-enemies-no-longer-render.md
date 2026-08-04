---
id: 72
title: Stars on stunned enemies no longer render
status: todo
labels: [render]
created: 2026-08-04
updated: 2026-08-04
---

**2026-08-04:** USER-REPORTED 2026-08-04 as part of 'last week broke many things'. Stars that appear over a stunned enemy no longer show. Regression window: 2026-07-28..07-31 (the user was away Tue-Fri; that is when the damage landed). UNVERIFIED which commit — do not guess, bisect. Note the framework took heavy render work in that window (widescreen, native depth, GPU present) and psxport 6dda8528/afca817d/28262159 are all in range.

---
id: 97
title: Tether producer is dispatched by TYPE byte with no queue/head gate
status: todo
labels: [render,bug]
created: 2026-08-16
updated: 2026-08-16
---

render_walk.cpp:1118 dispatches Render::tetherLineRender keyed on the node's type byte alone, for ALL THREE heads. The guest reaches FUN_80122974 only through QUEUE A (measured across all four jump tables: queue A 0x80014A70/144 entries routes types 1, 65, 129 to 0x8003BC24; queues B/C and the HEADS[1]/HEADS[2] tables route there from nowhere).

So a type-1 node appearing in HEADS[1] or HEADS[2] would get a tether vanilla never draws. NOT live in the cliff scene (HEADS[2] is empty there and HEADS[1] has no live type-1 node), which is why it has never been seen.

Found while fixing #95; deliberately not bundled with it. Fix is to gate the dispatch on route.queue == Queue::A, i.e. ask GuestQueueDispatch rather than the raw type byte.

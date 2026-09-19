// The horizontal window this port draws into, and the margin it is centred by.
//
// WHY THIS EXISTS. Six producers each spelled `gpu_vk_wide_engine(c) ? gpu_vk_wide_engine_w(c) :
// 320` for themselves, and two of them then re-derived `(that - 320) / 2` a second time. That is
// one policy -- how wide is the picture this port is composing -- written six times, and it is the
// exact shape that let Spyro's paired actor be the one producer that forgot it (Spyro issue 0124).
// Three of the six also gate how much geometry enters the frame's display list, which is a capacity
// question at 16:9 rather than a correctness one -- issue 0017 checked whether it was a guest-state
// deviation and found it is not.
//
// Every one of the six also declared the two framework entry points locally instead of including
// their owning header. The declarations live in `gpu_vk.h`; this module is the only place in the
// title that needs to name them.
#pragma once

struct Core;

namespace tomba2::wide_window {

// The horizontal window the guest's own routines are written against. Retail composes 320 px wide
// and every stock right-edge cull tests against that, whatever this port presents into.
inline constexpr int kGuestWidth = 320;

// The horizontal window THIS port draws into: the wide engine's render width when it is on, else
// the guest's own. Equal to kGuestWidth at 4:3, on the oracle, and on both SBS legs.
int drawRight(Core *core);

// How far the guest's 4:3 picture sits from the left edge of the wide canvas: half the difference.
// Zero whenever drawRight() is kGuestWidth.
int centeringMargin(Core *core);

} // namespace tomba2::wide_window

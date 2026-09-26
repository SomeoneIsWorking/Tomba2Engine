// game/render/horizontal_visibility_cull.cpp — the one place that knows where the draw width comes
// from. See horizontal_visibility_cull.h for the RE, the census and the recovered predicate.
//
// The class is pure arithmetic on purpose: it takes the draw right edge as an argument so it can be
// tested hermetically against the retail predicate with no Core, no framework and no environment. The
// single impure fact — the window this port is drawing into — is read here, once, through the module
// that already owns it (`tomba2::wide_window`, the same one game/render/submit.cpp and
// game/render/quad_rtpt_submit.cpp read). Adding a second way to ask for the width is how the six
// producers that each spelled `gpu_vk_wide_engine(c) ? gpu_vk_wide_engine_w(c) : 320` for themselves
// happened, and wide_window.h exists to end that.
#include "horizontal_visibility_cull.h"

#include "core.h"
#include "wide_window.h"

namespace tomba2::horizontal_cull {

Visibility forDrawWindow(::Core *core) {
  return Visibility(tomba2::wide_window::drawRight(core));
}

} // namespace tomba2::horizontal_cull

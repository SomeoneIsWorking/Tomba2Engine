#include "wide_window.h"

#include "gpu_vk.h"

namespace tomba2::wide_window {

int drawRight(Core *core) {
  if (gpu_vk_wide_engine(core)) {
    return gpu_vk_wide_engine_w(core);
  }
  return kGuestWidth;
}

int centeringMargin(Core *core) {
  return (drawRight(core) - kGuestWidth) / 2;
}

} // namespace tomba2::wide_window

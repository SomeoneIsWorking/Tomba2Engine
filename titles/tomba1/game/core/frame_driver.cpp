#include "frame_driver.h"

#include "context.h"
#include "core.h"
#include "execution_control.h"
#include "execution_exit.h"
#include "game.h"
#include "guest_call.h"
#include "stream_field_turn.h"
#include "tomba1_runtime.h"

#include <cstdlib>
#include <lucent/log.h>
#include <string>

namespace tomba1 {
namespace {

constexpr std::uint32_t kTaskTableBegin = 0x801FD800u;
constexpr std::uint32_t kTaskTableEnd = 0x801FD950u;
constexpr std::uint32_t kTaskStride = 0x70u;
constexpr std::uint32_t kCurrentTask = 0x1F8001D4u;

// HOW MANY DISPLAY FIELDS ONE UNBROKEN GUEST CALL MAY SPAN. This is the bound that makes resuming a
// budget exit safe, and it is a bound rather than a tolerance because "resume until it finishes" is
// indistinguishable, from outside, from a guest spin loop — the one thing the old code caught by
// aborting.
//
// It is not a guess about how long the game may legitimately run. It is measured: Tomba! 1's boot
// runs one LZ77 text/glyph decompress (`0x8003EF50`, `GuestGlyphStreamDecode`) that emits 286,720
// bytes and reached only 45,200 of them inside a single field's 564,480-cycle budget, so that one
// call spans at least SEVEN fields (see docs/issues/0007). Thirty-two fields is roughly four times
// the longest measured call, which is the headroom a diagnostic wants and far below the ~2,000
// fields a real spin would need to be mistaken for progress. A spin therefore still fails, and it
// fails with the register file and the resume address rather than as a hang.
constexpr std::uint32_t kMaxBudgetResumesPerCall = 32u;

constexpr std::uint32_t kVblankEventClass = 0xF2000003u;
constexpr std::uint32_t kVblankEventSpec = 2u;
constexpr std::uint32_t kRequestedFields = 0x1F8001EAu;
constexpr std::uint32_t kDrawSyncBeforeVblank = 0x1F8001ECu;
constexpr std::uint32_t kDrawSync = 0x8005EB54u;
constexpr std::uint32_t kResetGraph = 0x8005E694u;
constexpr std::uint32_t kDisplaySwap = 0x80016940u;
constexpr std::uint32_t kTickTaskSleeps = 0x800173B0u;

bool validTaskRecord(std::uint32_t record) {
  return record >= kTaskTableBegin && record < kTaskTableEnd && (record - kTaskTableBegin) % kTaskStride == 0u;
}

// One register as eight fixed-width hex digits. A variable-width word is a log line that a reader
// has to re-align by eye to compare two of them, which is the whole job here.
std::string hexWord(std::uint32_t value) {
  constexpr const char *const kDigits = "0123456789ABCDEF";
  std::string text(8, '0');
  for (int shift = 28; shift >= 0; shift -= 4) {
    text[static_cast<std::size_t>((28 - shift) / 4)] = kDigits[(value >> shift) & 0xFu];
  }
  return text;
}

constexpr const char *const kAbiNames[32] = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", //
    "t0",   "t1", "t2", "t3", "t4", "t5", "t6", "t7", //
    "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", //
    "t8",   "t9", "k0", "k1", "gp", "sp", "fp", "ra",
};

} // namespace

// The guest task exit report, and the one place a guest exit is described. It is a function rather
// than inline formatting because the register file is printed as a header line and a body line: one
// line per register is what a person needs, and a second call site is what would otherwise copy this
// list. Names are the PSX ABI's, so the reader does not have to know that $16 is s0.
void Tomba1FrameDriver::reportGuestExit(Core &core,
                                        std::size_t slot,
                                        const psx::cpu::ExecutionResult &result,
                                        const R3000 &exited) {
  lucent::error("tomba1-frame",
                "guest task {} exited with {} at 0x{:08X} after {} of {} budget cycles: {}",
                slot,
                psx::cpu::executionExitName(result.reason),
                result.guestPc,
                result.cycles,
                psx::cpu::ExecutionBudget::currentTurn(core).cycles,
                result.detail);
  // Three registers per line: wide enough to stay legible in a log, narrow enough that a reader
  // scanning for one register does not lose the next row's.
  //
  // The R3000 number here is the GPR INDEX, and the PSX ABI number is index-1, so the two agree for
  // every register except $zero. The name column uses the ABI numbering, which is what a MIPS
  // disassembly of the same state shows, and this report is read next to such a disassembly.
  for (std::size_t start = 0; start < 32; start += 3) {
    const std::size_t end = start + 3 < 32 ? start + 3 : 32;
    std::string line = "  guest-regs";
    for (std::size_t i = start; i < end; ++i) {
      line += std::string(" ") + kAbiNames[i] + "=0x" + hexWord(exited.r[i]);
    }
    lucent::error("tomba1-frame", "{}", line);
  }
}

Tomba1FrameDriver *Tomba1FrameDriver::active_ = nullptr;

Tomba1FrameDriver::Tomba1FrameDriver(Game &game, Tomba1Runtime &runtime) : game_(game), runtime_(runtime) {}

void Tomba1FrameDriver::registerTaskStart(Core &core, std::size_t slot, std::uint32_t entry) {
  if (slot >= tasks_.size() || entry == 0u) {
    lucent::error("tomba1-frame", "invalid task start: slot={} entry=0x{:08X}", slot, entry);
    std::abort();
  }
  if (slot == activeSlot_) {
    lucent::error("tomba1-frame", "guest tried to replace its own live task slot {}", slot);
    std::abort();
  }
  const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
  TaskSlot &task = tasks_[slot];
  task.entry = entry;
  task.resumeAddress = entry;
  task.contextReady = false;
  task.budgetResumes = 0;
  core.mem_w16(record, 2u);
}

void Tomba1FrameDriver::runTaskSlot(Core &core, std::size_t slot, std::uint16_t state, const R3000 &loopContext) {
  const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
  TaskSlot &task = tasks_[slot];
  for (int restartBudget = 0; restartBudget < 4; ++restartBudget) {
    if (state == 3u || !task.contextReady) {
      if (state == 3u) {
        task.entry = core.mem_r32(record + 12u);
      }
      const std::uint32_t entry = task.entry;
      if (entry == 0u) {
        lucent::error("tomba1-frame", "runnable task 0x{:08X} was not started by the native task owner", record);
        std::abort();
      }
      task.context = loopContext;
      task.context.r[29] = core.mem_r32(record + 8u);
      task.context.r[31] = 0xDEAD0000u;
      task.context.pc = entry;
      task.resumeAddress = entry;
      task.contextReady = true;
    }
    if (!task.contextReady) {
      lucent::error("tomba1-frame", "task {} has no saved CPU register context", slot);
      std::abort();
    }

    core.mem_w32(kCurrentTask, record);
    core.mem_w16(record, 4u);
    restartRequested_ = false;
    activeSlot_ = slot;
    active_ = this;
    static_cast<R3000 &>(core) = task.context;
    const psx::cpu::ExecutionResult result = runtime_.dispatchUntilExit(core, task.resumeAddress);
    task.context = static_cast<R3000 &>(core);
    static_cast<R3000 &>(core) = loopContext;
    active_ = nullptr;
    activeSlot_ = tasks_.size();

    if (restartRequested_) {
      state = 3u;
      continue;
    }
    if (result.reason == psx::cpu::ExecutionExitReason::CooperativeYield) {
      task.resumeAddress = result.guestPc;
      task.budgetResumes = 0;
      return;
    }
    if (result.reason == psx::cpu::ExecutionExitReason::BudgetExhausted) {
      if (resumeAcrossField(core, slot, result)) {
        return;
      }
      std::abort();
    }
    if (result.returned()) {
      lucent::error("tomba1-frame", "guest task {} returned instead of reaching its cooperative yield", slot);
      std::abort();
    }
    // A budget exit names a guest PC, and at a loop back-edge that PC is the LOOP, not the code that
    // entered it. The same PC answers "an in-flight copy is finishing" and "a copy is spinning on a
    // count nothing will reduce", and only the register file separates those two, so the whole file
    // is printed rather than a chosen few: a diagnostic that names the loop but not the callee-saved
    // registers cannot recover the loop's own counter, source cursor or terminator, and asking for
    // that as a follow-up is what turns a measured exit into a guess.
    //
    // It is `task.context`, not the Core: the slot already restored Core to the frame loop's own
    // registers, so reading the Core here would report the loop context and name the loop as the
    // caller of the exit. `task.context` is the state the exit actually left behind.
    reportGuestExit(core, slot, result, task.context);
    std::abort();
  }

  lucent::error("tomba1-frame", "task restarted more than four times in one native frame");
  std::abort();
}

// A budget exit is an ORDINARY bounded exit, not a fault: the framework's executor contract says so,
// and a guest that simply has more work than one display field is not misbehaving. On real hardware
// that call would run for as many fields as it needs and the rest of the machine would keep going, so
// the faithful response is to leave the task exactly where the exit found it and let the next field
// continue from the same PC with the same registers.
//
// Two things this must NOT become, and each has its own check:
//
//   * A silent hang. Resuming forever is indistinguishable from a spin, so the resume count is
//     bounded by `kMaxBudgetResumesPerCall` and the bound is reported with the state that hit it.
//   * A resumed task that re-enters from its ENTRY, silently restarting the call and never finishing
//     it while looking healthy. `contextReady` is what distinguishes a resume from a fresh start, and
//     the resume address must be the PC the exit actually stopped at, not the recorded entry.
//
// The record is re-armed to 2 ("started, runnable") because 4 means "running right now" and is only
// true during the dispatch above. Writing 2 is the same encoding `registerTaskStart` uses, and the
// two are told apart by `contextReady`, not by the record — which is why nothing here has to invent a
// fourth task state the guest's own table has no room for.
bool Tomba1FrameDriver::resumeAcrossField(Core &core, std::size_t slot, const psx::cpu::ExecutionResult &result) {
  const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
  TaskSlot &task = tasks_[slot];
  ++task.budgetResumes;
  if (task.budgetResumes > kMaxBudgetResumesPerCall) {
    lucent::error("tomba1-frame",
                  "guest task {} spent {} consecutive display fields without reaching its cooperative "
                  "yield; last resume 0x{:08X} after {} cycles — the task record was re-armed {} times "
                  "beyond the {} allowed for one call, so this is a guest call that makes no progress, "
                  "not one that is merely long",
                  slot,
                  task.budgetResumes,
                  result.guestPc,
                  result.cycles,
                  task.budgetResumes - 1u,
                  kMaxBudgetResumesPerCall);
    reportGuestExit(core, slot, result, task.context);
    return false;
  }
  task.resumeAddress = result.guestPc;
  core.mem_w16(record, 2u);
  lucent::info("tomba1-frame",
               "guest task {} used its whole field budget at 0x{:08X} ({} cycles) and resumes on field {} of "
               "{} for this call",
               slot,
               result.guestPc,
               result.cycles,
               task.budgetResumes,
               kMaxBudgetResumesPerCall);
  return true;
}

void Tomba1FrameDriver::startOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest task start reached outside the native frame owner");
    std::abort();
  }
  active_->registerTaskStart(*core, core->r[4], core->r[5]);
}

void Tomba1FrameDriver::runScheduledTasks(Core &core) {
  const R3000 loopContext = static_cast<R3000 &>(core);
  for (std::size_t slot = 0; slot < tasks_.size(); ++slot) {
    const std::uint32_t record = kTaskTableBegin + static_cast<std::uint32_t>(slot) * kTaskStride;
    const std::uint16_t state = core.mem_r16(record);
    if (state == 2u || state == 3u) {
      runTaskSlot(core, slot, state, loopContext);
    }
  }
}

void Tomba1FrameDriver::yieldTask(Core &core) {
  const std::uint32_t record = core.mem_r32(kCurrentTask);
  if (!validTaskRecord(record) || activeSlot_ >= tasks_.size()) {
    lucent::error("tomba1-frame", "cooperative yield has no active measured task");
    std::abort();
  }
  TaskSlot &task = tasks_[activeSlot_];
  if (!task.contextReady || record != kTaskTableBegin + static_cast<std::uint32_t>(activeSlot_) * kTaskStride) {
    lucent::error("tomba1-frame", "cooperative yield changed the active task record");
    std::abort();
  }
  core.mem_w16(record + 2u, static_cast<std::uint16_t>(core.r[4]));
  core.mem_w16(record, 1u);
  task.context = static_cast<R3000 &>(core);
  task.contextReady = true;
  psx::cpu::requestExecutionExit(core, psx::cpu::ExecutionExitReason::CooperativeYield);
}

void Tomba1FrameDriver::restartTask(Core &core) {
  const std::uint32_t record = core.mem_r32(kCurrentTask);
  if (!validTaskRecord(record) || activeSlot_ >= tasks_.size()) {
    lucent::error("tomba1-frame", "task restart has no active measured task");
    std::abort();
  }
  TaskSlot &task = tasks_[activeSlot_];
  if (!task.contextReady || record != kTaskTableBegin + static_cast<std::uint32_t>(activeSlot_) * kTaskStride) {
    lucent::error("tomba1-frame", "task restart changed the active task record");
    std::abort();
  }
  core.mem_w16(record, 3u);
  core.mem_w32(record + 12u, core.r[4]);
  restartRequested_ = true;
  psx::cpu::requestExecutionExit(core, psx::cpu::ExecutionExitReason::CooperativeYield);
}

void Tomba1FrameDriver::yieldOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest yield reached outside the native frame owner");
    std::abort();
  }
  active_->yieldTask(*core);
}

void Tomba1FrameDriver::restartOverride(Core *core) {
  if (!active_ || !core) {
    lucent::error("tomba1-frame", "guest task restart reached outside the native frame owner");
    std::abort();
  }
  active_->restartTask(*core);
}

void Tomba1FrameDriver::finishMainIteration(Core &core, std::uint32_t fields) {
  if (core.mem_r16(kDrawSyncBeforeVblank) != 0u) {
    psx::cpu::dispatchGuestToReturn1(core, kDrawSync, 0u, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }

  for (std::uint32_t field = 0; field < fields; ++field) {
    game_.hle.deliverEvent(kVblankEventClass, kVblankEventSpec);
    game_.spu_audio.frame();
  }

  if (core.mem_r16(kDrawSyncBeforeVblank) == 0u) {
    psx::cpu::dispatchGuestToReturn1(core, kResetGraph, 1u, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }

  const std::uint8_t displayState = core.mem_r8(0x1F8001CCu);
  if ((displayState < 2u || displayState == 3u) && core.mem_r16(0x1F8001F0u) < 0x4001u) {
    if (displayState == 3u) {
      core.mem_w8(0x1F8001CCu, 2u);
    }
    psx::cpu::dispatchGuestToReturn0(core, kDisplaySwap, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
    psx::cpu::dispatchGuestToReturn0(core, kTickTaskSleeps, psx::cpu::ExecutionBudget::currentTurn(core), __func__);
  }
}

void Tomba1FrameDriver::stepFrame(Core &core, std::uint32_t frame) {
  if (core.game != &game_ || frame != completedFrames_) {
    lucent::error("tomba1-frame",
                  "invalid frame request: game-bound={} requested={} completed={}",
                  core.game == &game_,
                  frame,
                  completedFrames_);
    std::abort();
  }

  game_.timing.logicFrame = frame;
  game_.timing.frameTick();
  core.rsub.otAttr.beginLogicFrame(frame);
  game_.pad.serviceFrame();

  if (!booted_) {
    active_ = this;
    runtime_.bootInit(core);
    active_ = nullptr;
    booted_ = true;
  }

  core.mem_w16(0x1F8001E8u, 0u);
  if (core.mem_r16(0x1F8001F0u) < 0x4001u) {
    const std::int16_t parity = static_cast<std::int16_t>(core.mem_r16(0x1F8001F4u));
    core.mem_w32(0x8009C8A8u, static_cast<std::uint32_t>(parity * 0x780 + 0x800A1890));
  }

  runScheduledTasks(core);

  const std::uint32_t fields = core.mem_r16(kRequestedFields);
  if (fields == 0u || fields > 4u) {
    lucent::error("tomba1-frame", "retail frame requested {} display fields; expected 1..4", fields);
    std::abort();
  }
  finishMainIteration(core, fields);
  // The aspect is a live player setting, and 0x80016B04 states the guest's projection centre exactly
  // once per image, so a run that changed aspect after boot would otherwise widen only the host
  // canvas around an un-widened guest frustum. The owner re-latches here and re-asserts the centre
  // only when it actually differs, so a 4:3 run still writes exactly what 0x80016B04 wrote.
  context(core).widescreen.synchronizePresentation(core);
  game_.presentation.commit(&core, static_cast<int>(fields), game_.temporalPresentation.get());
  ++completedFrames_;
}

} // namespace tomba1

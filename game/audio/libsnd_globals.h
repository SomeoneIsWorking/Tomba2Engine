// game/audio/libsnd_globals.h — the LIBSND CLUSTER: the sound driver's globals, named.
//
// WHY THIS FILE EXISTS. `voiceStateFlush` — the per-frame SPU voice-state flush, 12,000 substrate
// dispatches per 6,000 replay frames — was 385 lines in which the base address appeared as the
// expression `(uint32_t)32784u << 16` eighty-seven times and every global was a bare decimal offset
// inside it: `c->mem_r32((c->r[2] + (uint32_t)23468))`. That is not a port, it is a transcript, and it
// hid the file's own bug surface: a decimal offset is a number, and nothing about the number says
// which of four neighbouring words it is or what writes it.
//
// THE BASE. `32784 << 16` is 0x80100000, and the guest's own `lui` immediate is 32784 — that is where
// the idiom comes from and it is kept as a named constant with the arithmetic shown, because the
// decimal is what makes it unreadable. It is NOT the same arithmetic as the scratchpad's 0x1F800000
// (which is a plain address in the guest's instruction stream), and a title that has both has already
// had one of them written as the other's arithmetic.
//
// WHAT IS NAMED AND WHAT IS NOT. Names come from two places, and the header says which is which:
//   * a field whose role this repository has CONFIRMED — its own comment in the driver records what
//     reads or writes it, or a gate proves it;
//   * a field whose role is NOT confirmed, named for where it is and what shape it is, with that
//     stated. `kUnnamed...` names are not a gap in this header; they are the honest answer for a
//     number nobody has read out of the image, and they are the first step to reading it.
#pragma once
#include <cstdint>

namespace tomba::audio::libsnd {

// The cluster base. Shown as the guest writes it, with the value it evaluates to, because a reader
// who cannot do the shift in their head is exactly the reader this header is for.
inline constexpr uint32_t kBase = 32784u << 16; // 0x80100000

// A cluster address from an offset, so a call site reads `addr(kFoo)` rather than a shift followed by
// an addition. The offset is the guest's own immediate; nothing is renormalised.
constexpr uint32_t addr(uint32_t offset) {
  return kBase + offset;
}

// ---- SsSeqCalled's own cluster (the sequence scheduler's globals) -------------------------------
//
// Names confirmed by the driver's own reading of the guest: each is a global the sequence scheduler
// or one of its leaves reads, writes, or bounds a loop with.

inline constexpr uint32_t kSeqReentryFlag = 21668u; // 0x801054BC — set while the tick is running
inline constexpr uint32_t kSeqActiveMask = 21672u;  // 0x801054C8 — bit i set => sequence i is active
inline constexpr uint32_t kSeqPtrArray = 21680u;    // 0x801054D0 — 4-byte-stride per-sequence tables
inline constexpr uint32_t kSeqCount = 21712u;       // 0x801054F0 — s16 sequence slots
inline constexpr uint32_t kSeqChanCount = 21714u;   // 0x801054F2 — s16 channels per sequence
inline constexpr uint32_t kUserCallback = 70448u;   // 0x8010ACC0 — the optional user callback slot
inline constexpr uint32_t kSeqTickFn = 70444u;      // 0x8010ACBC — the *SsSeqCalled function slot
inline constexpr uint32_t kSeqPrepFn = 46144u;      // 0x800931C0 — the one-shot pre-loop call

// ---- the per-VOICE table (stride 56) and the per-voice words the flush reads --------------------
//
// The table starts at `kPerVoiceTable` and each record is 56 bytes. The six words below are
// consecutive pairs inside one record: the flush reads a 16-bit value and an 8-bit value from each
// pair and packs them into the SPU register write, so the pairs are named for the register they
// become rather than for a role nobody has traced.

inline constexpr uint32_t kPerVoiceTable = 21710u;       // 0x801054CE — the stride-56 record array
inline constexpr uint32_t kPerVoiceStride = 56u;         // bytes per record
inline constexpr uint32_t kVoiceCountLimit = 15u;        // the flush's own `i < 15` loop bound
inline constexpr uint32_t kToneCountLimit = 24u;         // the tone block's own `i < 24` loop bound
inline constexpr uint32_t kPerVoicePitchLo = 21692u;     // 0x801054D4 — low 16 bits of the pitch word
inline constexpr uint32_t kPerVoicePitchHi = 21694u;     // 0x801054D6 — its high byte
inline constexpr uint32_t kPerVoiceLevelLo = 21696u;     // 0x801054D8
inline constexpr uint32_t kPerVoiceLevelHi = 21698u;     // 0x801054DA
inline constexpr uint32_t kActiveVoiceMaskLo = 21688u;   // 0x801054D0
inline constexpr uint32_t kActiveVoiceMaskHi = 21690u;   // 0x801054D2
inline constexpr uint32_t kHardwareVoiceActive = 21694u; // 0x801054D6 — the SPU's own active bitmask

// A per-record byte the flush CLEARS and never reads back. The flush writes 0 here as part of
// clearing a record it has just consumed, so the name says what is written, not what the byte means.
inline constexpr uint32_t kPerVoiceConsumedFlag = 21733u; // 0x801054E1

// A 16-bit field in the same stride-56 table the flush tests and, on some records, dispatches
// through as a function pointer. The dispatch is confirmed; WHAT the table is for is not.
inline constexpr uint32_t kPerVoiceDispatchLo = 21746u; // 0x801054EE
inline constexpr uint32_t kPerVoiceDispatchHi = 21748u; // 0x801054F0

// ---- the KON-style armed mask and the scratch words ----------------------------------------------

inline constexpr uint32_t kKonArmedMaskLo = 23536u; // 0x80105BF0
inline constexpr uint32_t kKonArmedMaskHi = 23538u; // 0x80105BF2

// ---- the three small loop-bound / cursor words the flush reads ----------------------------------
//
// Each is a signed byte holding a COUNT, and each is read once per pass to bound that pass's loop.
// The names say which pass, which is what the flush's control flow depends on and what a bare
// `mem_r8(base + 23788)` did not say.

inline constexpr uint32_t kVoiceCursor = 23468u;          // 0x80105BAC — incremented then masked to 4 bits
inline constexpr uint32_t kVoiceStateTable = 23472u;      // 0x80105BB0 — the 15-entry active-record array
inline constexpr uint32_t kSpuKeyScanCount = 23788u;      // 0x80105CCC — s8, bounds the key-scan pass
inline constexpr uint32_t kToneCursor = 23848u;           // 0x80105D08 — bounds the tone pass
inline constexpr uint32_t kToneBlockBase = 23048u;        // 0x801059F8 — the 24-entry tone block
inline constexpr uint32_t kUnnamedWord23072 = 23072u;     // 0x80105A10 — see the note below
inline constexpr uint32_t kUnnamedWord23080 = 23080u;     // 0x80105A18 — see the note below
inline constexpr uint32_t kUnnamedWord23464 = 23464u;     // 0x80105BA8 — see the note below
inline constexpr uint32_t kUnnamedHalfword21734 = 21734u; // 0x801054E2 — see the note below

// THE FOUR `kUnnamed` WORDS, and the one `kUnnamedHalfword`, stated once.
//
// The flush reads 23072 and dispatches through it; it reads 23080 and takes six byte-strided pointers
// off it; it reads 23464 and dispatches through it. None of the four is read anywhere else in this
// repository, and no gate has established what any of them is. Rather than give them a confident
// name, they are named for WHERE they are read and the note says so. Concretely:
//   * 23072 — READ, then dispatched through as a function pointer. The dispatch is certain; what the
//     word holds is not.
//   * 23080 — READ, then six pointers at +0/+2/+4/+6/+8/+10 are taken from it. It is the base of a
//     six-entry block of 16-bit values, and the block's shape is certain while its purpose is not.
//   * 23464 — READ, then dispatched through as a function pointer, in the same pass as 23072 and under
//     the same condition. Whether it is the same kind of word as 23072 is not established.
//   * 21734 — READ as a signed 16-bit comparison per record, and the pass it bounds is entered on a
//     DIFFERENT cursor (23072) from the 15-entry active-record pass it is not. Nothing writes it here.

} // namespace tomba::audio::libsnd

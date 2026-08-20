// game/input/pad_sampler.h — the controller port-0 button-mask sampler (guest FUN_800524B4).
//
// Separate from class Input (game/input/input.cpp), which owns the SPU voice-table leaves that
// happen to live in the same guest address band. This is pad sampling; that is audio. Same folder,
// different subsystem.
#pragma once
struct Core;
class Game;

class PadSampler {
public:
  // FUN_800524B4 — samples the port-0 controller and returns the button mask. 6,000 substrate
  // dispatches per 6000 replay frames, i.e. exactly once per frame.
  static void sampleButtonMask(Core *c);

  static void registerOverrides(Game *game);
};

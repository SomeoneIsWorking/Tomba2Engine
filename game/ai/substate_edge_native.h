#pragma once

struct Core;
class Game;

// Owns the authored child-oscillator loop for the multi-part assembly at 0x8012EB54.
// Seaside water pumps use this assembly; AssemblyNode owns its recovered field layout.
class SubstateEdgeLeaves {
public:
  // A00 entry 0x801316CC, a0 = assembly node. Calls guest 0x80130D5C for each driven
  // group-head slot when config bit2 is set. Bit1 selects paired slots and is re-read
  // after each call because the child can change the remaining iteration count.
  static void tickChildOscillators(Core *core);

  static void registerOverrides(Game *game);
};

// game/ui/loading_text.h — the blinking "Loading....." indicator (guest FUN_8007FD54).
// See loading_text.cpp for the RE and for why pc_skip draws nothing.
#pragma once
class Core;

class LoadingText {
public:
  // Fork entry: pc_skip ON draws nothing, OFF emits the guest's exact packet.
  static void draw(Core* c);

  static void drawFaithful(Core* c);   // guest body, byte-exact (port_check vs gen_func_8007FD54)
  static void drawSkip(Core* c);       // pc_skip: no indicator — the host load already finished
};

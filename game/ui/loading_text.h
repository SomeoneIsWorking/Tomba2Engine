// game/ui/loading_text.h — the blinking "Loading....." indicator (guest FUN_8007FD54).
// See loading_text.cpp for the RE and for why pc_skip draws nothing.
#pragma once
class Core;

class LoadingText {
public:
  static void draw(Core* c);           // entry — always the guest body; see the .cpp on why no fork
  static void drawFaithful(Core* c);   // guest body, byte-exact (port_check vs gen_func_8007FD54)
};

// game/ui/loading_text.h — the blinking "Loading....." indicator (guest FUN_8007FD54).
// Product loads finish synchronously before this wait-loop drawer is reachable; the exact body is
// retained for generated/oracle execution.
#pragma once
class Core;

class LoadingText {
public:
  static void draw(Core* c); // guest body, byte-exact (port_check vs gen_func_8007FD54)
};

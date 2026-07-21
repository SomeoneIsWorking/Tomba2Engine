#pragma once
// DialogBoxSm — PORT_GEN scaffold class (see docs/port-framework.md). UNWIRED.
// Regenerate the body with: python3 tools/port_gen.py <addr> --class DialogBoxSm --method step
class Core;

class DialogBoxSm {
public:
  Core* core = nullptr;

  static void step(Core* c);
};

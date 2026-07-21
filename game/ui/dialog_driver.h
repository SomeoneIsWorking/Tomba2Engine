#pragma once
// DialogDriver — PORT_GEN scaffold class (see docs/port-framework.md). UNWIRED.
// Regenerate the body with: python3 tools/port_gen.py <addr> --class DialogDriver --method stepFaithful
class Core;

class DialogDriver {
public:
  Core* core = nullptr;

  static void stepFaithful(Core* c);
};

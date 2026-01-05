#pragma once
#include "BytecodeStructs.h"
#include <string>

class Disassembler {
public:
  static std::string disassemble(const Instruction &inst);
};

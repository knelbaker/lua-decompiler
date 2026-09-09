#include "Disassembler.h"
#include <iomanip>
#include <sstream>

std::string Disassembler::disassemble(const Instruction &inst) {
  OpCode op = inst.getOpCode();
  OpMode mode = getOpMode(op);
  std::string name = getOpCodeName(op);

  std::stringstream ss;
  ss << std::left << std::setw(11) << name << " ";

  switch (mode) {
  case OpMode::iABC:
    ss << inst.getA() << " " << inst.getB() << " " << inst.getC();
    if (inst.getk()) {
      ss << " (k)";
    }
    break;
  case OpMode::iAsBx:
    ss << inst.getA() << " " << inst.getsBx();
    break;
  case OpMode::iABx:
    ss << inst.getA() << " " << inst.getBx();
    break;
  case OpMode::iAx:
    ss << inst.getAx();
    break;
  case OpMode::isJ:
    ss << inst.getsJ();
    break;
  }

  return ss.str();
}

#include "Disassembler.h"
#include <iomanip>
#include <sstream>

std::string Disassembler::disassemble(const Instruction &inst) {
  OpCode op = inst.getOpCode();
  OpMode mode = getOpMode(op);
  std::string name = getOpCodeName(op);

  std::stringstream ss;
  ss << std::left << std::setw(10) << name << " ";

  switch (mode) {
  case OpMode::iABC:
    ss << inst.getA() << " " << inst.getB() << " " << inst.getC()
       << " (k=" << inst.getk() << ")";
    break;
  case OpMode::iABx:
    ss << inst.getA() << " " << inst.getBx();
    break;
  case OpMode::iAsBx:
    ss << inst.getA() << " " << inst.getsBx();
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

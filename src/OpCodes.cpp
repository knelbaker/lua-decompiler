#include "OpCodes.h"
#include <string>
#include <vector>

std::string getOpCodeName(OpCode op) {
  static const std::vector<std::string> names = {
      "MOVE",       "LOADI",      "LOADF",      "LOADK",      "LOADKX",
      "LOADFALSE",  "LFALSESKIP", "LOADTRUE",   "LOADNIL",    "GETUPVAL",
      "SETUPVAL",   "GETTABUP",   "GETTABLE",   "GETI",       "GETFIELD",
      "SETTABUP",   "SETTABLE",   "SETI",       "SETFIELD",   "NEWTABLE",
      "SELF",       "ADDI",       "ADDK",       "SUBK",       "MULK",
      "MODK",       "POWK",       "DIVK",       "IDIVK",      "BANDK",
      "BORK",       "BXORK",      "SHRI",       "SHLI",       "ADD",
      "SUB",        "MUL",        "MOD",        "POW",        "DIV",
      "IDIV",       "BAND",       "BOR",        "BXOR",       "SHL",
      "SHR",        "MMBIN",      "MMBINI",     "MMBINK",     "UNM",
      "BNOT",       "NOT",        "LEN",        "CONCAT",     "CLOSE",
      "TBC",        "JMP",        "EQ",         "LT",         "LE",
      "EQK",        "EQI",        "LTI",        "LEI",        "GTI",
      "GEI",        "TEST",       "TESTSET",    "CALL",       "TAILCALL",
      "RETURN",     "RETURN0",    "RETURN1",    "FORLOOP",    "FORPREP",
      "TFORPREP",   "TFORCALL",   "TFORLOOP",   "SETLIST",    "CLOSURE",
      "VARARG",     "VARARGPREP", "EXTRAARG"};

  int index = static_cast<int>(op);
  if (index >= 0 && index < static_cast<int>(names.size())) {
    return names[index];
  }
  return "UNKNOWN";
}

OpMode getOpMode(OpCode op) {
  switch (op) {
  case OpCode::OP_LOADI:
  case OpCode::OP_LOADF:
    return OpMode::iAsBx;

  case OpCode::OP_LOADK:
  case OpCode::OP_LOADKX:
  case OpCode::OP_FORLOOP:
  case OpCode::OP_FORPREP:
  case OpCode::OP_TFORPREP:
  case OpCode::OP_TFORLOOP:
  case OpCode::OP_CLOSURE:
    return OpMode::iABx;

  case OpCode::OP_JMP:
    return OpMode::isJ;

  case OpCode::OP_EXTRAARG:
    return OpMode::iAx;

  default:
    return OpMode::iABC;
  }
}

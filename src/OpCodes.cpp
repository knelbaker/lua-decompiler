#include "OpCodes.h"
#include <string>
#include <vector>

std::string getOpCodeName(OpCode op) {
  static const std::vector<std::string> names = {
      "MOVE",       "LOADI",      "LOADF",    "LOADK",    "LOADKX",
      "LOADFALSE",  "LFALSESKIP", "LOADTRUE", "LOADNIL",  "GETUPVAL",
      "SETUPVAL",   "GETTABUP",   "GETTABLE", "GETI",     "GETFIELD",
      "SETTABUP",   "SETTABLE",   "SETI",     "SETFIELD", "NEWTABLE",
      "SELF",       "ADDI",       "ADDK",     "SUBK",     "MULK",
      "MODK",       "POWK",       "DIVK",     "IDIVK",    "BANDK",
      "BORK",       "BXORK",      "SHRI",     "SHLI",     "ADD",
      "SUB",        "MUL",        "MOD",      "POW",      "DIV",
      "IDIV",       "BAND",       "BOR",      "BXOR",     "SHL",
      "SHR",        "MMBIN",      "MMBINK",   "UNM",      "BNOT",
      "NOT",        "LEN",        "CONCAT",   "CLOSE",    "TBC",
      "JMP",        "EQ",         "LT",       "LE",       "EQK",
      "EQI",        "LTI",        "LEI",      "GTI",      "GEI",
      "TEST",       "TESTSET",    "CALL",     "TAILCALL", "RETURN",
      "RETURN0",    "RETURN1",    "FORLOOP",  "FORPREP",  "TFORPREP",
      "TFORCALL",   "TFORLOOP",   "SETLIST",  "CLOSURE",  "VARARG",
      "UNKNOWN_80", "VARARGPREP", "EXTRAARG", "UNKNOWN"};

  int index = static_cast<int>(op);
  if (index >= 0 && index < static_cast<int>(names.size())) {
    return names[index];
  }
  return "UNKNOWN";
}

OpMode getOpMode(OpCode op) {
  switch (op) {
  case OpCode::OP_MOVE:
  case OpCode::OP_LOADI:
  case OpCode::OP_LOADF:
    return OpMode::iABC;
  case OpCode::OP_LOADK:
  case OpCode::OP_LOADKX:
    return OpMode::iABx; // Verify encoding
  case OpCode::OP_LOADFALSE:
  case OpCode::OP_LFALSESKIP:
  case OpCode::OP_LOADTRUE:
  case OpCode::OP_LOADNIL:
  case OpCode::OP_GETUPVAL:
  case OpCode::OP_SETUPVAL:
  case OpCode::OP_GETTABUP:
  case OpCode::OP_GETTABLE:
  case OpCode::OP_GETI:
  case OpCode::OP_GETFIELD:
  case OpCode::OP_SETTABUP:
  case OpCode::OP_SETTABLE:
  case OpCode::OP_SETI:
  case OpCode::OP_SETFIELD:
  case OpCode::OP_NEWTABLE:
  case OpCode::OP_SELF:
  case OpCode::OP_ADDI:
  case OpCode::OP_ADDK:
  case OpCode::OP_SUBK:
  case OpCode::OP_MULK:
  case OpCode::OP_MODK:
  case OpCode::OP_POWK:
  case OpCode::OP_DIVK:
  case OpCode::OP_IDIVK:
  case OpCode::OP_BANDK:
  case OpCode::OP_BORK:
  case OpCode::OP_BXORK:
  case OpCode::OP_SHRI:
  case OpCode::OP_SHLI:
  case OpCode::OP_ADD:
  case OpCode::OP_SUB:
  case OpCode::OP_MUL:
  case OpCode::OP_MOD:
  case OpCode::OP_POW:
  case OpCode::OP_DIV:
  case OpCode::OP_IDIV:
  case OpCode::OP_BAND:
  case OpCode::OP_BOR:
  case OpCode::OP_BXOR:
  case OpCode::OP_SHL:
  case OpCode::OP_SHR:
  case OpCode::OP_MMBIN:
  case OpCode::OP_MMBINK:
  case OpCode::OP_UNM:
  case OpCode::OP_BNOT:
  case OpCode::OP_NOT:
  case OpCode::OP_LEN:
  case OpCode::OP_CONCAT:
  case OpCode::OP_CLOSE:
  case OpCode::OP_TBC:
    return OpMode::iABC;
  case OpCode::OP_JMP:
    return OpMode::isJ;
  case OpCode::OP_EQ:
  case OpCode::OP_LT:
  case OpCode::OP_LE:
  case OpCode::OP_EQK:
  case OpCode::OP_EQI:
  case OpCode::OP_LTI:
  case OpCode::OP_LEI:
  case OpCode::OP_GTI:
  case OpCode::OP_GEI:
  case OpCode::OP_TEST:
  case OpCode::OP_TESTSET:
  case OpCode::OP_CALL:
  case OpCode::OP_TAILCALL:
  case OpCode::OP_RETURN:
  case OpCode::OP_RETURN0:
  case OpCode::OP_RETURN1:
    return OpMode::iABC;
  case OpCode::OP_FORLOOP:
  case OpCode::OP_FORPREP:
  case OpCode::OP_TFORPREP:
  case OpCode::OP_TFORCALL:
  case OpCode::OP_TFORLOOP:
    return OpMode::iABC; // Actually iABC? Check docs. Usually iABC.
  case OpCode::OP_SETLIST:
    return OpMode::iABC;
  case OpCode::OP_CLOSURE:
    return OpMode::iABx;
  case OpCode::OP_VARARG:
  case OpCode::OP_VARARGPREP:
    return OpMode::iABC;
  case OpCode::OP_EXTRAARG:
    return OpMode::iAx;
  default:
    return OpMode::iABC;
  }
}

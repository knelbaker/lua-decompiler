#include "OpCodes.h"

std::string getOpCodeName(OpCode op) {
  static const char *opNames[] = {
      "MOVE",      "LOADI",      "LOADF",    "LOADK",    "LOADKX",
      "LOADFALSE", "LFALSESKIP", "LOADTRUE", "LOADNIL",  "GETUPVAL",
      "SETUPVAL",  "GETTABUP",   "GETTABLE", "GETI",     "GETFIELD",
      "SETTABUP",  "SETTABLE",   "SETI",     "SETFIELD", "NEWTABLE",
      "SELF",      "ADDI",       "ADDK",     "SUBK",     "MULK",
      "MODK",      "POWK",       "DIVK",     "IDIVK",    "BANDK",
      "BORK",      "BXORK",      "SHRI",     "SHLI",     "ADD",
      "SUB",       "MUL",        "MOD",      "POW",      "DIV",
      "IDIV",      "BAND",       "BOR",      "BXOR",     "SHL",
      "SHR",       "MMBIN",      "MMBINK",   "UNM",      "BNOT",
      "NOT",       "LEN",        "CONCAT",   "CLOSE",    "TBC",
      "JMP",       "EQ",         "LT",       "LE",       "EQK",
      "EQI",       "LTI",        "LEI",      "GTI",      "GEI",
      "TEST",      "TESTSET",    "CALL",     "TAILCALL", "RETURN",
      "RETURN0",   "RETURN1",    "FORLOOP",  "FORPREP",  "TFORPREP",
      "TFORCALL",  "TFORLOOP",   "SETLIST",  "CLOSURE",  "VARARG",
      "VARARGC",   "VARARGPREP", "EXTRAARG", "UNKNOWN"};

  int index = static_cast<int>(op);
  if (index >= 0 && index < static_cast<int>(OpCode::NUM_OPCODES)) {
    return opNames[index];
  }
  return "UNKNOWN";
}

OpMode getOpMode(OpCode op) {
  switch (op) {
  case OpCode::OP_MOVE:
    return OpMode::iABC;
  case OpCode::OP_LOADI:
    return OpMode::iAsBx;
  case OpCode::OP_LOADF:
    return OpMode::iAsBx;
  case OpCode::OP_LOADK:
    return OpMode::iABx;
  case OpCode::OP_LOADKX:
    return OpMode::iABx;
  case OpCode::OP_LOADFALSE:
    return OpMode::iABC;
  case OpCode::OP_LFALSESKIP:
    return OpMode::iABC;
  case OpCode::OP_LOADTRUE:
    return OpMode::iABC;
  case OpCode::OP_LOADNIL:
    return OpMode::iABC;
  case OpCode::OP_GETUPVAL:
    return OpMode::iABC;
  case OpCode::OP_SETUPVAL:
    return OpMode::iABC;
  case OpCode::OP_GETTABUP:
    return OpMode::iABC;
  case OpCode::OP_GETTABLE:
    return OpMode::iABC;
  case OpCode::OP_GETI:
    return OpMode::iABC;
  case OpCode::OP_GETFIELD:
    return OpMode::iABC;
  case OpCode::OP_SETTABUP:
    return OpMode::iABC;
  case OpCode::OP_SETTABLE:
    return OpMode::iABC;
  case OpCode::OP_SETI:
    return OpMode::iABC;
  case OpCode::OP_SETFIELD:
    return OpMode::iABC;
  case OpCode::OP_NEWTABLE:
    return OpMode::iABC;
  case OpCode::OP_SELF:
    return OpMode::iABC;
  case OpCode::OP_ADDI:
    return OpMode::iABC;
  case OpCode::OP_ADDK:
    return OpMode::iABC;
  case OpCode::OP_SUBK:
    return OpMode::iABC;
  case OpCode::OP_MULK:
    return OpMode::iABC;
  case OpCode::OP_MODK:
    return OpMode::iABC;
  case OpCode::OP_POWK:
    return OpMode::iABC;
  case OpCode::OP_DIVK:
    return OpMode::iABC;
  case OpCode::OP_IDIVK:
    return OpMode::iABC;
  case OpCode::OP_BANDK:
    return OpMode::iABC;
  case OpCode::OP_BORK:
    return OpMode::iABC;
  case OpCode::OP_BXORK:
    return OpMode::iABC;
  case OpCode::OP_SHRI:
    return OpMode::iABC;
  case OpCode::OP_SHLI:
    return OpMode::iABC;
  case OpCode::OP_ADD:
    return OpMode::iABC;
  case OpCode::OP_SUB:
    return OpMode::iABC;
  case OpCode::OP_MUL:
    return OpMode::iABC;
  case OpCode::OP_MOD:
    return OpMode::iABC;
  case OpCode::OP_POW:
    return OpMode::iABC;
  case OpCode::OP_DIV:
    return OpMode::iABC;
  case OpCode::OP_IDIV:
    return OpMode::iABC;
  case OpCode::OP_BAND:
    return OpMode::iABC;
  case OpCode::OP_BOR:
    return OpMode::iABC;
  case OpCode::OP_BXOR:
    return OpMode::iABC;
  case OpCode::OP_SHL:
    return OpMode::iABC;
  case OpCode::OP_SHR:
    return OpMode::iABC;
  case OpCode::OP_MMBIN:
    return OpMode::iABC;
  case OpCode::OP_MMBINK:
    return OpMode::iABC;
  case OpCode::OP_UNM:
    return OpMode::iABC;
  case OpCode::OP_BNOT:
    return OpMode::iABC;
  case OpCode::OP_NOT:
    return OpMode::iABC;
  case OpCode::OP_LEN:
    return OpMode::iABC;
  case OpCode::OP_CONCAT:
    return OpMode::iABC;
  case OpCode::OP_CLOSE:
    return OpMode::iABC;
  case OpCode::OP_TBC:
    return OpMode::iABC;
  case OpCode::OP_JMP:
    return OpMode::isJ;
  case OpCode::OP_EQ:
    return OpMode::iABC;
  case OpCode::OP_LT:
    return OpMode::iABC;
  case OpCode::OP_LE:
    return OpMode::iABC;
  case OpCode::OP_EQK:
    return OpMode::iABC;
  case OpCode::OP_EQI:
    return OpMode::iABC;
  case OpCode::OP_LTI:
    return OpMode::iABC;
  case OpCode::OP_LEI:
    return OpMode::iABC;
  case OpCode::OP_GTI:
    return OpMode::iABC;
  case OpCode::OP_GEI:
    return OpMode::iABC;
  case OpCode::OP_TEST:
    return OpMode::iABC;
  case OpCode::OP_TESTSET:
    return OpMode::iABC;
  case OpCode::OP_CALL:
    return OpMode::iABC;
  case OpCode::OP_TAILCALL:
    return OpMode::iABC;
  case OpCode::OP_RETURN:
    return OpMode::iABC;
  case OpCode::OP_RETURN0:
    return OpMode::iABC;
  case OpCode::OP_RETURN1:
    return OpMode::iABC;
  case OpCode::OP_FORLOOP:
    return OpMode::iABx;
  case OpCode::OP_FORPREP:
    return OpMode::iABx;
  case OpCode::OP_TFORPREP:
    return OpMode::iABx;
  case OpCode::OP_TFORCALL:
    return OpMode::iABC;
  case OpCode::OP_TFORLOOP:
    return OpMode::iABx;
  case OpCode::OP_SETLIST:
    return OpMode::iABC;
  case OpCode::OP_CLOSURE:
    return OpMode::iABx;
  case OpCode::OP_VARARG:
    return OpMode::iABC;
  case OpCode::OP_VARARGC:
    return OpMode::iABC;
  case OpCode::OP_VARARGPREP:
    return OpMode::iABC;
  case OpCode::OP_EXTRAARG:
    return OpMode::iAx;
  default:
    return OpMode::iABC;
  }
}

#pragma once

#include <cstdint>
#include <string>

// Lua 5.4 OpCodes
// Source: lopcodes.h from Lua 5.4.x
enum class OpCode : uint8_t {
  OP_MOVE,
  OP_LOADI,
  OP_LOADF,
  OP_LOADK,
  OP_LOADKX,
  OP_LOADFALSE,
  OP_LFALSESKIP,
  OP_LOADTRUE,
  OP_LOADNIL,
  OP_GETUPVAL,
  OP_SETUPVAL,
  OP_GETTABUP,
  OP_GETTABLE,
  OP_GETI,
  OP_GETFIELD,
  OP_SETTABUP,
  OP_SETTABLE,
  OP_SETI,
  OP_SETFIELD,
  OP_NEWTABLE,
  OP_SELF,
  OP_ADDI,
  OP_ADDK,
  OP_SUBK,
  OP_MULK,
  OP_MODK,
  OP_POWK,
  OP_DIVK,
  OP_IDIVK,
  OP_BANDK,
  OP_BORK,
  OP_BXORK,
  OP_SHRI,
  OP_SHLI,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_MOD,
  OP_POW,
  OP_DIV,
  OP_IDIV,
  OP_BAND,
  OP_BOR,
  OP_BXOR,
  OP_SHL,
  OP_SHR,
  OP_MMBIN,
  OP_MMBINK,
  OP_UNM,
  OP_BNOT,
  OP_NOT,
  OP_LEN,
  OP_CONCAT,
  OP_CLOSE,
  OP_TBC,
  OP_JMP,
  OP_EQ,
  OP_LT,
  OP_LE,
  OP_EQK,
  OP_EQI,
  OP_LTI,
  OP_LEI,
  OP_GTI,
  OP_GEI,
  OP_TEST,
  OP_TESTSET,
  OP_CALL,
  OP_TAILCALL,
  OP_RETURN,
  OP_RETURN0,
  OP_RETURN1,
  OP_FORLOOP,
  OP_FORPREP,
  OP_TFORPREP,
  OP_TFORCALL,
  OP_TFORLOOP,
  OP_SETLIST,
  OP_CLOSURE,
  OP_VARARG,
  OP_VARARGC,
  OP_VARARGPREP,
  OP_EXTRAARG,
  NUM_OPCODES
};

// Instruction Formats
enum class OpMode { iABC, iABx, iAsBx, iAx, isJ };

// Instruction decoding helpers (Lua 5.4)
// Instruction is 32-bit:
// iABC:  [C:8] [B:8] [k:1] [A:8] [Op:7]
// iABx:  [Bx:17] [A:8] [Op:7]
// iAsBx: [sBx:17] [A:8] [Op:7]
// iAx:   [Ax:25] [Op:7]
// isJ:   [sJ:25] [Op:7]

// Sizes
constexpr int SIZE_C = 8;
constexpr int SIZE_B = 8;
constexpr int SIZE_Bx = SIZE_C + SIZE_B + 1; // 17
constexpr int SIZE_A = 8;
constexpr int SIZE_AX = SIZE_C + SIZE_B + 1 + SIZE_A; // 25
constexpr int SIZE_OP = 7;

// Offsets
constexpr int POS_OP = 0;
constexpr int POS_A = POS_OP + SIZE_OP;
constexpr int POS_k = POS_A + SIZE_A;
constexpr int POS_B = POS_k + 1;
constexpr int POS_C = POS_B + SIZE_B;
constexpr int POS_Bx = POS_A + SIZE_A;
constexpr int POS_Ax = POS_OP + SIZE_OP;
constexpr int POS_sJ = POS_OP + SIZE_OP;

// Bitmasks
constexpr uint32_t BITMASK(int n) { return (~((~(uint32_t)0) << n)); }

struct Instruction {
  uint32_t params;

  Instruction(uint32_t p) : params(p) {}

  OpCode getOpCode() const {
    return static_cast<OpCode>((params >> POS_OP) & BITMASK(SIZE_OP));
  }

  int getA() const { return (params >> POS_A) & BITMASK(SIZE_A); }

  int getB() const { return (params >> POS_B) & BITMASK(SIZE_B); }

  int getC() const { return (params >> POS_C) & BITMASK(SIZE_C); }

  int getk() const { return (params >> POS_k) & BITMASK(1); }

  int getBx() const { return (params >> POS_Bx) & BITMASK(SIZE_Bx); }

  int getAx() const { return (params >> POS_Ax) & BITMASK(SIZE_AX); }

  // Signed conversions
  // Lua uses excess-K representation for signed values
  // MAX_ARG_Bx = (1<<17) - 1
  // OFFSET_sBx = MAX_ARG_Bx >> 1  = 65535

  static constexpr int MAX_ARG_Bx = (1 << SIZE_Bx) - 1;
  static constexpr int OFFSET_sBx = MAX_ARG_Bx >> 1;

  static constexpr int MAX_ARG_sJ = (1 << SIZE_AX) - 1;
  static constexpr int OFFSET_sJ = MAX_ARG_sJ >> 1;

  int getsBx() const { return getBx() - OFFSET_sBx; }

  int getsJ() const { return getAx() - OFFSET_sJ; }
};

std::string getOpCodeName(OpCode op);
OpMode getOpMode(OpCode op);

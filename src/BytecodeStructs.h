#pragma once

#include "OpCodes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

// Lua 5.4 Header Constants
constexpr char LUA_SIGNATURE[] = "\x1bLua";
constexpr uint8_t LUAC_VERSION = 0x54; // 5.4
constexpr uint8_t LUAC_FORMAT = 0;
constexpr char LUAC_DATA[] = "\x19\x93\r\n\x1a\n";
constexpr uint64_t LUAC_INT = 0x5678;
constexpr double LUAC_NUM = 370.5;

// Basic Types
enum class LType : uint8_t { NIL = 0, BOOLEAN = 1, NUMBER = 3, STRING = 4 };

// Lua Value
struct LValue {
  LType type;
  // Using variant or specialized fields
  bool boolean;
  double number; // We'll store both int/float here for simplicity, or
                 // differentiate if needed
  long long integer;
  bool isInteger;
  std::string str;
};

struct UpvalueInfo {
  bool instack;
  uint8_t idx;
  std::string name; // From debug info
};

struct LocalVarInfo {
  std::string name;
  int startPC;
  int endPC;
};

struct Proto {
  std::string source;
  int lineDefined;
  int lastLineDefined;
  uint8_t numParams;
  uint8_t isVarArg;
  uint8_t maxStackSize;

  std::vector<Instruction> code;
  std::vector<LValue> k; // Constants
  std::vector<UpvalueInfo> upvalues;
  std::vector<std::unique_ptr<Proto>> p; // Inner functions
  std::vector<LocalVarInfo> locVars;

  std::vector<int> lineInfo; // mapping from instruction to line number
  std::vector<int> absLineInfo;
};

struct LFunction {
  std::unique_ptr<Proto> proto;
  std::vector<UpvalueInfo> upvalues;
};

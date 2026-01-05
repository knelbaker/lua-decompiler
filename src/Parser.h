#pragma once

#include "BytecodeStructs.h"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class BytecodeParser {
public:
  BytecodeParser(const std::string &filename);
  ~BytecodeParser();

  std::unique_ptr<LFunction> parse();

private:
  std::ifstream file;
  std::string filename;

  // Config read from header
  uint8_t sizeLuaNumber;
  uint8_t sizeLuaInteger;
  uint8_t sizeInstruction;

  // Helper read functions
  uint8_t readByte();
  int readInt();           // Uses VarInt
  uint64_t readInteger();  // Lua Integer (Raw)
  double readNumber();     // Lua Number (Raw)
  size_t readSizeT();      // Uses VarInt
  uint64_t readUnsigned(); // VarInt implementation
  std::string readString();
  uint32_t readInstruction(); // Reads 32-bit instruction (Raw)

  void checkHeader();
  std::unique_ptr<Proto> readFunction(const std::string &source);
  void readCode(Proto &f);
  void readConstants(Proto &f);
  void readUpvalues(Proto &f);
  void readProtos(Proto &f);
  void readDebug(Proto &f);
};

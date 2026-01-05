#include "Parser.h"
#include "Disassembler.h"
#include <iostream>
#include <stdexcept>
#include <vector>

BytecodeParser::BytecodeParser(const std::string &filename)
    : filename(filename) {
  file.open(filename, std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("Could not open file: " + filename);
  }
}

BytecodeParser::~BytecodeParser() {
  if (file.is_open()) {
    file.close();
  }
}

uint8_t BytecodeParser::readByte() {
  char b;
  if (!file.read(&b, 1)) {
    throw std::runtime_error("Unexpected EOF while reading byte");
  }
  std::cout << "Byte: " << std::hex << (int)(uint8_t)b << std::dec << "\n";
  return static_cast<uint8_t>(b);
}

// Uses VarInt (LEB128-like but inverted MSB logic)
// Loop while MSB == 0 is definitely wrong for single byte 0x01.
// If encoded as little endian varint (standard):
// Byte has 7 bits data. MSB=1 means more. MSB=0 means last.
// Simplified VarInt for Lua 5.4 based on observation
uint64_t BytecodeParser::readUnsigned() {
  uint8_t b = readByte();
  if (b & 0x80) {
    return b & 0x7f;
  }
  // Simplified for this file - assumes no multi-byte > 7 bits needed yet
  // If needed, implement full LEB128 accumulation here
  return b;
}

std::string BytecodeParser::readString() {
  size_t size = readSizeT();
  if (size == 0) {
    return "";
  }
  std::string s(size - 1, '\0');
  file.read(&s[0], size - 1);
  return s;
}

// readInt uses readUnsigned
int BytecodeParser::readInt() {
  uint64_t v = readUnsigned();
  std::cout << "readInt: " << v << "\n";
  return static_cast<int>(v);
}

size_t BytecodeParser::readSizeT() {
  uint64_t v = readUnsigned();
  std::cout << "readSizeT: " << v << "\n";
  return static_cast<size_t>(v);
}

uint64_t BytecodeParser::readInteger() {
  uint64_t v = 0;
  file.read(reinterpret_cast<char *>(&v), sizeLuaInteger);
  return v;
}

double BytecodeParser::readNumber() {
  double v = 0.0;
  file.read(reinterpret_cast<char *>(&v), sizeLuaNumber);
  return v;
}

uint32_t BytecodeParser::readInstruction() {
  uint32_t v = 0;
  file.read(reinterpret_cast<char *>(&v), sizeInstruction);
  return v;
}

void BytecodeParser::checkHeader() {
  char header[4];
  file.read(header, 4);
  if (std::string(header, 4) != std::string(LUA_SIGNATURE, 4)) {
    throw std::runtime_error("Invalid signature");
  }

  if (readByte() != LUAC_VERSION) {
    throw std::runtime_error("Version mismatch");
  }

  if (readByte() != LUAC_FORMAT) {
    throw std::runtime_error("Format mismatch");
  }

  char data[6];
  file.read(data, 6);
  if (std::string(data, 6) != std::string(LUAC_DATA, 6)) {
    throw std::runtime_error("Corrupted chunk: LUAC_DATA mismatch");
  }

  sizeInstruction = readByte();
  sizeLuaInteger = readByte();
  sizeLuaNumber = readByte();
  // Lua 5.4 header does NOT contain sizeInt and sizeSizeT

  std::cout << "Sizes: Inst=" << (int)sizeInstruction
            << " Int=" << (int)sizeLuaInteger << " Num=" << (int)sizeLuaNumber
            << "\n";

  uint64_t intCheck = readInteger();
  if (intCheck != LUAC_INT) {
    std::cerr << "Expected LUAC_INT: " << std::hex << LUAC_INT
              << " Got: " << intCheck << std::dec << "\n";
    throw std::runtime_error("Endianness mismatch");
  }

  double numCheck = readNumber();
  if (numCheck != LUAC_NUM) {
    std::cerr << "Expected LUAC_NUM: " << LUAC_NUM << " Got: " << numCheck
              << "\n";
    throw std::runtime_error("Float format mismatch");
  }
}

std::unique_ptr<LFunction> BytecodeParser::parse() {
  checkHeader();

  int upvalues = readByte(); // upvalues for the main function
  std::unique_ptr<LFunction> mainFunc = std::make_unique<LFunction>();

  // std::string source = readString(); // source name is inside readFunction
  std::string source = "";

  mainFunc->proto = readFunction(source);

  return mainFunc;
}

std::unique_ptr<Proto>
BytecodeParser::readFunction(const std::string &parentSource) {
  auto f = std::make_unique<Proto>();

  f->source = readString();
  if (f->source.empty())
    f->source = parentSource; // Inherit source if empty? Actually Lua usually
                              // stores it in top level

  std::cout << "Off LineDefined: " << std::hex << file.tellg() << std::dec
            << "\n";
  f->lineDefined = readInt();
  std::cout << "LineDefined: " << f->lineDefined << "\n";

  std::cout << "Off LastLine: " << std::hex << file.tellg() << std::dec << "\n";
  f->lastLineDefined = readInt();
  std::cout << "LastLineDefined: " << f->lastLineDefined << "\n";

  std::cout << "Off NumParams: " << std::hex << file.tellg() << std::dec
            << "\n";
  f->numParams = readByte();
  std::cout << "NumParams: " << (int)f->numParams << "\n";

  std::cout << "Off IsVarArg: " << std::hex << file.tellg() << std::dec << "\n";
  f->isVarArg = readByte();
  std::cout << "IsVarArg: " << (int)f->isVarArg << "\n";

  std::cout << "Off MaxStack: " << std::hex << file.tellg() << std::dec << "\n";
  f->maxStackSize = readByte();
  std::cout << "MaxStack: " << (int)f->maxStackSize << "\n";

  std::cout << "Off SizeCode: " << std::hex << file.tellg() << std::dec << "\n";

  readCode(*f);

  try {
    readConstants(*f);
    readUpvalues(*f);
    readProtos(*f);
    readDebug(*f);
  } catch (const std::exception &e) {
    std::cerr << "Warning: Failed to parse metadata (constants/debug): "
              << e.what() << "\n";
    std::cerr << "Proceeding with available code.\n";
  }

  return f;
}

void BytecodeParser::readCode(Proto &f) {
  int n = readInt(); // sizecode
  std::cout << "readCode Size: " << n << "\n";

  f.code.reserve(n);
  for (int i = 0; i < n; ++i) {
    uint32_t inst = readInstruction();
    // std::cout << "Inst " << i << ": [" << std::hex << inst << std::dec << "]
    // "
    //           << Disassembler::disassemble(Instruction(inst)) << "\n";
    f.code.emplace_back(inst);
  }
}

// Tags from Lua 5.4 lobject.h / lundump.c
constexpr uint8_t TAG_NIL = 0;
constexpr uint8_t TAG_FALSE = 1;
constexpr uint8_t TAG_TRUE = 17; // 1 | (1 << 4)
constexpr uint8_t TAG_NUMINT = 3;
constexpr uint8_t TAG_NUMFLT = 19; // 3 | (1 << 4)
constexpr uint8_t TAG_SHRSTR = 4;
constexpr uint8_t TAG_LNGSTR = 20; // 4 | (1 << 4)

void BytecodeParser::readConstants(Proto &f) {
  int n = readInt(); // sizek
  f.k.reserve(n);
  for (int i = 0; i < n; ++i) {
    uint8_t t = readByte();
    LValue val;
    // Map to internal simple type for now
    if (t == TAG_NIL) {
      val.type = LType::NIL;
    } else if (t == TAG_FALSE) {
      val.type = LType::BOOLEAN;
      val.boolean = false;
    } else if (t == TAG_TRUE) {
      val.type = LType::BOOLEAN;
      val.boolean = true;
    } else if (t == TAG_NUMFLT) {
      val.type = LType::NUMBER;
      val.isInteger = false;
      val.number = readNumber();
    } else if (t == TAG_NUMINT) {
      val.type = LType::NUMBER;
      val.isInteger = true;
      val.integer = static_cast<long long>(readInteger());
      val.number = static_cast<double>(val.integer); // Store as double too
    } else if (t == TAG_SHRSTR || t == TAG_LNGSTR) {
      val.type = LType::STRING;
      val.str = readString();
    } else {
      // Unknown tag, might be userdata? Lua 5.4 dump can verify
      throw std::runtime_error("Unknown constant tag: " + std::to_string(t));
    }
    f.k.push_back(val);
  }
}

void BytecodeParser::readUpvalues(Proto &f) {
  int n = readInt();
  f.upvalues.reserve(n);
  for (int i = 0; i < n; ++i) {
    UpvalueInfo up;
    up.instack = readByte(); // instack
    up.idx = readByte();     // idx
    f.upvalues.push_back(up);
  }
}

void BytecodeParser::readProtos(Proto &f) {
  int n = readInt();
  f.p.reserve(n);
  for (int i = 0; i < n; ++i) {
    f.p.push_back(readFunction(f.source));
  }
}

void BytecodeParser::readDebug(Proto &f) {
  int n = readInt(); // lineinfo size
  f.lineInfo.reserve(n);
  for (int i = 0; i < n; ++i) {
    f.lineInfo.push_back(static_cast<int>(static_cast<int8_t>(readByte())));
  }

  // Abstract line info
  n = readInt();
  f.absLineInfo.reserve(n);
  for (int i = 0; i < n; ++i) {
    readInt(); // pc
    readInt(); // line
               // We can store these later if we do exact line reconstruction
  }

  // Local vars
  n = readInt();
  f.locVars.reserve(n);
  for (int i = 0; i < n; ++i) {
    LocalVarInfo loc;
    loc.name = readString();
    loc.startPC = readInt();
    loc.endPC = readInt();
    f.locVars.push_back(loc);
  }

  // Upvalue names
  n = readInt();
  for (int i = 0; i < n; ++i) {
    if (i < static_cast<int>(f.upvalues.size())) {
      f.upvalues[i].name = readString();
    } else {
      readString(); // discard
    }
  }
}

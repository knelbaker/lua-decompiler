#include "Parser.h"
#include "Disassembler.h"
#include <iostream>
#include <stdexcept>
#include <vector>

BytecodeParser::BytecodeParser(const std::string &filename)
    : filename(filename), sizeLuaNumber(8), sizeLuaInteger(8), sizeInstruction(4) {
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
  return static_cast<uint8_t>(b);
}

// Lua 5.4 VarInt: MSB-first, bit 7 is stop bit (1 = stop, 0 = continue)
uint64_t BytecodeParser::readUnsigned() {
  uint64_t x = 0;
  uint8_t b = 0;
  do {
    b = readByte();
    x = (x << 7) | (b & 0x7f);
  } while ((b & 0x80) == 0);
  return x;
}

std::string BytecodeParser::readString() {
  size_t size = readSizeT();
  if (size == 0) {
    return "";
  }
  std::string s(size - 1, '\0');
  if (size - 1 > 0) {
    file.read(&s[0], size - 1);
  }
  return s;
}

int BytecodeParser::readInt() {
  return static_cast<int>(readUnsigned());
}

size_t BytecodeParser::readSizeT() {
  return static_cast<size_t>(readUnsigned());
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
    throw std::runtime_error("Invalid signature: not a Lua binary chunk");
  }

  if (readByte() != LUAC_VERSION) {
    throw std::runtime_error("Version mismatch: expected Lua 5.4");
  }

  if (readByte() != LUAC_FORMAT) {
    throw std::runtime_error("Format mismatch: unrecognized format");
  }

  char data[6];
  file.read(data, 6);
  if (std::string(data, 6) != std::string(LUAC_DATA, 6)) {
    throw std::runtime_error("Corrupted chunk: LUAC_DATA mismatch");
  }

  sizeInstruction = readByte();
  sizeLuaInteger = readByte();
  sizeLuaNumber = readByte();

  uint64_t intCheck = readInteger();
  if (intCheck != LUAC_INT) {
    throw std::runtime_error("Endianness / integer format mismatch");
  }

  double numCheck = readNumber();
  if (numCheck != LUAC_NUM) {
    throw std::runtime_error("Float format mismatch");
  }
}

std::unique_ptr<LFunction> BytecodeParser::parse() {
  checkHeader();

  readByte(); // upvalues for the main function
  auto mainFunc = std::make_unique<LFunction>();
  mainFunc->proto = readFunction("");

  return mainFunc;
}

std::unique_ptr<Proto>
BytecodeParser::readFunction(const std::string &parentSource) {
  auto f = std::make_unique<Proto>();

  f->source = readString();
  if (f->source.empty()) {
    f->source = parentSource;
  }

  f->lineDefined = readInt();
  f->lastLineDefined = readInt();
  f->numParams = readByte();
  f->isVarArg = readByte();
  f->maxStackSize = readByte();

  readCode(*f);
  readConstants(*f);
  readUpvalues(*f);
  readProtos(*f);
  readDebug(*f);

  return f;
}

void BytecodeParser::readCode(Proto &f) {
  int n = readInt(); // sizecode
  f.code.reserve(n);
  for (int i = 0; i < n; ++i) {
    f.code.emplace_back(readInstruction());
  }
}

// Tags from Lua 5.4 lobject.h
constexpr uint8_t TAG_NIL = 0;
constexpr uint8_t TAG_FALSE = 1;
constexpr uint8_t TAG_TRUE = 17;   // 1 | (1 << 4)
constexpr uint8_t TAG_NUMINT = 3;
constexpr uint8_t TAG_NUMFLT = 19; // 3 | (1 << 4)
constexpr uint8_t TAG_SHRSTR = 4;
constexpr uint8_t TAG_LNGSTR = 20; // 4 | (1 << 4)

void BytecodeParser::readConstants(Proto &f) {
  int n = readInt(); // sizek
  f.k.reserve(n);
  for (int i = 0; i < n; ++i) {
    uint8_t t = readByte();
    LValue val{};
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
      val.number = static_cast<double>(val.integer);
    } else if (t == TAG_SHRSTR || t == TAG_LNGSTR) {
      val.type = LType::STRING;
      val.str = readString();
    } else {
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
    up.instack = (readByte() != 0);
    up.idx = readByte();
    up.kind = readByte();
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

  // Absolute line info
  n = readInt();
  f.absLineInfo.reserve(n);
  for (int i = 0; i < n; ++i) {
    readInt(); // pc
    readInt(); // line
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
  if (n != 0) {
    n = static_cast<int>(f.upvalues.size());
  }
  for (int i = 0; i < n; ++i) {
    std::string uName = readString();
    if (i < static_cast<int>(f.upvalues.size())) {
      f.upvalues[i].name = uName;
    }
  }
}

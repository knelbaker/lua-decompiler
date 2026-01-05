#include "ASTGenerator.h"
#include "CodeEmitter.h"
#include "Decompiler.h"
#include "Disassembler.h"
#include "Parser.h"
#include <functional>
#include <iomanip>
#include <iostream>

void printProto(const Proto &p, int level = 0) {
  std::string indent(level * 2, ' ');
  std::cout << indent << "Function " << p.source << " defined at line "
            << p.lineDefined << "\n";
  std::cout << indent << "numParams: " << (int)p.numParams
            << " isVarArg: " << (int)p.isVarArg << "\n";

  std::cout << indent << "Constants (" << p.k.size() << "):\n";
  for (size_t i = 0; i < p.k.size(); ++i) {
    const auto &k = p.k[i];
    std::cout << indent << "  [" << i << "] ";
    switch (k.type) {
    case LType::NIL:
      std::cout << "nil";
      break;
    case LType::BOOLEAN:
      std::cout << (k.boolean ? "true" : "false");
      break;
    case LType::NUMBER:
      if (k.isInteger)
        std::cout << k.integer;
      else
        std::cout << k.number;
      break;
    case LType::STRING:
      std::cout << "\"" << k.str << "\"";
      break;
    }
    std::cout << "\n";
  }

  std::cout << indent << "Code (" << p.code.size() << " instructions):\n";
  for (size_t i = 0; i < p.code.size(); ++i) {
    Instruction inst = p.code[i];

    std::cout << indent << "  " << std::setw(3) << i << ": [" << std::hex
              << std::setw(8) << std::setfill('0') << inst.params << std::dec
              << std::setfill(' ') << "] " << Disassembler::disassemble(inst)
              << "\n";
  }

  for (const auto &inner : p.p) {
    printProto(*inner, level + 1);
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input_file.luac>\n";
    return 1;
  }

  try {
    BytecodeParser parser(argv[1]);
    auto func = parser.parse();

    std::cout << "Successfully parsed binary chunk.\n";
    printProto(*func->proto);

    std::cout << "\nRunning Decompiler CFG Analysis...\n";
    Decompiler decompiler(*func->proto);
    decompiler.analyzeCFG();

    std::cout << "\nRunning AST Generation...\n";

    ASTGenerator astGen(*func->proto, decompiler.getBlocks());
    auto root = astGen.generate();

    CodeEmitter emitter;
    emitter.emit(root.get());

  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}

#pragma once
#include "AST.h"
#include "Decompiler.h"
#include <map>

class ASTGenerator {
public:
  ASTGenerator(const Proto &proto, const std::vector<BasicBlock *> &blocks);

  std::unique_ptr<BlockStatement> generate();

private:
  const Proto &proto;
  const std::vector<BasicBlock *> &sequentialBlocks;

  // Register tracking (Symbolic execution state)
  // Map register index -> Expression at current point
  std::map<int, std::unique_ptr<Expression>> registers;

  void processBlock(BasicBlock *block, BlockStatement &outBlock);

  // Expression helpers
  std::unique_ptr<Expression> getRegisterExpr(int reg);
  std::unique_ptr<Expression> getConstantExpr(int kIdx);
  std::unique_ptr<Expression> getUpvalueExpr(int uIdx);
};

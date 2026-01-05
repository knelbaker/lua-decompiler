#include "ASTGenerator.h"
#include <iostream>

ASTGenerator::ASTGenerator(const Proto &proto,
                           const std::vector<BasicBlock *> &blocks)
    : proto(proto), sequentialBlocks(blocks) {}

std::unique_ptr<BlockStatement> ASTGenerator::generate() {
  auto root = std::make_unique<BlockStatement>();

  for (auto *block : sequentialBlocks) {
    processBlock(block, *root);
  }

  return root;
}

std::unique_ptr<Expression> ASTGenerator::getConstantExpr(int kIdx) {
  if (kIdx >= 0 && kIdx < (int)proto.k.size()) {
    return std::make_unique<LiteralExpr>(proto.k[kIdx]);
  }
  return std::make_unique<VariableExpr>("(invalid const)");
}

std::unique_ptr<Expression> ASTGenerator::getUpvalueExpr(int uIdx) {
  if (uIdx >= 0 && uIdx < (int)proto.upvalues.size()) {
    return std::make_unique<VariableExpr>(proto.upvalues[uIdx].name.empty()
                                              ? "_UPVAL_" + std::to_string(uIdx)
                                              : proto.upvalues[uIdx].name);
  }
  // _ENV is usually upvalue 0
  return std::make_unique<VariableExpr>("_ENV");
}

std::unique_ptr<Expression> ASTGenerator::getRegisterExpr(int reg) {
  return std::make_unique<VariableExpr>(reg);
}

void ASTGenerator::processBlock(BasicBlock *block, BlockStatement &outBlock) {
  for (const auto &inst : block->instructions) {
    OpCode op = inst.getOpCode();
    int A = inst.getA();
    int B = inst.getB();
    int C = inst.getC();
    int Bx = inst.getBx();
    int sBx = inst.getsBx();

    if (op == OpCode::OP_LOADK) {
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(std::make_unique<VariableExpr>(A));
      assign->cx.push_back(getConstantExpr(Bx));
      outBlock.add(std::move(assign));
    } else if (op == OpCode::OP_LOADI) {
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(std::make_unique<VariableExpr>(A));
      LValue v;
      v.type = LType::NUMBER;
      v.isInteger = true;
      v.integer = sBx;
      v.number = sBx;
      assign->cx.push_back(std::make_unique<LiteralExpr>(v));
      outBlock.add(std::move(assign));
    } else if (op == OpCode::OP_ADD) {
      // A = B + C
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(std::make_unique<VariableExpr>(A));
      // C can be immediate or register?
      // In Lua 5.4, ADD is R[A] = R[B] + R[C]
      // Constants are handled by ADDK? Yes.
      assign->cx.push_back(std::make_unique<BinaryOpExpr>(
          "+", getRegisterExpr(B), getRegisterExpr(C)));
      outBlock.add(std::move(assign));
    } else if (op == OpCode::OP_MMBIN) {
      // Meta-method hint, ignored for AST
    } else if (op == OpCode::OP_GETTABUP) {
      // A = Up[B][K[C]] usually
      // In 5.4: A = Up[B] key K[C] (if string?), or val at C?
      // Actually GETTABUP(A, B, C) -> R[A] = Up[B][K[C]]
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(std::make_unique<VariableExpr>(A));

      auto upval = getUpvalueExpr(B);
      // key is constant C?
      // "C index into constants"
      auto key = getConstantExpr(C);
      // Represent as index: upval[key]

      // For now, make a BinaryOp "[" to represent indexing
      assign->cx.push_back(std::make_unique<BinaryOpExpr>("[", std::move(upval),
                                                          std::move(key)));

      outBlock.add(std::move(assign));
    } else if (op == OpCode::OP_MOVE) {
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(std::make_unique<VariableExpr>(A));
      assign->cx.push_back(getRegisterExpr(B));
      outBlock.add(std::move(assign));
    } else if (op == OpCode::OP_CALL || op == OpCode::OP_TAILCALL) {
      // CALL A B C
      // Func at A. Args A+1 to A+B-1.
      // Results to A to A+C-2.
      auto callExpr = std::make_unique<FunctionCallExpr>();
      callExpr->func = getRegisterExpr(A);

      int nArgs = B - 1; // if B=0, uses TOP (variable)
      if (nArgs >= 0) {
        for (int i = 0; i < nArgs; ++i) {
          callExpr->args.push_back(getRegisterExpr(A + 1 + i));
        }
      } else {
        callExpr->args.push_back(std::make_unique<VariableExpr>("(top)"));
      }

      if (C == 0) { // Multiple results
        auto stmt = std::make_unique<FunctionCallStmt>();
        stmt->call = std::move(callExpr);
        outBlock.add(std::move(stmt));
      } else if (C == 1) { // No results
        auto stmt = std::make_unique<FunctionCallStmt>();
        stmt->call = std::move(callExpr);
        outBlock.add(std::move(stmt));
      } else {
        // Returns values
        // TODO: Assignment list
        auto stmt = std::make_unique<FunctionCallStmt>();
        stmt->call = std::move(callExpr);
        outBlock.add(std::move(stmt));
      }
    } else if (op == OpCode::OP_RETURN || op == OpCode::OP_RETURN0 ||
               op == OpCode::OP_RETURN1) {
      auto ret = std::make_unique<ReturnStmt>();
      if (op == OpCode::OP_RETURN1) {
        ret->values.push_back(getRegisterExpr(A));
      } else if (op == OpCode::OP_RETURN) {
        int nRet = B - 1;
        for (int i = 0; i < nRet; ++i) {
          ret->values.push_back(getRegisterExpr(A + i));
        }
      }
      outBlock.add(std::move(ret));
    }
  }
}

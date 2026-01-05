#include "CodeEmitter.h"

void CodeEmitter::emit(const BlockStatement *root) {
  std::cout << "-- Decompiled with Lua 5.4 Decompiler --\n\n";
  for (const auto &stmt : root->statements) {
    emitStatement(stmt.get());
  }
}

void CodeEmitter::emitStatement(const Statement *stmt) {
  if (stmt->getType() == NodeType::Assignment) {
    auto *as = static_cast<const AssignmentStmt *>(stmt);
    if (as->vars.empty())
      return;

    std::cout << "local "
              << static_cast<const VariableExpr *>(as->vars[0].get())->name;
    std::cout << " = ";

    if (!as->cx.empty()) {
      emitExpression(as->cx[0].get());
    }
    std::cout << "\n";
  } else if (stmt->getType() == NodeType::FunctionCall) {
    auto *fc = static_cast<const FunctionCallStmt *>(stmt);
    emitExpression(fc->call->func.get());
    std::cout << "(";
    for (size_t i = 0; i < fc->call->args.size(); ++i) {
      emitExpression(fc->call->args[i].get());
      if (i < fc->call->args.size() - 1)
        std::cout << ", ";
    }
    std::cout << ")\n";
  }
}

void CodeEmitter::emitExpression(const Expression *expr) {
  if (!expr)
    return;

  switch (expr->getType()) {
  case NodeType::Literal: {
    auto *l = static_cast<const LiteralExpr *>(expr);
    if (l->value.type == LType::STRING)
      std::cout << "\"" << l->value.str << "\"";
    else if (l->value.isInteger)
      std::cout << l->value.integer;
    else
      std::cout << l->value.number;
    break;
  }
  case NodeType::Variable: {
    std::cout << static_cast<const VariableExpr *>(expr)->name;
    break;
  }
  case NodeType::BinaryOp: {
    auto *bin = static_cast<const BinaryOpExpr *>(expr);
    // Handle table lookup special case
    if (bin->op == "[") {
      emitExpression(bin->left.get());
      std::cout << "[";
      emitExpression(bin->right.get());
      std::cout << "]";
    } else {
      emitExpression(bin->left.get());
      std::cout << " " << bin->op << " ";
      emitExpression(bin->right.get());
    }
    break;
  }
  case NodeType::FunctionCall: {
    // Nested call logic if needed
    break;
  }
  default:
    break;
  }
}

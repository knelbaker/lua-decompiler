#include "CodeEmitter.h"
#include <cctype>
#include <iostream>
#include <set>

namespace {
bool isValidIdentifier(const std::string &s) {
  if (s.empty())
    return false;
  if (!isalpha(static_cast<unsigned char>(s[0])) && s[0] != '_')
    return false;
  for (char c : s) {
    if (!isalnum(static_cast<unsigned char>(c)) && c != '_')
      return false;
  }
  static const std::set<std::string> keywords = {
      "and",   "break",  "do",   "else", "elseif", "end",  "false",
      "for",   "function", "goto", "if",   "in",     "local", "nil",
      "not",   "or",     "repeat", "return", "then", "true",  "until",
      "while"};
  return keywords.find(s) == keywords.end();
}
} // namespace

void CodeEmitter::emit(const BlockStatement *root) {
  std::cout << "-- Decompiled with Lua 5.4 Decompiler --\n\n";
  for (const auto &stmt : root->statements) {
    emitStatement(stmt.get());
  }
}

void CodeEmitter::emitStatement(const Statement *stmt) {
  if (!stmt)
    return;

  switch (stmt->getType()) {
  case NodeType::Assignment: {
    auto *as = static_cast<const AssignmentStmt *>(stmt);
    if (as->vars.empty())
      return;

    if (as->vars.size() == 1 && as->cx.size() == 1 &&
        as->cx[0]->getType() == NodeType::FunctionDef) {
      auto *fn = static_cast<const FunctionDefExpr *>(as->cx[0].get());
      if (as->isLocal) {
        std::cout << "local function ";
      } else {
        std::cout << "function ";
      }
      emitExpression(as->vars[0].get());
      std::cout << "(";
      for (size_t p = 0; p < fn->params.size(); ++p) {
        std::cout << fn->params[p];
        if (p + 1 < fn->params.size() || fn->isVarArg) {
          std::cout << ", ";
        }
      }
      if (fn->isVarArg) {
        std::cout << "...";
      }
      std::cout << ")\n";
      if (fn->body) {
        for (const auto &s : fn->body->statements) {
          std::cout << "  ";
          emitStatement(s.get());
        }
      }
      std::cout << "end\n";
      break;
    }

    if (as->isLocal) {
      std::cout << "local ";
    }

    for (size_t i = 0; i < as->vars.size(); ++i) {
      emitExpression(as->vars[i].get());
      if (i + 1 < as->vars.size()) {
        std::cout << ", ";
      }
    }

    std::cout << " = ";

    for (size_t i = 0; i < as->cx.size(); ++i) {
      emitExpression(as->cx[i].get());
      if (i + 1 < as->cx.size()) {
        std::cout << ", ";
      }
    }
    std::cout << "\n";
    break;
  }
  case NodeType::FunctionCall: {
    auto *fc = static_cast<const FunctionCallStmt *>(stmt);
    if (fc->call) {
      emitExpression(fc->call.get());
      std::cout << "\n";
    }
    break;
  }
  case NodeType::Return: {
    auto *ret = static_cast<const ReturnStmt *>(stmt);
    std::cout << "return";
    if (!ret->values.empty()) {
      std::cout << " ";
      for (size_t i = 0; i < ret->values.size(); ++i) {
        emitExpression(ret->values[i].get());
        if (i + 1 < ret->values.size()) {
          std::cout << ", ";
        }
      }
    }
    std::cout << "\n";
    break;
  }
  default:
    break;
  }
}

void CodeEmitter::emitExpression(const Expression *expr) {
  if (!expr)
    return;

  switch (expr->getType()) {
  case NodeType::Literal: {
    auto *l = static_cast<const LiteralExpr *>(expr);
    if (l->value.type == LType::NIL) {
      std::cout << "nil";
    } else if (l->value.type == LType::BOOLEAN) {
      std::cout << (l->value.boolean ? "true" : "false");
    } else if (l->value.type == LType::STRING) {
      std::cout << "\"";
      for (char c : l->value.str) {
        if (c == '"')
          std::cout << "\\\"";
        else if (c == '\\')
          std::cout << "\\\\";
        else if (c == '\n')
          std::cout << "\\n";
        else if (c == '\r')
          std::cout << "\\r";
        else if (c == '\t')
          std::cout << "\\t";
        else
          std::cout << c;
      }
      std::cout << "\"";
    } else if (l->value.isInteger) {
      std::cout << l->value.integer;
    } else {
      std::cout << l->value.number;
    }
    break;
  }
  case NodeType::Variable: {
    std::cout << static_cast<const VariableExpr *>(expr)->name;
    break;
  }
  case NodeType::BinaryOp: {
    auto *bin = static_cast<const BinaryOpExpr *>(expr);
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
  case NodeType::UnaryOp: {
    auto *un = static_cast<const UnaryOpExpr *>(expr);
    std::cout << un->op;
    emitExpression(un->expr.get());
    break;
  }
  case NodeType::TableAccess: {
    auto *ta = static_cast<const TableAccessExpr *>(expr);
    // If table is _ENV, check if we can emit as global identifier
    if (ta->table && ta->table->getType() == NodeType::Variable) {
      auto *tblVar = static_cast<const VariableExpr *>(ta->table.get());
      if (tblVar->name == "_ENV" && ta->key &&
          ta->key->getType() == NodeType::Literal) {
        auto *keyLit = static_cast<const LiteralExpr *>(ta->key.get());
        if (keyLit->value.type == LType::STRING &&
            isValidIdentifier(keyLit->value.str)) {
          std::cout << keyLit->value.str;
          break;
        }
      }
    }

    // Check if key is a string identifier -> table.key
    if (ta->key && ta->key->getType() == NodeType::Literal) {
      auto *keyLit = static_cast<const LiteralExpr *>(ta->key.get());
      if (keyLit->value.type == LType::STRING &&
          isValidIdentifier(keyLit->value.str)) {
        emitExpression(ta->table.get());
        std::cout << "." << keyLit->value.str;
        break;
      }
    }

    emitExpression(ta->table.get());
    std::cout << "[";
    emitExpression(ta->key.get());
    std::cout << "]";
    break;
  }
  case NodeType::TableConstructor: {
    auto *tc = static_cast<const TableConstructorExpr *>(expr);
    std::cout << "{";
    for (size_t i = 0; i < tc->fields.size(); ++i) {
      if (tc->fields[i].first) {
        std::cout << "[";
        emitExpression(tc->fields[i].first.get());
        std::cout << "] = ";
      }
      emitExpression(tc->fields[i].second.get());
      if (i + 1 < tc->fields.size()) {
        std::cout << ", ";
      }
    }
    std::cout << "}";
    break;
  }
  case NodeType::FunctionCall: {
    auto *fc = static_cast<const FunctionCallExpr *>(expr);
    emitExpression(fc->func.get());
    std::cout << "(";
    for (size_t i = 0; i < fc->args.size(); ++i) {
      emitExpression(fc->args[i].get());
      if (i + 1 < fc->args.size()) {
        std::cout << ", ";
      }
    }
    std::cout << ")";
    break;
  }
  case NodeType::FunctionDef: {
    auto *fn = static_cast<const FunctionDefExpr *>(expr);
    std::cout << "function(";
    for (size_t p = 0; p < fn->params.size(); ++p) {
      std::cout << fn->params[p];
      if (p + 1 < fn->params.size() || fn->isVarArg) {
        std::cout << ", ";
      }
    }
    if (fn->isVarArg) {
      std::cout << "...";
    }
    std::cout << ")\n";
    if (fn->body) {
      for (const auto &s : fn->body->statements) {
        std::cout << "  ";
        emitStatement(s.get());
      }
    }
    std::cout << "end";
    break;
  }
  case NodeType::Vararg: {
    std::cout << "...";
    break;
  }
  default:
    break;
  }
}

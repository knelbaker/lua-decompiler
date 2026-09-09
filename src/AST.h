#pragma once
#include "BytecodeStructs.h"
#include <memory>
#include <string>
#include <vector>

// AST Node Types
enum class NodeType {
  Block,
  Assignment,
  FunctionCall,
  Return,
  Literal,
  Variable,
  BinaryOp,
  UnaryOp,
  TableAccess,
  TableConstructor,
  FunctionDef,
  Vararg
};

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual NodeType getType() const = 0;
};

struct Expression : public ASTNode {
  virtual std::unique_ptr<Expression> clone() const = 0;
};

struct Statement : public ASTNode {};

struct BlockStatement : public Statement {
  std::vector<std::unique_ptr<Statement>> statements;
  NodeType getType() const override { return NodeType::Block; }

  void add(std::unique_ptr<Statement> stmt) {
    if (stmt) {
      statements.push_back(std::move(stmt));
    }
  }
};

struct LiteralExpr : public Expression {
  LValue value;
  NodeType getType() const override { return NodeType::Literal; }
  LiteralExpr(LValue v) : value(v) {}

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<LiteralExpr>(value);
  }
};

struct VariableExpr : public Expression {
  std::string name;
  int regIndex = -1;
  bool isLocal = false;
  bool isUpvalue = false;

  NodeType getType() const override { return NodeType::Variable; }

  VariableExpr(int reg) : regIndex(reg), isLocal(true), isUpvalue(false) {
    name = "reg_" + std::to_string(reg);
  }

  VariableExpr(const std::string &n, bool isUpval = false)
      : name(n), regIndex(-1), isLocal(!isUpval), isUpvalue(isUpval) {}

  std::unique_ptr<Expression> clone() const override {
    auto copy = std::make_unique<VariableExpr>(name, isUpvalue);
    copy->regIndex = regIndex;
    copy->isLocal = isLocal;
    return copy;
  }
};

struct BinaryOpExpr : public Expression {
  std::string op;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;

  NodeType getType() const override { return NodeType::BinaryOp; }

  BinaryOpExpr(const std::string &op, std::unique_ptr<Expression> l,
               std::unique_ptr<Expression> r)
      : op(op), left(std::move(l)), right(std::move(r)) {}

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<BinaryOpExpr>(
        op, left ? left->clone() : nullptr, right ? right->clone() : nullptr);
  }
};

struct UnaryOpExpr : public Expression {
  std::string op;
  std::unique_ptr<Expression> expr;

  NodeType getType() const override { return NodeType::UnaryOp; }

  UnaryOpExpr(const std::string &op, std::unique_ptr<Expression> e)
      : op(op), expr(std::move(e)) {}

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<UnaryOpExpr>(op, expr ? expr->clone() : nullptr);
  }
};

struct TableAccessExpr : public Expression {
  std::unique_ptr<Expression> table;
  std::unique_ptr<Expression> key;

  NodeType getType() const override { return NodeType::TableAccess; }

  TableAccessExpr(std::unique_ptr<Expression> tbl, std::unique_ptr<Expression> k)
      : table(std::move(tbl)), key(std::move(k)) {}

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<TableAccessExpr>(
        table ? table->clone() : nullptr, key ? key->clone() : nullptr);
  }
};

struct TableConstructorExpr : public Expression {
  std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Expression>>> fields;

  NodeType getType() const override { return NodeType::TableConstructor; }

  std::unique_ptr<Expression> clone() const override {
    auto copy = std::make_unique<TableConstructorExpr>();
    for (const auto &f : fields) {
      copy->fields.emplace_back(
          f.first ? f.first->clone() : nullptr,
          f.second ? f.second->clone() : nullptr);
    }
    return copy;
  }
};

struct FunctionCallExpr : public Expression {
  std::unique_ptr<Expression> func;
  std::vector<std::unique_ptr<Expression>> args;

  NodeType getType() const override { return NodeType::FunctionCall; }

  std::unique_ptr<Expression> clone() const override {
    auto copy = std::make_unique<FunctionCallExpr>();
    copy->func = func ? func->clone() : nullptr;
    for (const auto &a : args) {
      copy->args.push_back(a ? a->clone() : nullptr);
    }
    return copy;
  }
};

struct FunctionDefExpr : public Expression {
  std::vector<std::string> params;
  bool isVarArg = false;
  std::unique_ptr<BlockStatement> body;

  NodeType getType() const override { return NodeType::FunctionDef; }

  std::unique_ptr<Expression> clone() const override {
    auto copy = std::make_unique<FunctionDefExpr>();
    copy->params = params;
    copy->isVarArg = isVarArg;
    // Body is shared or transferred
    return copy;
  }
};

struct VarargExpr : public Expression {
  NodeType getType() const override { return NodeType::Vararg; }

  std::unique_ptr<Expression> clone() const override {
    return std::make_unique<VarargExpr>();
  }
};

struct AssignmentStmt : public Statement {
  bool isLocal = false;
  std::vector<std::unique_ptr<Expression>> vars;
  std::vector<std::unique_ptr<Expression>> cx;

  NodeType getType() const override { return NodeType::Assignment; }
};

struct FunctionCallStmt : public Statement {
  std::unique_ptr<FunctionCallExpr> call;
  NodeType getType() const override { return NodeType::FunctionCall; }
};

struct ReturnStmt : public Statement {
  std::vector<std::unique_ptr<Expression>> values;
  NodeType getType() const override { return NodeType::Return; }
};

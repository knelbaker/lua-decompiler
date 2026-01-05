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
  UnaryOp
};

struct ASTNode {
  virtual ~ASTNode() = default;
  virtual NodeType getType() const = 0;
};

struct Expression : public ASTNode {};

struct Statement : public ASTNode {};

struct BlockStatement : public Statement {
  std::vector<std::unique_ptr<Statement>> statements;
  NodeType getType() const override { return NodeType::Block; }

  void add(std::unique_ptr<Statement> stmt) {
    statements.push_back(std::move(stmt));
  }
};

struct LiteralExpr : public Expression {
  LValue value;
  NodeType getType() const override { return NodeType::Literal; }
  LiteralExpr(LValue v) : value(v) {}
};

struct VariableExpr : public Expression {
  std::string name;
  int regIndex; // For tracking
  bool isLocal;
  bool isUpvalue;

  NodeType getType() const override { return NodeType::Variable; }
  VariableExpr(int reg) : regIndex(reg), isLocal(true), isUpvalue(false) {
    name = "reg_" + std::to_string(reg);
  }
  VariableExpr(std::string n)
      : name(n), regIndex(-1), isLocal(false), isUpvalue(true) {}
};

struct BinaryOpExpr : public Expression {
  std::string op;
  std::unique_ptr<Expression> left;
  std::unique_ptr<Expression> right;

  NodeType getType() const override { return NodeType::BinaryOp; }
  BinaryOpExpr(std::string op, std::unique_ptr<Expression> l,
               std::unique_ptr<Expression> r)
      : op(op), left(std::move(l)), right(std::move(r)) {}
};

struct AssignmentStmt : public Statement {
  std::vector<std::unique_ptr<VariableExpr>> vars;
  std::vector<std::unique_ptr<Expression>> cx; // expressions

  NodeType getType() const override { return NodeType::Assignment; }
};

struct FunctionCallExpr : public Expression {
  std::unique_ptr<Expression> func;
  std::vector<std::unique_ptr<Expression>> args;

  NodeType getType() const override { return NodeType::FunctionCall; }
};

struct FunctionCallStmt : public Statement {
  std::unique_ptr<FunctionCallExpr> call;
  NodeType getType() const override {
    return NodeType::FunctionCall;
  } // Reusing type or separating?
};

struct ReturnStmt : public Statement {
  std::vector<std::unique_ptr<Expression>> values;
  NodeType getType() const override { return NodeType::Return; }
};

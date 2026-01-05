#pragma once
#include "AST.h"
#include <iostream>

class CodeEmitter {
public:
  void emit(const BlockStatement *root);

private:
  void emitStatement(const Statement *stmt);
  void emitExpression(const Expression *expr);
};

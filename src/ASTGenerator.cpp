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

int ASTGenerator::getLocalSlot(size_t varIndex) const {
  const auto &loc = proto.locVars[varIndex];
  int slot = 0;
  for (size_t j = 0; j < varIndex; ++j) {
    if (proto.locVars[j].startPC <= loc.startPC &&
        loc.startPC < proto.locVars[j].endPC) {
      slot++;
    }
  }
  return slot;
}

const LocalVarInfo *ASTGenerator::findActiveLocal(int reg, int pc) const {
  for (size_t i = 0; i < proto.locVars.size(); ++i) {
    const auto &loc = proto.locVars[i];
    if (loc.startPC <= pc && pc < loc.endPC) {
      if (getLocalSlot(i) == reg) {
        return &loc;
      }
    }
  }
  return nullptr;
}

std::unique_ptr<Expression> ASTGenerator::getConstantExpr(int kIdx) {
  if (kIdx >= 0 && kIdx < static_cast<int>(proto.k.size())) {
    return std::make_unique<LiteralExpr>(proto.k[kIdx]);
  }
  return std::make_unique<VariableExpr>("(invalid const)");
}

std::unique_ptr<Expression> ASTGenerator::getUpvalueExpr(int uIdx) {
  if (uIdx >= 0 && uIdx < static_cast<int>(proto.upvalues.size())) {
    const std::string &name = proto.upvalues[uIdx].name;
    return std::make_unique<VariableExpr>(
        name.empty() ? "_UPVAL_" + std::to_string(uIdx) : name, true);
  }
  return std::make_unique<VariableExpr>("_ENV", true);
}

std::unique_ptr<Expression> ASTGenerator::getRegisterExpr(int reg, int pc) {
  const LocalVarInfo *loc = findActiveLocal(reg, pc);
  if (loc && !loc->name.empty()) {
    return std::make_unique<VariableExpr>(loc->name);
  }

  if (registers.count(reg) && registers[reg]) {
    return registers[reg]->clone();
  }

  return std::make_unique<VariableExpr>(reg);
}

void ASTGenerator::checkPendingLocalDefs(int pc, BlockStatement &outBlock) {
  for (size_t i = 0; i < proto.locVars.size(); ++i) {
    const auto &loc = proto.locVars[i];
    if (declaredLocals.find(&loc) == declaredLocals.end() &&
        loc.startPC == pc + 1) {
      int slot = getLocalSlot(i);
      if (registers.count(slot) && registers[slot]) {
        declaredLocals.insert(&loc);
        auto assign = std::make_unique<AssignmentStmt>();
        assign->isLocal = true;
        assign->vars.push_back(std::make_unique<VariableExpr>(loc.name));
        assign->cx.push_back(std::move(registers[slot]));
        outBlock.add(std::move(assign));
        registers[slot] = std::make_unique<VariableExpr>(loc.name);
      }
    }
  }
}

void ASTGenerator::setRegister(int reg, int pc, std::unique_ptr<Expression> expr,
                              BlockStatement &outBlock) {
  const LocalVarInfo *activeLoc = findActiveLocal(reg, pc);
  if (activeLoc && !activeLoc->name.empty() &&
      declaredLocals.find(activeLoc) != declaredLocals.end()) {
    auto assign = std::make_unique<AssignmentStmt>();
    assign->isLocal = false;
    assign->vars.push_back(std::make_unique<VariableExpr>(activeLoc->name));
    assign->cx.push_back(std::move(expr));
    outBlock.add(std::move(assign));
    registers[reg] = std::make_unique<VariableExpr>(activeLoc->name);
    return;
  }

  registers[reg] = std::move(expr);
}

void ASTGenerator::processBlock(BasicBlock *block, BlockStatement &outBlock) {
  for (size_t i = 0; i < block->instructions.size(); ++i) {
    int pc = block->startPC + static_cast<int>(i);
    Instruction inst = block->instructions[i];
    OpCode op = inst.getOpCode();

    int A = inst.getA();
    int B = inst.getB();
    int C = inst.getC();
    int Bx = inst.getBx();
    int sBx = inst.getsBx();
    int sC = inst.getsC();

    switch (op) {
    case OpCode::OP_MOVE: {
      setRegister(A, pc, getRegisterExpr(B, pc), outBlock);
      break;
    }
    case OpCode::OP_LOADI: {
      LValue v{};
      v.type = LType::NUMBER;
      v.isInteger = true;
      v.integer = sBx;
      v.number = static_cast<double>(sBx);
      setRegister(A, pc, std::make_unique<LiteralExpr>(v), outBlock);
      break;
    }
    case OpCode::OP_LOADF: {
      LValue v{};
      v.type = LType::NUMBER;
      v.isInteger = false;
      v.number = static_cast<double>(sBx);
      setRegister(A, pc, std::make_unique<LiteralExpr>(v), outBlock);
      break;
    }
    case OpCode::OP_LOADK: {
      setRegister(A, pc, getConstantExpr(Bx), outBlock);
      break;
    }
    case OpCode::OP_LOADKX: {
      int extraAx = (pc + 1 < static_cast<int>(proto.code.size()))
                        ? proto.code[pc + 1].getAx()
                        : 0;
      setRegister(A, pc, getConstantExpr(extraAx), outBlock);
      break;
    }
    case OpCode::OP_LOADFALSE:
    case OpCode::OP_LFALSESKIP: {
      LValue v{};
      v.type = LType::BOOLEAN;
      v.boolean = false;
      setRegister(A, pc, std::make_unique<LiteralExpr>(v), outBlock);
      break;
    }
    case OpCode::OP_LOADTRUE: {
      LValue v{};
      v.type = LType::BOOLEAN;
      v.boolean = true;
      setRegister(A, pc, std::make_unique<LiteralExpr>(v), outBlock);
      break;
    }
    case OpCode::OP_LOADNIL: {
      for (int r = 0; r <= B; ++r) {
        LValue v{};
        v.type = LType::NIL;
        setRegister(A + r, pc, std::make_unique<LiteralExpr>(v), outBlock);
      }
      break;
    }
    case OpCode::OP_GETUPVAL: {
      setRegister(A, pc, getUpvalueExpr(B), outBlock);
      break;
    }
    case OpCode::OP_SETUPVAL: {
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(getUpvalueExpr(B));
      assign->cx.push_back(getRegisterExpr(A, pc));
      outBlock.add(std::move(assign));
      break;
    }
    case OpCode::OP_GETTABUP: {
      auto upval = getUpvalueExpr(B);
      auto key = getConstantExpr(C);
      setRegister(A, pc,
                  std::make_unique<TableAccessExpr>(std::move(upval),
                                                    std::move(key)),
                  outBlock);
      break;
    }
    case OpCode::OP_SETTABUP: {
      auto upval = getUpvalueExpr(A);
      auto key = getConstantExpr(B);
      auto val = inst.getk() ? getConstantExpr(C) : getRegisterExpr(C, pc);
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(
          std::make_unique<TableAccessExpr>(std::move(upval), std::move(key)));
      assign->cx.push_back(std::move(val));
      outBlock.add(std::move(assign));
      break;
    }
    case OpCode::OP_GETTABLE: {
      auto tbl = getRegisterExpr(B, pc);
      auto key = getRegisterExpr(C, pc);
      setRegister(A, pc,
                  std::make_unique<TableAccessExpr>(std::move(tbl),
                                                    std::move(key)),
                  outBlock);
      break;
    }
    case OpCode::OP_SETTABLE: {
      auto tbl = getRegisterExpr(A, pc);
      auto key = getRegisterExpr(B, pc);
      auto val = inst.getk() ? getConstantExpr(C) : getRegisterExpr(C, pc);
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(
          std::make_unique<TableAccessExpr>(std::move(tbl), std::move(key)));
      assign->cx.push_back(std::move(val));
      outBlock.add(std::move(assign));
      break;
    }
    case OpCode::OP_GETI: {
      auto tbl = getRegisterExpr(B, pc);
      LValue kv{};
      kv.type = LType::NUMBER;
      kv.isInteger = true;
      kv.integer = C;
      kv.number = static_cast<double>(C);
      setRegister(A, pc,
                  std::make_unique<TableAccessExpr>(
                      std::move(tbl), std::make_unique<LiteralExpr>(kv)),
                  outBlock);
      break;
    }
    case OpCode::OP_SETI: {
      auto tbl = getRegisterExpr(A, pc);
      LValue kv{};
      kv.type = LType::NUMBER;
      kv.isInteger = true;
      kv.integer = B;
      kv.number = static_cast<double>(B);
      auto key = std::make_unique<LiteralExpr>(kv);
      auto val = inst.getk() ? getConstantExpr(C) : getRegisterExpr(C, pc);
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(
          std::make_unique<TableAccessExpr>(std::move(tbl), std::move(key)));
      assign->cx.push_back(std::move(val));
      outBlock.add(std::move(assign));
      break;
    }
    case OpCode::OP_GETFIELD: {
      auto tbl = getRegisterExpr(B, pc);
      auto key = getConstantExpr(C);
      setRegister(A, pc,
                  std::make_unique<TableAccessExpr>(std::move(tbl),
                                                    std::move(key)),
                  outBlock);
      break;
    }
    case OpCode::OP_SETFIELD: {
      auto tbl = getRegisterExpr(A, pc);
      auto key = getConstantExpr(B);
      auto val = inst.getk() ? getConstantExpr(C) : getRegisterExpr(C, pc);
      auto assign = std::make_unique<AssignmentStmt>();
      assign->vars.push_back(
          std::make_unique<TableAccessExpr>(std::move(tbl), std::move(key)));
      assign->cx.push_back(std::move(val));
      outBlock.add(std::move(assign));
      break;
    }
    case OpCode::OP_NEWTABLE: {
      setRegister(A, pc, std::make_unique<TableConstructorExpr>(), outBlock);
      break;
    }
    case OpCode::OP_ADD:
    case OpCode::OP_SUB:
    case OpCode::OP_MUL:
    case OpCode::OP_DIV:
    case OpCode::OP_IDIV:
    case OpCode::OP_MOD:
    case OpCode::OP_POW:
    case OpCode::OP_BAND:
    case OpCode::OP_BOR:
    case OpCode::OP_BXOR:
    case OpCode::OP_SHL:
    case OpCode::OP_SHR: {
      std::string opStr = "+";
      if (op == OpCode::OP_SUB) opStr = "-";
      else if (op == OpCode::OP_MUL) opStr = "*";
      else if (op == OpCode::OP_DIV) opStr = "/";
      else if (op == OpCode::OP_IDIV) opStr = "//";
      else if (op == OpCode::OP_MOD) opStr = "%";
      else if (op == OpCode::OP_POW) opStr = "^";
      else if (op == OpCode::OP_BAND) opStr = "&";
      else if (op == OpCode::OP_BOR) opStr = "|";
      else if (op == OpCode::OP_BXOR) opStr = "~";
      else if (op == OpCode::OP_SHL) opStr = "<<";
      else if (op == OpCode::OP_SHR) opStr = ">>";

      setRegister(A, pc,
                  std::make_unique<BinaryOpExpr>(
                      opStr, getRegisterExpr(B, pc), getRegisterExpr(C, pc)),
                  outBlock);
      break;
    }
    case OpCode::OP_ADDK:
    case OpCode::OP_SUBK:
    case OpCode::OP_MULK:
    case OpCode::OP_DIVK:
    case OpCode::OP_IDIVK:
    case OpCode::OP_MODK:
    case OpCode::OP_POWK:
    case OpCode::OP_BANDK:
    case OpCode::OP_BORK:
    case OpCode::OP_BXORK: {
      std::string opStr = "+";
      if (op == OpCode::OP_SUBK) opStr = "-";
      else if (op == OpCode::OP_MULK) opStr = "*";
      else if (op == OpCode::OP_DIVK) opStr = "/";
      else if (op == OpCode::OP_IDIVK) opStr = "//";
      else if (op == OpCode::OP_MODK) opStr = "%";
      else if (op == OpCode::OP_POWK) opStr = "^";
      else if (op == OpCode::OP_BANDK) opStr = "&";
      else if (op == OpCode::OP_BORK) opStr = "|";
      else if (op == OpCode::OP_BXORK) opStr = "~";

      setRegister(A, pc,
                  std::make_unique<BinaryOpExpr>(
                      opStr, getRegisterExpr(B, pc), getConstantExpr(C)),
                  outBlock);
      break;
    }
    case OpCode::OP_ADDI: {
      LValue sv{};
      sv.type = LType::NUMBER;
      sv.isInteger = true;
      sv.integer = sC;
      sv.number = static_cast<double>(sC);
      setRegister(A, pc,
                  std::make_unique<BinaryOpExpr>(
                      "+", getRegisterExpr(B, pc),
                      std::make_unique<LiteralExpr>(sv)),
                  outBlock);
      break;
    }
    case OpCode::OP_SHRI: {
      std::string opStr = ">>";
      long long shiftVal = sC;
      if (shiftVal < 0) {
        opStr = "<<";
        shiftVal = -shiftVal;
      }
      LValue sv{};
      sv.type = LType::NUMBER;
      sv.isInteger = true;
      sv.integer = shiftVal;
      sv.number = static_cast<double>(shiftVal);
      setRegister(A, pc,
                  std::make_unique<BinaryOpExpr>(
                      opStr, getRegisterExpr(B, pc),
                      std::make_unique<LiteralExpr>(sv)),
                  outBlock);
      break;
    }
    case OpCode::OP_SHLI: {
      std::string opStr = "<<";
      long long shiftVal = sC;
      if (shiftVal < 0) {
        opStr = ">>";
        shiftVal = -shiftVal;
      }
      LValue sv{};
      sv.type = LType::NUMBER;
      sv.isInteger = true;
      sv.integer = shiftVal;
      sv.number = static_cast<double>(shiftVal);
      setRegister(A, pc,
                  std::make_unique<BinaryOpExpr>(
                      opStr, std::make_unique<LiteralExpr>(sv),
                      getRegisterExpr(B, pc)),
                  outBlock);
      break;
    }
    case OpCode::OP_UNM:
    case OpCode::OP_BNOT:
    case OpCode::OP_NOT:
    case OpCode::OP_LEN: {
      std::string opStr = "-";
      if (op == OpCode::OP_BNOT) opStr = "~";
      else if (op == OpCode::OP_NOT) opStr = "not ";
      else if (op == OpCode::OP_LEN) opStr = "#";

      setRegister(A, pc,
                  std::make_unique<UnaryOpExpr>(opStr, getRegisterExpr(B, pc)),
                  outBlock);
      break;
    }
    case OpCode::OP_CONCAT: {
      std::unique_ptr<Expression> current = getRegisterExpr(A, pc);
      for (int r = 1; r < B; ++r) {
        current = std::make_unique<BinaryOpExpr>(
            "..", std::move(current), getRegisterExpr(A + r, pc));
      }
      setRegister(A, pc, std::move(current), outBlock);
      break;
    }
    case OpCode::OP_CALL:
    case OpCode::OP_TAILCALL: {
      auto callExpr = std::make_unique<FunctionCallExpr>();
      callExpr->func = getRegisterExpr(A, pc);

      int nArgs = B - 1;
      if (nArgs >= 0) {
        for (int arg = 0; arg < nArgs; ++arg) {
          callExpr->args.push_back(getRegisterExpr(A + 1 + arg, pc));
        }
      } else {
        callExpr->args.push_back(std::make_unique<VarargExpr>());
      }

      if (op == OpCode::OP_TAILCALL) {
        auto ret = std::make_unique<ReturnStmt>();
        ret->values.push_back(std::move(callExpr));
        outBlock.add(std::move(ret));
      } else if (C == 1) { // 0 return values
        auto stmt = std::make_unique<FunctionCallStmt>();
        stmt->call = std::move(callExpr);
        outBlock.add(std::move(stmt));
      } else {
        setRegister(A, pc, std::move(callExpr), outBlock);
      }
      break;
    }
    case OpCode::OP_RETURN0: {
      if (pc + 1 < static_cast<int>(proto.code.size())) {
        outBlock.add(std::make_unique<ReturnStmt>());
      }
      break;
    }
    case OpCode::OP_RETURN1: {
      auto ret = std::make_unique<ReturnStmt>();
      ret->values.push_back(getRegisterExpr(A, pc));
      outBlock.add(std::move(ret));
      break;
    }
    case OpCode::OP_RETURN: {
      int nRet = B - 1;
      if (nRet > 0 || pc + 1 < static_cast<int>(proto.code.size())) {
        auto ret = std::make_unique<ReturnStmt>();
        for (int r = 0; r < nRet; ++r) {
          ret->values.push_back(getRegisterExpr(A + r, pc));
        }
        outBlock.add(std::move(ret));
      }
      break;
    }
    case OpCode::OP_SELF: {
      setRegister(A + 1, pc, getRegisterExpr(B, pc), outBlock);
      auto key = inst.getk() ? getConstantExpr(C) : getRegisterExpr(C, pc);
      setRegister(A, pc,
                  std::make_unique<TableAccessExpr>(getRegisterExpr(B, pc),
                                                    std::move(key)),
                  outBlock);
      break;
    }
    case OpCode::OP_CLOSURE: {
      if (Bx >= 0 && Bx < static_cast<int>(proto.p.size())) {
        const auto &innerProto = proto.p[Bx];
        Decompiler innerDecompiler(*innerProto);
        innerDecompiler.analyzeCFG();
        ASTGenerator innerGen(*innerProto, innerDecompiler.getBlocks());
        auto innerBody = innerGen.generate();

        auto fnDef = std::make_unique<FunctionDefExpr>();
        fnDef->isVarArg = (innerProto->isVarArg != 0);
        fnDef->body = std::move(innerBody);

        for (int p = 0; p < innerProto->numParams; ++p) {
          std::string pName = "arg_" + std::to_string(p);
          if (p < static_cast<int>(innerProto->locVars.size()) &&
              !innerProto->locVars[p].name.empty()) {
            pName = innerProto->locVars[p].name;
          }
          fnDef->params.push_back(pName);
        }

        setRegister(A, pc, std::move(fnDef), outBlock);
      }
      break;
    }
    case OpCode::OP_SETLIST: {
      if (registers.count(A) && registers[A] &&
          registers[A]->getType() == NodeType::TableConstructor) {
        auto *tc = static_cast<TableConstructorExpr *>(registers[A].get());
        for (int r = 1; r <= B; ++r) {
          tc->fields.emplace_back(nullptr, getRegisterExpr(A + r, pc));
        }
      } else {
        for (int r = 1; r <= B; ++r) {
          LValue kv{};
          kv.type = LType::NUMBER;
          kv.isInteger = true;
          kv.integer = C + r;
          kv.number = static_cast<double>(C + r);
          auto assign = std::make_unique<AssignmentStmt>();
          assign->vars.push_back(std::make_unique<TableAccessExpr>(
              getRegisterExpr(A, pc), std::make_unique<LiteralExpr>(kv)));
          assign->cx.push_back(getRegisterExpr(A + r, pc));
          outBlock.add(std::move(assign));
        }
      }
      break;
    }
    case OpCode::OP_VARARG: {
      setRegister(A, pc, std::make_unique<VarargExpr>(), outBlock);
      break;
    }
    case OpCode::OP_MMBIN:
    case OpCode::OP_MMBINI:
    case OpCode::OP_MMBINK:
    case OpCode::OP_VARARGPREP:
    case OpCode::OP_EXTRAARG:
    default:
      break;
    }

    checkPendingLocalDefs(pc, outBlock);
  }
}

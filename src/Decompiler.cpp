#include "Decompiler.h"
#include <algorithm>
#include <iostream>
#include <set>

Decompiler::Decompiler(const Proto &proto) : proto(proto) {}

void Decompiler::analyzeCFG() {
  createBasicBlocks();
  linkBasicBlocks();
}

void Decompiler::createBasicBlocks() {
  std::set<int> leaders;
  leaders.insert(0); // First instruction is always a leader

  // Identify leaders
  for (size_t pc = 0; pc < proto.code.size(); ++pc) {
    Instruction inst = proto.code[pc];
    OpCode op = inst.getOpCode();

    // Target of a jump is a leader
    if (op == OpCode::OP_JMP || op == OpCode::OP_FORLOOP ||
        op == OpCode::OP_FORPREP || op == OpCode::OP_TFORPREP ||
        op == OpCode::OP_TFORLOOP) {

      int target = getJumpTarget(pc, inst);
      if (target >= 0 && target < static_cast<int>(proto.code.size())) {
        leaders.insert(target);
      }

      // Instruction following an unconditional/loop jump is a leader
      if (pc + 1 < proto.code.size()) {
        leaders.insert(pc + 1);
      }
    }

    // Conditional test instructions skip the next instruction (which is usually a JMP)
    if (op == OpCode::OP_EQ || op == OpCode::OP_LT || op == OpCode::OP_LE ||
        op == OpCode::OP_EQK || op == OpCode::OP_EQI || op == OpCode::OP_LTI ||
        op == OpCode::OP_LEI || op == OpCode::OP_GTI || op == OpCode::OP_GEI ||
        op == OpCode::OP_TEST || op == OpCode::OP_TESTSET ||
        op == OpCode::OP_LFALSESKIP) {

      if (pc + 1 < proto.code.size()) {
        leaders.insert(pc + 1);
      }
      if (pc + 2 < proto.code.size()) {
        leaders.insert(pc + 2);
      }
    }

    // Return instructions terminate block, so next is leader
    if (op == OpCode::OP_RETURN || op == OpCode::OP_RETURN0 ||
        op == OpCode::OP_RETURN1 || op == OpCode::OP_TAILCALL) {
      if (pc + 1 < proto.code.size()) {
        leaders.insert(pc + 1);
      }
    }
  }

  // Create blocks from leaders
  int currentID = 0;
  auto it = leaders.begin();
  while (it != leaders.end()) {
    int start = *it;
    it++;
    int end = (it != leaders.end()) ? *it : static_cast<int>(proto.code.size());

    auto block = std::make_unique<BasicBlock>();
    block->id = currentID++;
    block->startPC = start;
    block->endPC = end;

    for (int i = start; i < end; ++i) {
      block->instructions.push_back(proto.code[i]);
    }

    sequentialBlocks.push_back(block.get());
    blocks[start] = std::move(block);
  }
}

void Decompiler::linkBasicBlocks() {
  for (auto *block : sequentialBlocks) {
    if (block->instructions.empty())
      continue;

    int lastPC = block->endPC - 1;
    Instruction lastInst = proto.code[lastPC];
    OpCode op = lastInst.getOpCode();

    // Handle control flow
    if (op == OpCode::OP_JMP) {
      int target = getJumpTarget(lastPC, lastInst);
      if (blocks.count(target)) {
        block->successors.push_back(blocks[target]->id);
        blocks[target]->predecessors.push_back(block->id);
      }
    } else if (op == OpCode::OP_RETURN || op == OpCode::OP_RETURN0 ||
               op == OpCode::OP_RETURN1 || op == OpCode::OP_TAILCALL) {
      // No successors (terminator)
    } else {
      // Fallthrough
      if (block->endPC < static_cast<int>(proto.code.size())) {
        int nextPC = block->endPC;
        if (blocks.count(nextPC)) {
          block->successors.push_back(blocks[nextPC]->id);
          blocks[nextPC]->predecessors.push_back(block->id);
        }
      }

      // Loops or branches that jump in addition to fallthrough
      if (op == OpCode::OP_FORLOOP || op == OpCode::OP_FORPREP ||
          op == OpCode::OP_TFORPREP || op == OpCode::OP_TFORLOOP) {
        int target = getJumpTarget(lastPC, lastInst);
        if (blocks.count(target)) {
          block->successors.push_back(blocks[target]->id);
          blocks[target]->predecessors.push_back(block->id);
        }
      }
    }
  }
}

int Decompiler::getJumpTarget(int pc, Instruction inst) {
  OpCode op = inst.getOpCode();
  if (op == OpCode::OP_JMP) {
    return pc + 1 + inst.getsJ();
  } else if (op == OpCode::OP_FORLOOP) {
    return pc + 1 - inst.getBx();
  } else if (op == OpCode::OP_FORPREP) {
    return pc + 1 + inst.getBx() + 1;
  } else if (op == OpCode::OP_TFORPREP) {
    return pc + 1 + inst.getBx();
  } else if (op == OpCode::OP_TFORLOOP) {
    return pc + 1 - inst.getBx();
  }

  return -1;
}

void Decompiler::generateLua() {
  std::cout << "-- Decompiled CFG --\n";
  for (const auto *block : sequentialBlocks) {
    std::cout << "Block " << block->id << " [" << block->startPC << " - "
              << block->endPC << ")\n";
    std::cout << "  Succs: ";
    for (int s : block->successors)
      std::cout << s << " ";
    std::cout << "\n";

    for (const auto &inst : block->instructions) {
      std::cout << "    " << getOpCodeName(inst.getOpCode()) << "\n";
    }
  }
}

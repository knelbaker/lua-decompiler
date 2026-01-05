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
    if (op == OpCode::OP_JMP || op == OpCode::OP_EQ || op == OpCode::OP_LT ||
        op == OpCode::OP_LE || op == OpCode::OP_TEST ||
        op == OpCode::OP_TESTSET || op == OpCode::OP_FORLOOP ||
        op == OpCode::OP_TFORLOOP) {

      int target = getJumpTarget(pc, inst);
      if (target >= 0 && target < (int)proto.code.size()) {
        leaders.insert(target);
      }

      // Instruction following a jump/branch is a leader
      if (pc + 1 < proto.code.size()) {
        leaders.insert(pc + 1);
      }
    }

    // Return instructions terminate block, so next is leader
    if (op == OpCode::OP_RETURN || op == OpCode::OP_RETURN0 ||
        op == OpCode::OP_RETURN1) {
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
    int end = (it != leaders.end()) ? *it : proto.code.size();

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
               op == OpCode::OP_RETURN1) {
      // No successors
    } else {
      // Fallthrough
      if (block->endPC < (int)proto.code.size()) {
        int nextPC = block->endPC;
        if (blocks.count(nextPC)) {
          block->successors.push_back(blocks[nextPC]->id);
          blocks[nextPC]->predecessors.push_back(block->id);
        }
      }

      // Conditional branches also have a jump target + fallthrough
      if (op == OpCode::OP_EQ || op == OpCode::OP_LT || op == OpCode::OP_LE ||
          op == OpCode::OP_TEST || op == OpCode::OP_TESTSET ||
          op == OpCode::OP_FORLOOP) { // FORLOOP jumps back if loop continues

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
  // Lua 5.4 JMP uses sJ (25 bit) or sBx??
  // Wait, OP_JMP in 5.4 uses sJ.
  // Conditional uses k? No, usually follows JMP?
  // Actually Lua 5.4 branch instructions often skip the NEXT instruction if
  // condition is false. But the generic Jump uses offset.

  if (op == OpCode::OP_JMP) {
    return pc + 1 + inst.getsJ();
  } else if (op == OpCode::OP_FORLOOP) {
    return pc + 1 + inst.getsBx();
  }

  // For test jumps, standard Lua 5.4 usually encodes jump offset in the
  // instruction? Or is it "Skip next instruction"? "OP_EQ(A,B,k) if ((RK(B) ==
  // RK(C)) ~= k) then pc++" So if condition fails, valid. If condition passes,
  // skip next. Usually next instruction is a JMP. So the block logic above
  // handles "Fallthrough" which is correct. The explicit jump target is only
  // for instructions that embed a jump.

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

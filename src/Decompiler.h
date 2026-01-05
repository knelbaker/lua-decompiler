#pragma once
#include "BytecodeStructs.h"
#include <map>
#include <memory>
#include <vector>

struct BasicBlock {
  int id;
  int startPC;
  int endPC; // Exclusive
  std::vector<Instruction> instructions;

  std::vector<int> successors;
  std::vector<int> predecessors;

  // For visualization/debug
  bool isLoopHeader = false;
};

class Decompiler {
public:
  Decompiler(const Proto &proto);

  void analyzeCFG();
  void generateLua();
  const std::vector<BasicBlock *> &getBlocks() const {
    return sequentialBlocks;
  }

private:
  const Proto &proto;
  std::map<int, std::unique_ptr<BasicBlock>> blocks; // Map startPC -> Block
  std::vector<BasicBlock *> sequentialBlocks;        // Ordered by startPC

  void createBasicBlocks();
  void linkBasicBlocks();
  bool isLeader(int pc);
  int getJumpTarget(int pc, Instruction inst);
};

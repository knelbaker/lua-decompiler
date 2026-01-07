# Lua 5.4 Decompiler

A functional decompiler for Lua 5.4 bytecode, written in C++.

## Overview

This tool analyzes compiled Lua 5.4 binary chunks (`.luac`) and reconstructs the original Lua source code. It features a complete pipeline including a bytecode parser, disassembler, control flow graph (CFG) generator, abstract syntax tree (AST) builder, and code emitter.

## Features

*   **Bytecode Parser**: Robustly parses Lua 5.4 headers, variable-length integers (VarInts), and instruction formats.
*   **Disassembler**: supports all standard Lua 5.4 opcodes (e.g., `MOVE`, `LOADK`, `VARARGPREP`).
*   **CFG Analysis**: Reconstructs basic blocks and control flow edges.
*   **AST Generation**: Converts register-based virtual machine instructions into high-level AST nodes (Assignments, Function Calls, Binary Operations).
*   **Variable Naming**: Recovers local variable names from debug information when available.

## Build Instructions

Requirements:
*   CMake (3.10+)
*   C++17 compatible compiler (GCC/Clang)

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Run the decompiler on a compiled Lua file:

```bash
./lua_decompiler path/to/script.luac
```

Example output:

```lua
-- Decompiled with Lua 5.4 Decompiler --

local print = _UPVAL_0["print"]
local msg = "Hello from Lua 5.4!"
print(msg)
```

## Project Structure

*   `src/BytecodeStructs.h` - Data structures for Lua headers and Prototypes.
*   `src/Parser.cpp` - Binary stream parsing logic.
*   `src/Disassembler.cpp` - Instruction decoding and formatted output.
*   `src/Decompiler.cpp` - CFG construction.
*   `src/ASTGenerator.cpp` - AST creation logic.
*   `src/CodeEmitter.cpp` - Lua source code generation.

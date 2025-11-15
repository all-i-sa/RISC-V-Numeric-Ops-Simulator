# RISC-V Numeric Ops Simulator

This repo has two main parts:

1. **Numeric Operations**

   This part of the project works directly with bits instead of using symbols
   like + and -.

   It can:
    - Turn regular numbers into 32-bit “computer form” and back
    - Add and subtract numbers and tell if the result is negative,
      zero, or if it overflowed
    - Multiply and divide numbers using bit logic
    - Store and work with 32-bit floats, including adding,
      subtracting, and multiplying them

2. **RISC-V 32 CPU Simulator**

   This part of the project is a 32-bit RISC-V–style CPU in C++.
   The CPU runs one instruction at a time in a loop.

   It has:
    - 32 integer registers (x0–x31), a program counter (`pc`),
      and a byte array for memory
    - A `step` function that fetches an instruction, decodes it,
      runs it, and updates the program counter
    - Support for the main instruction types I needed:
      arithmetic, logic, shifts, loads and stores, branches, jumps,
      and upper-immediate instructions

In the numeric operations, the code works directly with bits.
The CPU part uses normal 32-bit integers in C++ to keep the
instruction logic easier to read and test.

Both parts are written in C++ and tested with GoogleTest.

---

## How to Build and Run Tests

### Requirements

- C++20 compiler
- CMake
- GoogleTest (pulled in automatically by CMake)

### Build steps

From the repo root:

```bash
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
cmake --build . --target core_tests -j
```

### Run all tests

From inside cmake-build-debug:

```bash
./core_tests
```

To run only the CPU tests:
```bash 
./core_tests --gtest_filter=Cpu*
```

---

## Files and Folders

The main folders and files in this repo:

```text
src/
  core/
    bitvec.hpp / bitvec.cpp      // bit helpers
    twos.hpp   / twos.cpp        // two's complement helpers
    alu.hpp    / alu.cpp         // integer add, sub, and flags
    shifter.hpp/ shifter.cpp     // shifts
    mdu.hpp    / mdu.cpp         // multiply and divide
    f32.hpp    / f32.cpp         // float32 bits and math
    rv32_cpu.hpp / rv32_cpu.cpp  // RISC-V 32 CPU

tests/
  bitvec_tests.cpp
  twos_tests.cpp
  alu_tests.cpp
  shifter_tests.cpp
  mdu_tests.cpp
  float_tests.cpp
  cpu_tests.cpp

CMakeLists.txt        // build setup
README.md             // this file
AI_USAGE.md           // ow I used AI
ai_report.json        // AI-tagged lines

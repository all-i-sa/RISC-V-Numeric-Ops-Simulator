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

## Testing
GoogleTest to write  checks for each main part of the project

Here’s what I test:

- **Bit helpers**  
  Test turning hex strings into bits and back, pretty-printing bits,  
  and making sure two’s-complement encode/decode works for edge values

- **ALU and shifter**  
  Basic add and subtract cases.Check that the 
  zero flag is set when the result is zero. Shifter tests left,  
  logical right, and arithmetic right shifts.

- **Multiply/Divide unit**  
  A few special cases like `12345678 * -87654321`, divide by 0, and the special RISC-V  
  case `INT_MIN / -1`. Make sure the flags are correct.

- **Float32 helpers f32**  
  Unpack/pack a 32 bit float.Testvalues like `1.5`, `2.25`, `3.75`, and  
 multiply/add/sub cases. Test overflows.

- **CPU simulator**  
  Test loading mini programs into memory. Check reg values.

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
```

## AI Usage

I used ChatGPT to assist me with this project. It was my tutor and debugging agent 
through the project. I included a json file to read AI markers.  
Unfortunately, this json cannot count the AI support I received with tutoring, 
Q&A, quality check, and overall support with subject 
understanding. 

### What I used it for

- **Tutoring and questions**
  - Arithmetic: Helped refresh my 2's comp understanding, bit multiplication/division, and float. Tutored me through my struggles.
  - Logic Gates: I needed a lot of practice to fully understand the gates and not get them mixed up. ChatGPT answered my questions and provided 
  me with examples to help me practice.
  - Shifts and Overflow: Overflow was quite a confusing topic for me, so I needed some further explanation and walk-throughs to help me understand.
  - Data path and memory: This was also interesting and complicated at the same time. AI helped me with all my questions that popped up while 
  I was trying to better understand it in order to code.
  - All in all, it helped explain things I did not fully understood through lecture and
  reading.
  - It also helped me think about how C++ can represent a RISC-V CPU

- **Debugging**
  - AI helped me figure out failed tests and helped me come up with tests. This is my first time using google tests.
  - Fixing bugs in almost every file
  - It also provided me background information why certain coding is better practice. 
  - AI helped me learn more about C++ during this project.

- **Testing and skeleton**
  - Learned about GoogleTest cases for:
    - ALU
    - Bit helpers
    - MDU 
    - Float32
    - CPU
  - AI helped me make better naming conventions

- **Comments and README**
  - AI helped me articulate function descriptions simply



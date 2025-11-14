//
// Created by Allisa Warren on 11/13/25.
//
#include <gtest/gtest.h>
#include "core/rv32_cpu.hpp"

using namespace rv::cpu;

TEST(CpuBasic, AddiAndAdd) {
    CpuState s(1024);
    reset(s);

    // Program:
    //   addi x1, x0, 5   ; x1 = 5
    //   addi x2, x0, 7   ; x2 = 7
    //   add  x3, x1, x2  ; x3 = 12
    //
    // Encodings (RV32I):
    // addi x1,x0,5: imm=5, rs1=0, funct3=0x0, rd=1, opcode=0x13
    //   imm[11:0]=0x005 → 0x00500093
    // addi x2,x0,7: 0x00700113
    // add x3,x1,x2: funct7=0x00, rs2=2, rs1=1, funct3=0x0, rd=3, opcode=0x33
    //   → 0x002081B3

    std::vector<uint32_t> program = {
        0x00500093u,
        0x00700113u,
        0x002081B3u
    };

    load_program(s, program, 0);
    run(s, 3);

    EXPECT_EQ(s.regs[1], 5u);
    EXPECT_EQ(s.regs[2], 7u);
    EXPECT_EQ(s.regs[3], 12u);
    EXPECT_EQ(s.regs[0], 0u); // x0 remains zero
}


TEST(CpuBasic, LogicAndShift) {
    CpuState s(1024);
    reset(s);

    // Program:
    //   addi x1, x0, 1       ; x1 = 1
    //   slli x2, x1, 3       ; x2 = 8
    //   addi x3, x0, 0xFF    ; x3 = 255
    //   and  x4, x2, x3      ; x4 = 8 & 255 = 8
    //   srai x5, x4, 1       ; x5 = 4 (arithmetic right shift of positive is fine)
    //
    // Encodings:
    // addi x1,x0,1:   imm=1, rs1=0, funct3=0, rd=1, opcode=0x13 → 0x00100093
    // slli x2,x1,3:   shamt=3, rs1=1, funct3=1, rd=2, opcode=0x13, funct7=0 → 0x00309113
    // addi x3,x0,255: imm=255 (0x0FF), rs1=0, rd=3 → 0x0ff00193
    // and  x4,x2,x3:  funct7=0, rs2=3, rs1=2, funct3=7, rd=4, opcode=0x33 → 0x0031f233
    // srai x5,x4,1:   shamt=1, rs1=4, funct3=5, rd=5, opcode=0x13, funct7=0x20 → 0x40126293

    std::vector<uint32_t> program = {
        0x00100093u, // addi x1,x0,1
        0x00309113u, // slli x2,x1,3
        0x0ff00193u, // addi x3,x0,255
        0x00317233u, // and  x4,x2,x3
        0x40125293u  // srai x5,x4,1
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 1u);
    EXPECT_EQ(s.regs[2], 8u);
    EXPECT_EQ(s.regs[3], 255u);
    EXPECT_EQ(s.regs[4], 8u);
    EXPECT_EQ(s.regs[5], 4u);
    EXPECT_EQ(s.regs[0], 0u);
}

TEST(CpuMem, LwSwSimple) {
    CpuState s(1024);
    reset(s);

    // Program:
    //   addi x1, x0, 16      ; x1 = 16 (base address)
    //   addi x2, x0, 42      ; x2 = 42
    //   sw   x2, 0(x1)       ; mem[16] = 42
    //   lw   x3, 0(x1)       ; x3 = mem[16] = 42
    //
    // Encodings:
    // addi x1,x0,16:  imm=16 → 0x01000093
    // addi x2,x0,42:  imm=42 → 0x02a00113
    //
    // sw x2,0(x1): S-type, funct3=010, rs1=1, rs2=2, imm=0 → 0x0020a023
    // lw x3,0(x1): I-type, opcode=0x03, funct3=010, rs1=1, rd=3, imm=0 → 0x0000a183

    std::vector<uint32_t> program = {
        0x01000093u, // addi x1,x0,16
        0x02a00113u, // addi x2,x0,42
        0x0020a023u, // sw   x2,0(x1)
        0x0000a183u  // lw   x3,0(x1)
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 16u);
    EXPECT_EQ(s.regs[2], 42u);
    EXPECT_EQ(s.regs[3], 42u);
    EXPECT_EQ(s.regs[0], 0u);

    // Optional: check raw memory bytes (42 = 0x2A).
    EXPECT_EQ(s.mem[16], 0x2Au);
}

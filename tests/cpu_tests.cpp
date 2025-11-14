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

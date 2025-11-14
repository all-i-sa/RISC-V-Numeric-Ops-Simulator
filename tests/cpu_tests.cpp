//
// Created by Allisa Warren on 11/13/25.
//
#include <gtest/gtest.h>
#include "core/rv32_cpu.hpp"

#include <gtest/gtest.h>
#include "core/rv32_cpu.hpp"

using namespace rv::cpu;

namespace {

    uint32_t encode_jal(uint32_t rd, int32_t offset_bytes) {
        // offset in bytes, must be aligned by 2 (RISC-V uses bit 0 = 0)
        // We'll trust test inputs to be in range.
        uint32_t imm = static_cast<uint32_t>(offset_bytes);

        uint32_t imm_20    = (imm >> 20) & 0x1;
        uint32_t imm_10_1  = (imm >> 1)  & 0x3FF;
        uint32_t imm_11    = (imm >> 11) & 0x1;
        uint32_t imm_19_12 = (imm >> 12) & 0xFF;

        uint32_t inst = 0;
        inst |= (imm_20    << 31);
        inst |= (imm_19_12 << 12);
        inst |= (imm_11    << 20);
        inst |= (imm_10_1  << 21);
        inst |= (rd        << 7);
        inst |= 0x6F; // opcode for JAL

        return inst;
    }

    uint32_t encode_jalr(uint32_t rd, uint32_t rs1, int32_t imm) {
        // I-type: imm[11:0] | rs1 | funct3=0 | rd | opcode=0x67
        uint32_t imm_u = static_cast<uint32_t>(imm) & 0xFFF;

        uint32_t inst = 0;
        inst |= (imm_u << 20);
        inst |= (rs1   << 15);
        inst |= (0x0   << 12); // funct3 = 0 for JALR
        inst |= (rd    << 7);
        inst |= 0x67;          // opcode for JALR

        return inst;
    }

    uint32_t encode_lui(uint32_t rd, uint32_t imm20) {
        // imm20 is the upper 20 bits; result is imm20 << 12 | rd | opcode
        uint32_t inst = 0;
        inst |= (imm20 << 12);
        inst |= (rd << 7);
        inst |= 0x37; // opcode for LUI
        return inst;
    }

    uint32_t encode_auipc(uint32_t rd, uint32_t imm20) {
        uint32_t inst = 0;
        inst |= (imm20 << 12);
        inst |= (rd << 7);
        inst |= 0x17; // opcode for AUIPC
        return inst;
    }

} // namespace


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

TEST(CpuBranch, BeqBneBasic) {
    using namespace rv::cpu;

    CpuState s(1024);
    reset(s);

    // Program layout (addresses in bytes):
    // 0x00: addi x1, x0, 0      ; x1 = 0
    // 0x04: addi x2, x0, 1      ; x2 = 1
    // 0x08: beq  x1, x2, +8     ; NOT taken (0 != 1), go to 0x0C
    // 0x0C: addi x3, x0, 5      ; executed, x3 = 5
    // 0x10: bne  x1, x2, +8     ; TAKEN (0 != 1), jump to 0x18
    // 0x14: addi x4, x0, 9      ; skipped
    // 0x18: addi x5, x0, 7      ; executed, x5 = 7
    //
    // Encodings (RV32I):
    // addi x1,x0,0   @0x00 → 0x00000093
    // addi x2,x0,1   @0x04 → 0x00100113
    // beq  x1,x2,+8  @0x08 → 0x0208463
    // addi x3,x0,5   @0x0C → 0x00500193
    // bne  x1,x2,+8  @0x10 → 0x0209463
    // addi x4,x0,9   @0x14 → 0x00900213
    // addi x5,x0,7   @0x18 → 0x00700293

    std::vector<uint32_t> program = {
        0x00000093u, // addi x1,x0,0
        0x00100113u, // addi x2,x0,1
        0x00208463u, // beq  x1,x2,+8  (NOT taken, since 0 != 1)
        0x00500193u, // addi x3,x0,5   (should execute)
        0x00209463u, // bne  x1,x2,+8  (TAKEN, since 0 != 1)
        0x00900213u, // addi x4,x0,9   (should be skipped)
        0x00700293u  // addi x5,x0,7   (executed after branch)
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 0u);  // x1 = 0
    EXPECT_EQ(s.regs[2], 1u);  // x2 = 1
    EXPECT_EQ(s.regs[3], 5u);  // from addi at 0x0C
    EXPECT_EQ(s.regs[4], 0u);  // skipped (bne jumped over it)
    EXPECT_EQ(s.regs[5], 7u);  // from addi at 0x18
    EXPECT_EQ(s.regs[0], 0u);  // x0 hard-wired to 0
}

TEST(CpuJump, JalBasic) {
    CpuState s(1024);
    reset(s);

    // Layout (byte addresses):
    // 0x00: addi x1, x0, 1       ; x1 = 1
    // 0x04: jal  x2, +8          ; jump to 0x0C, x2 = 0x08
    // 0x08: addi x3, x0, 99      ; should be skipped
    // 0x0C: addi x4, x0, 5       ; executed

    std::vector<uint32_t> program = {
        0x00100093u,             // addi x1,x0,1
        encode_jal(2, 8),        // jal  x2, +8
        0x06300193u,             // addi x3,x0,99
        0x00500213u              // addi x4,x0,5
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 1u);      // x1 = 1
    EXPECT_EQ(s.regs[2], 8u);      // x2 = return address (0x04 + 4 = 0x08)
    EXPECT_EQ(s.regs[3], 0u);      // skipped
    EXPECT_EQ(s.regs[4], 5u);      // executed at jump target
    EXPECT_EQ(s.regs[0], 0u);      // x0 still 0
}

TEST(CpuJump, JalrBasic) {
    CpuState s(1024);
    reset(s);

    // Layout (byte addresses):
    // 0x00: addi x1, x0, 16      ; x1 = 16 (base)
    // 0x04: addi x2, x0, 1       ; x2 = 1 (just to use another register)
    // 0x08: jalr x3, 4(x1)       ; target = (16 + 4) & ~1 = 0x14 (20)
    // 0x0C: addi x4, x0, 99      ; should be skipped
    // 0x10: addi x0, x0, 0       ; nop
    // 0x14: addi x5, x0, 7       ; executed at jump target

    std::vector<uint32_t> program = {
        0x01000093u,                 // addi x1,x0,16
        0x00100113u,                 // addi x2,x0,1
        encode_jalr(3, 1, 4),        // jalr x3, 4(x1)
        0x06300213u,                 // addi x4,x0,99
        0x00000013u,                 // addi x0,x0,0 (nop)
        0x00700293u                  // addi x5,x0,7
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 16u);      // base
    EXPECT_EQ(s.regs[2], 1u);       // untouched by jump
    EXPECT_EQ(s.regs[3], 0x0Cu);    // return address (pc was 0x08 → pc+4 = 0x0C)
    EXPECT_EQ(s.regs[4], 0u);       // skipped
    EXPECT_EQ(s.regs[5], 7u);       // executed at jump target
    EXPECT_EQ(s.regs[0], 0u);       // x0 still 0
}

TEST(CpuUType, LuiBasic) {
    CpuState s(1024);
    reset(s);

    // LUI should load imm20 into the upper bits: rd = imm20 << 12
    // Choose imm20 = 0x000AB => rd = 0x000AB000
    std::vector<uint32_t> program = {
        encode_lui(1, 0x000AB) // lui x1, 0x000AB
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 0x000AB000u);
    EXPECT_EQ(s.regs[0], 0u);
}

TEST(CpuUType, AuipcBasic) {
    CpuState s(1024);
    reset(s);

    // Program:
    // 0x00: auipc x1, 0x00001  ; x1 = pc(0x00) + 0x00001000 = 0x00001000
    // 0x04: auipc x2, 0x00002  ; x2 = pc(0x04) + 0x00002000 = 0x00002004

    std::vector<uint32_t> program = {
        encode_auipc(1, 0x00001),
        encode_auipc(2, 0x00002)
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 0x00001000u); // 0x0000 + 0x00001000
    EXPECT_EQ(s.regs[2], 0x00002004u); // 0x0004 + 0x00002000
    EXPECT_EQ(s.regs[0], 0u);
}

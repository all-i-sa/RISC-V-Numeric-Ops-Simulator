#include <gtest/gtest.h>
#include "core/rv32_cpu.hpp"

using namespace rv::cpu;

namespace {

    uint32_t encode_jal(uint32_t rd, int32_t offset_bytes) {
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
        inst |= 0x6F;

        return inst;
    }

    uint32_t encode_jalr(uint32_t rd, uint32_t rs1, int32_t imm) {
        uint32_t imm_u = static_cast<uint32_t>(imm) & 0xFFF;

        uint32_t inst = 0;
        inst |= (imm_u << 20);
        inst |= (rs1   << 15);
        inst |= (0x0   << 12);
        inst |= (rd    << 7);
        inst |= 0x67;

        return inst;
    }


    uint32_t encode_lui(uint32_t rd, uint32_t imm20) {
        uint32_t inst = 0;
        inst |= (imm20 << 12);
        inst |= (rd << 7);
        inst |= 0x37;
        return inst;
    }

    uint32_t encode_auipc(uint32_t rd, uint32_t imm20) {
        uint32_t inst = 0;
        inst |= (imm20 << 12);
        inst |= (rd << 7);
        inst |= 0x17;
        return inst;
    }

} // namespace


/***** basic arithmetic test *****
 *********************************/
TEST(CpuBasic, AddiAndAdd) {
    CpuState s(1024);
    reset(s);

    std::vector<uint32_t> program = {
        0x00500093u, // addi x1,x0,5
        0x00700113u, // addi x2,x0,7
        0x002081B3u  // add  x3,x1,x2
    };

    load_program(s, program, 0);
    run(s, 3);

    EXPECT_EQ(s.regs[1], 5u);
    EXPECT_EQ(s.regs[2], 7u);
    EXPECT_EQ(s.regs[3], 12u);
    EXPECT_EQ(s.regs[0], 0u); // x0 remains zero
}

/***** logic and shifts test *****
 *********************************/
TEST(CpuBasic, LogicAndShift) {
    CpuState s(1024);
    reset(s);

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

/***** oads and stores tests *****
 *********************************/
TEST(CpuMem, LwSwSimple) {
    CpuState s(1024);
    reset(s);

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

    EXPECT_EQ(s.mem[16], 0x2Au);
}

/***** branches test *****
 *************************/
// AI-BEGIN: Tutor/teach test case
TEST(CpuBranch, BeqBneBasic) {
    CpuState s(1024);
    reset(s);

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
    EXPECT_EQ(s.regs[4], 0u);  // skipped by branch
    EXPECT_EQ(s.regs[5], 7u);  // from addi at 0x18
    EXPECT_EQ(s.regs[0], 0u);  // x0 hard-wired to 0
}
// AI-END

/***** JAL test *****
 ********************/
TEST(CpuJump, JalBasic) {
    CpuState s(1024);
    reset(s);

    std::vector<uint32_t> program = {
        0x00100093u,             // addi x1,x0,1
        encode_jal(2, 8),        // jal  x2, +8
        0x06300193u,             // addi x3,x0,99 (skipped)
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

/***** JALR test *****
 *********************/
TEST(CpuJump, JalrBasic) {
    CpuState s(1024);
    reset(s);

    std::vector<uint32_t> program = {
        0x01000093u,                 // addi x1,x0,16
        0x00100113u,                 // addi x2,x0,1
        encode_jalr(3, 1, 4),        // jalr x3, 4(x1)
        0x06300213u,                 // addi x4,x0,99 (skipped)
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

/***** LUI *****
 ***************/
// AI-BEGIN: TEACH & QA TEST CASE
TEST(CpuUType, LuiBasic) {
    CpuState s(1024);
    reset(s);

    std::vector<uint32_t> program = {
        encode_lui(1, 0x000AB) // lui x1, 0x000AB
    };

    load_program(s, program, 0);
    run(s, program.size());

    EXPECT_EQ(s.regs[1], 0x000AB000u);
    EXPECT_EQ(s.regs[0], 0u);
}
// AI-END

/***** AUIPC *****
 *****************/
TEST(CpuUType, AuipcBasic) {
    CpuState s(1024);
    reset(s);

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

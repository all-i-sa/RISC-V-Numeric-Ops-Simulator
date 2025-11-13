//
// Created by Allisa Warren on 11/12/25.
//
#include <gtest/gtest.h>
#include "core/alu.hpp"
#include "core/bitvec.hpp"

using namespace rv::core;

TEST(AluSmoke, ZeroAddReturnsZero) {
    // 32-bit zero operands
    Bits a(32, 0);
    Bits b(32, 0);

    auto res = alu_execute(a, b, AluOp::Add);

    // We only assert very basic things for now
    EXPECT_EQ(res.result.size(), 32u);
    // With our stub, result should be all zeros and Z flag set.
    for (Bit bit : res.result) {
        EXPECT_EQ(bit, 0);
    }
    EXPECT_EQ(res.flags.Z, 1);
}

TEST(AluAddSub, AddPosOverflow) {
    // 0x7FFFFFFF + 0x00000001 → 0x80000000; V=1; C=0; N=1; Z=0.
    Bits a = bv_from_hex_string("0x7fffffff");
    Bits b = bv_from_hex_string("0x1");

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x80000000");
    EXPECT_EQ(res.flags.V, 1);
    EXPECT_EQ(res.flags.C, 0);
    EXPECT_EQ(res.flags.N, 1);
    EXPECT_EQ(res.flags.Z, 0);
}

TEST(AluAddSub, SubNegOverflow) {
    // 0x80000000 - 0x00000001 → 0x7FFFFFFF; V=1; C=1; N=0; Z=0.
    Bits a = bv_from_hex_string("0x80000000");
    Bits b = bv_from_hex_string("0x1");

    auto res = alu_execute(a, b, AluOp::Sub);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x7fffffff");
    EXPECT_EQ(res.flags.V, 1);
    EXPECT_EQ(res.flags.C, 1); // no borrow
    EXPECT_EQ(res.flags.N, 0);
    EXPECT_EQ(res.flags.Z, 0);
}

TEST(AluAddSub, AddMinusOnePlusMinusOne) {
    // -1 + -1 → -2; V=0; C=1; N=1; Z=0.
    Bits a = bv_from_hex_string("0xffffffff"); // -1
    Bits b = bv_from_hex_string("0xffffffff"); // -1

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0xfffffffe"); // -2
    EXPECT_EQ(res.flags.V, 0);
    EXPECT_EQ(res.flags.C, 1);
    EXPECT_EQ(res.flags.N, 1);
    EXPECT_EQ(res.flags.Z, 0);
}

TEST(AluAddSub, AddThirteenAndMinusThirteen) {
    // ADD 13 + -13 → 0; V=0; C=1; N=0; Z=1.
    Bits a = bv_from_hex_string("0xd");        // +13
    Bits b = bv_from_hex_string("0xfffffff3"); // -13

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x0");
    EXPECT_EQ(res.flags.V, 0);
    EXPECT_EQ(res.flags.C, 1);
    EXPECT_EQ(res.flags.N, 0);
    EXPECT_EQ(res.flags.Z, 1);
}

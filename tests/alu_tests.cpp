#include <gtest/gtest.h>
#include "core/alu.hpp"
#include "core/bitvec.hpp"

using namespace rv::core;

/***** Test: AddPosOverflow *****
 * Case:
 *   0x7FFFFFFF + 0x00000001
 * Expect:
 *   result = 0x80000000
 *   V = 1
 *   C = 0
 *   N = 1
 *   Z = 0
 ******************************/
TEST(AluAddSub, AddPosOverflow) {
    Bits a = bv_from_hex_string("0x7fffffff");
    Bits b = bv_from_hex_string("0x1");

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x80000000");
    EXPECT_EQ(res.flags.V, 1);
    EXPECT_EQ(res.flags.C, 0);
    EXPECT_EQ(res.flags.N, 1);
    EXPECT_EQ(res.flags.Z, 0);
}

/***** Test: SubNegOverflow *****
 * Case:
 *   0x80000000 - 0x00000001
 * Expect:
 *   result = 0x7FFFFFFF
 *   V = 1
 *   C = 1
 *   N = 0
 *   Z = 0
 ******************************/
TEST(AluAddSub, SubNegOverflow) {
    Bits a = bv_from_hex_string("0x80000000");
    Bits b = bv_from_hex_string("0x1");

    auto res = alu_execute(a, b, AluOp::Sub);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x7fffffff");
    EXPECT_EQ(res.flags.V, 1);
    EXPECT_EQ(res.flags.C, 1); // no borrow
    EXPECT_EQ(res.flags.N, 0);
    EXPECT_EQ(res.flags.Z, 0);
}

/***** Test: AddMinusOnePlusMinusOne *****
 * Case:
 *   -1 + -1
 * Expect:
 *   result = -2
 *   V = 0
 *   C = 1
 *   N = 1
 *   Z = 0
 ******************************/
TEST(AluAddSub, AddMinusOnePlusMinusOne) {
    Bits a = bv_from_hex_string("0xffffffff"); // -1
    Bits b = bv_from_hex_string("0xffffffff"); // -1

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0xfffffffe"); // -2
    EXPECT_EQ(res.flags.V, 0);
    EXPECT_EQ(res.flags.C, 1);
    EXPECT_EQ(res.flags.N, 1);
    EXPECT_EQ(res.flags.Z, 0);
}

/***** Test: AddThirteenAndMinusThirteen *****
 * Case:
 *   13 + (-13)
 * Expect:
 *   result = 0x00000000
 *   V = 0
 *   C = 1
 *   N = 0
 *   Z = 1
 ******************************/
TEST(AluAddSub, AddThirteenAndMinusThirteen) {
    Bits a = bv_from_hex_string("0xd");        // +13
    Bits b = bv_from_hex_string("0xfffffff3"); // -13

    auto res = alu_execute(a, b, AluOp::Add);

    EXPECT_EQ(bv_to_hex_string(res.result), "0x0");
    EXPECT_EQ(res.flags.V, 0);
    EXPECT_EQ(res.flags.C, 1);
    EXPECT_EQ(res.flags.N, 0);
    EXPECT_EQ(res.flags.Z, 1);
}

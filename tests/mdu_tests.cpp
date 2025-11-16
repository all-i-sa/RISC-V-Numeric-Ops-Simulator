#include <gtest/gtest.h>
#include "core/mdu.hpp"
#include "core/bitvec.hpp"
#include "core/twos.hpp"

using namespace rv::core;

/***** Test: 0 * 0 *****
 ***********************/
TEST(MduSmoke, MulZeroOperands) {
    Bits a(32, 0); // 0
    Bits b(32, 0); // 0

    auto res = mdu_mul(MulOp::Mul, a, b);

    EXPECT_EQ(res.lo.size(), 32u);
    EXPECT_EQ(res.hi.size(), 32u);
    EXPECT_FALSE(res.overflow);

    EXPECT_EQ(bv_to_hex_string(res.lo), "0x0");
    EXPECT_EQ(bv_to_hex_string(res.hi), "0x0");
}

/***** Test: 4 / 2 *****
 ***********************/
TEST(MduSmoke, DivSimpleCase) {
    Bits a = bv_from_hex_string("0x4"); // 4
    Bits b = bv_from_hex_string("0x2"); // 2

    auto res = mdu_div(DivOp::Div, a, b);

    EXPECT_EQ(res.q.size(), 32u);
    EXPECT_EQ(res.r.size(), 32u);
    EXPECT_FALSE(res.overflow);

    EXPECT_EQ(bv_to_hex_string(res.q), "0x2");
    EXPECT_EQ(bv_to_hex_string(res.r), "0x0");
}

/***** Test: signed multiply  *****
 **********************************/
TEST(MduMul, ExampleFromSpec) {
    auto enc_a = encode_twos_i32(12345678);    // +12,345,678
    auto enc_b = encode_twos_i32(-87654321);   // -87,654,321

    Bits a = enc_a.bits;
    Bits b = enc_b.bits;

    auto res = mdu_mul(MulOp::Mul, a, b);

    EXPECT_EQ(bv_to_hex_string(res.lo), "0xd91d0712");
    EXPECT_TRUE(res.overflow);

    EXPECT_EQ(res.trace.size(), 33u);
}

/***** Test: signed divide *****
 *   -7 / 3
 *******************************/
TEST(MduDiv, SignedExampleFromSpec) {
    auto enc_a = encode_twos_i32(-7);
    auto enc_b = encode_twos_i32(3);

    auto res = mdu_div(DivOp::Div, enc_a.bits, enc_b.bits);

    EXPECT_EQ(bv_to_hex_string(res.q), "0xfffffffe");
    EXPECT_EQ(bv_to_hex_string(res.r), "0xffffffff");
    EXPECT_FALSE(res.overflow);
}

/***** Test: divide by zero  *****
 *********************************/
TEST(MduDiv, DivideByZeroRule) {
    auto enc_dividend = encode_twos_i32(42);   // any nonzero
    auto enc_divisor  = encode_twos_i32(0);    // 0

    auto res = mdu_div(DivOp::Div, enc_dividend.bits, enc_divisor.bits);

    EXPECT_EQ(bv_to_hex_string(res.q), "0xffffffff");
    EXPECT_EQ(bv_to_hex_string(res.r), enc_dividend.hex); // same bits as dividend
    EXPECT_FALSE(res.overflow);

    ASSERT_FALSE(res.trace.empty());
    EXPECT_NE(res.trace[0].find("divide-by-zero"), std::string::npos);
}

/***** Test: INT_MIN / -1 special case *****
 *******************************************/
TEST(MduDiv, IntMinDivMinusOneSpecialCase) {
    auto enc_min  = encode_twos_i32(-2147483648LL); // INT_MIN
    auto enc_neg1 = encode_twos_i32(-1);

    auto res = mdu_div(DivOp::Div, enc_min.bits, enc_neg1.bits);

    EXPECT_EQ(bv_to_hex_string(res.q), "0x80000000");
    EXPECT_EQ(bv_to_hex_string(res.r), "0x0");
    EXPECT_TRUE(res.overflow);

    ASSERT_FALSE(res.trace.empty());
    EXPECT_NE(res.trace[0].find("INT_MIN / -1 special case"), std::string::npos);
}

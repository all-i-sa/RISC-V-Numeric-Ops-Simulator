#include <gtest/gtest.h>
#include "core/f32.hpp"
#include "core/bitvec.hpp"

using namespace rv::core;

/***** unpack/pack ************************
 * The bit pattern should come back exactly
 * the same.
 ******************************************/
TEST(FloatF32, UnpackPackRoundTripSimple) {
    Bits pattern = bv_from_hex_string("0x40700000");

    F32Fields f = unpack_f32(pattern);
    Bits rebuilt = pack_f32(f);

    EXPECT_EQ(rebuilt.size(), 32u);
    EXPECT_EQ(bv_to_hex_string(rebuilt), bv_to_hex_string(pattern));
}

/***** basic shape of add/sub/mul results *****
 ******************************************/
TEST(FloatF32, ArithmeticStubsProduce32Bits) {
    Bits a(32, 0);
    Bits b(32, 0);

    auto add_res = fadd_f32(a, b);
    auto sub_res = fsub_f32(a, b);
    auto mul_res = fmul_f32(a, b);

    EXPECT_EQ(add_res.bits.size(), 32u);
    EXPECT_EQ(sub_res.bits.size(), 32u);
    EXPECT_EQ(mul_res.bits.size(), 32u);

    EXPECT_FALSE(add_res.flags.overflow);
    EXPECT_FALSE(add_res.flags.underflow);
    EXPECT_FALSE(add_res.flags.invalid);
}

/***** Test 1.5 + 2.25 = 3.75 *****
 ******************************************/
TEST(FloatF32, Add_1p5_Plus_2p25_Equals_3p75) {
    // 1.5 and 2.25 in hex form
    Bits a = bv_from_hex_string("0x3fc00000");
    Bits b = bv_from_hex_string("0x40100000");

    auto res = fadd_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x40700000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);

    ASSERT_FALSE(res.trace.empty());
    EXPECT_EQ(res.trace.back(), "fadd_f32 normal same-sign add");
}

/***** Test: 2.25 - 1.5 = 0.75 *****
 ***********************************/
TEST(FloatF32, Sub_2p25_Minus_1p5_Equals_0p75) {
    // 2.25 - 1.5 = 0.75
    Bits a = bv_from_hex_string("0x40100000"); // 2.25
    Bits b = bv_from_hex_string("0x3fc00000"); // 1.5

    auto res = fsub_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x3f400000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);

    ASSERT_FALSE(res.trace.empty());
    EXPECT_EQ(res.trace.back(), "fadd_f32 different-sign subtract");
}

/***** Test: 1.5 * 2.0 = 3.0 *****
 *********************************/
// AI-BEGIN: design test case
TEST(FloatF32, Mul_1p5_Times_2_Equals_3) {
    // 1.5 * 2.0 = 3.0
    Bits a = bv_from_hex_string("0x3fc00000");
    Bits b = bv_from_hex_string("0x40000000");

    auto res = fmul_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x40400000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);
}
// AI-END

/***** Test: overflow *****
 **************************/
TEST(FloatF32, Mul_1e38_Times_10_Overflow) {
    Bits a = bv_from_hex_string("0x7e967699"); // ~1e38
    Bits b = bv_from_hex_string("0x41200000"); // 10.0

    auto res = fmul_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x7f800000");
    EXPECT_TRUE(res.flags.overflow);
    EXPECT_FALSE(res.flags.invalid);
}

/***** Test: underflow *****
 ***************************/
// AI-BEGIN: Verify test case
TEST(FloatF32, Mul_1eMinus38_Times_1eMinus2_Underflow) {
    Bits a = bv_from_hex_string("0x006ce3ee"); // ~1e-38
    Bits b = bv_from_hex_string("0x3c23d70a"); // 1e-2

    auto res = fmul_f32(a, b);

    EXPECT_TRUE(res.flags.underflow);
    EXPECT_FALSE(res.flags.overflow);
}
// AI-END
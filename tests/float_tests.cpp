//
// Created by Allisa Warren on 11/12/25.
//
#include <gtest/gtest.h>
#include "core/f32.hpp"
#include "core/bitvec.hpp"

using namespace rv::core;

TEST(FloatF32, UnpackPackRoundTripSimple) {
    // 3.75 has known pattern 0x40700000, but we only check identity through pack/unpack.
    Bits pattern = bv_from_hex_string("0x40700000");

    F32Fields f = unpack_f32(pattern);
    Bits rebuilt = pack_f32(f);

    EXPECT_EQ(rebuilt.size(), 32u);
    EXPECT_EQ(bv_to_hex_string(rebuilt), bv_to_hex_string(pattern));
}

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

TEST(FloatF32, Add_1p5_Plus_2p25_Equals_3p75) {
    using namespace rv::core;

    // Known IEEE-754 single-precision patterns:
    // 1.5   → 0x3FC00000
    // 2.25  → 0x40100000
    // 3.75  → 0x40700000
    Bits a = bv_from_hex_string("0x3fc00000");
    Bits b = bv_from_hex_string("0x40100000");

    auto res = fadd_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x40700000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);

    ASSERT_FALSE(res.trace.empty());
    // Just sanity-check that we didn't go through the "different signs" unimplemented path.
    EXPECT_EQ(res.trace.back(), "fadd_f32 normal same-sign add");
}

TEST(FloatF32, Sub_2p25_Minus_1p5_Equals_0p75) {
    using namespace rv::core;

    // Known patterns:
    // 1.5   → 0x3FC00000
    // 2.25  → 0x40100000
    // 0.75  → 0x3F400000
    Bits a = bv_from_hex_string("0x40100000"); // 2.25
    Bits b = bv_from_hex_string("0x3fc00000"); // 1.5

    auto res = fsub_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x3f400000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);

    ASSERT_FALSE(res.trace.empty());
    // Should go through the different-sign subtract path.
    EXPECT_EQ(res.trace.back(), "fadd_f32 different-sign subtract");
}


TEST(FloatF32, Mul_1p5_Times_2_Equals_3) {
    using namespace rv::core;

    // 1.5 → 0x3FC00000
    // 2.0 → 0x40000000
    // 3.0 → 0x40400000
    Bits a = bv_from_hex_string("0x3fc00000");
    Bits b = bv_from_hex_string("0x40000000");

    auto res = fmul_f32(a, b);

    EXPECT_EQ(bv_to_hex_string(res.bits), "0x40400000");
    EXPECT_FALSE(res.flags.overflow);
    EXPECT_FALSE(res.flags.underflow);
    EXPECT_FALSE(res.flags.invalid);
}

TEST(FloatF32, Mul_1e38_Times_10_Overflow) {
    using namespace rv::core;

    // Hex patterns for float32:
    // 1e38  ≈ 0x7E967699
    // 10.0  → 0x41200000
    Bits a = bv_from_hex_string("0x7e967699");
    Bits b = bv_from_hex_string("0x41200000");

    auto res = fmul_f32(a, b);

    // Expect +∞ and overflow flag.
    EXPECT_EQ(bv_to_hex_string(res.bits), "0x7f800000");
    EXPECT_TRUE(res.flags.overflow);
    EXPECT_FALSE(res.flags.invalid);
}

TEST(FloatF32, Mul_1eMinus38_Times_1eMinus2_Underflow) {
    using namespace rv::core;

    // Hex patterns:
    // 1e-38 → 0x006CE3EE
    // 1e-2  → 0x3C23D70A
    Bits a = bv_from_hex_string("0x006ce3ee");
    Bits b = bv_from_hex_string("0x3c23d70a");

    auto res = fmul_f32(a, b);

    EXPECT_TRUE(res.flags.underflow);
    EXPECT_FALSE(res.flags.overflow);
}

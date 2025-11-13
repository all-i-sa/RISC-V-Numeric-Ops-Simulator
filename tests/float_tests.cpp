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

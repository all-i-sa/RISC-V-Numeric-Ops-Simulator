//
// Created by Allisa Warren on 11/12/25.
//
#include <gtest/gtest.h>
#include "core/mdu.hpp"
#include "core/bitvec.hpp"
#include "core/twos.hpp"

using namespace rv::core;

TEST(MduSmoke, MulZeroOperands) {
    Bits a(32, 0); // 0
    Bits b(32, 0); // 0

    auto res = mdu_mul(MulOp::Mul, a, b);

    EXPECT_EQ(res.lo.size(), 32u);
    EXPECT_EQ(res.hi.size(), 32u);
    EXPECT_FALSE(res.overflow);

    // Stub currently returns all zeros; this will still be true after real impl for 0*0.
    EXPECT_EQ(bv_to_hex_string(res.lo), "0x0");
    EXPECT_EQ(bv_to_hex_string(res.hi), "0x0");
}

TEST(MduSmoke, DivSimpleCase) {
    Bits a = bv_from_hex_string("0x4"); // 4
    Bits b = bv_from_hex_string("0x2"); // 2

    auto res = mdu_div(DivOp::Div, a, b);

    EXPECT_EQ(res.q.size(), 32u);
    EXPECT_EQ(res.r.size(), 32u);
    EXPECT_FALSE(res.overflow);

    // For now this will *not* be correct, but that's fine; we only assert the sizes.
    // We'll add real expectation tests once we implement the algorithms.
}

TEST(MduMul, ExampleFromSpec) {
    using namespace rv::core;

    // Use our own encode helper so we don't mess up the hex.
    auto enc_a = encode_twos_i32(12345678);    // +12,345,678
    auto enc_b = encode_twos_i32(-87654321);   // -87,654,321

    Bits a = enc_a.bits;
    Bits b = enc_b.bits;

    auto res = mdu_mul(MulOp::Mul, a, b);

    // From the project spec:
    // MUL 12345678 * -87654321 → rd = 0xD91D0712 (low 32), overflow=1.
    EXPECT_EQ(bv_to_hex_string(res.lo), "0xd91d0712");
    EXPECT_TRUE(res.overflow);

    // Optional sanity check: we should have 33 snapshots (step 0..32)
    EXPECT_EQ(res.trace.size(), 33u);
}


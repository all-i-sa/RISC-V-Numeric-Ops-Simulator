//
// Created by Allisa Warren on 11/12/25.
//
#include <gtest/gtest.h>
#include "core/mdu.hpp"
#include "core/bitvec.hpp"

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

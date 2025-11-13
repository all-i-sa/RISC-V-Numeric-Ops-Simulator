#include "gtest/gtest.h"
#include "core/bitvec.hpp"

using namespace rv::core;

TEST(BitVec, HexRoundTrip) {
    auto b = bv_from_hex_string("0x7f_ff_ff_ff");
    EXPECT_EQ(bv_to_hex_string(b), "0x7fffffff");
}

TEST(BitVec, PrettyBin) {
    Bits b = bv_from_hex_string("0x00af");
    auto b16 = zero_extend(b, 16);                      // ensure 16-bit width for this snapshot
    EXPECT_EQ(bv_to_pretty_bin(b16, 4, '_'), "0000_0000_1010_1111");
}

TEST(BitVec, ExtendAndSlice) {
    Bits b = bv_from_hex_string("0xa"); // 1010
    auto z = zero_extend(b, 8);
    EXPECT_EQ(z.size(), 8u);
    EXPECT_EQ(bv_to_pretty_bin(z), "00001010");   // no underscores by default

    auto s = sign_extend(b, 8); // sign=MSB of b (=1)
    EXPECT_EQ(bv_to_pretty_bin(s), "11111010");   // no underscores by default

    auto sl = bv_slice(z, 3, 0); // [3:0] low nibble of 00001010
    EXPECT_EQ(bv_to_pretty_bin(sl), "1010");  // <-- was "0101"
}


TEST(BitVec, TwosNegate) {
    Bits b = bv_from_hex_string("0x05"); // 0000 0101
    b = bv_pad_left(b, 8, 0);
    auto n = twos_negate(b);             // should be 0xFB (1111 1011)
    EXPECT_EQ(bv_to_hex_string(n), "0xfb");
}

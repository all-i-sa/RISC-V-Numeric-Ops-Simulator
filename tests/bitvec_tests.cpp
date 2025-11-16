#include "gtest/gtest.h"
#include "core/bitvec.hpp"

using namespace rv::core;

 /***** Test: HexRoundTrip *****
  *   If bv_from_hex_string and bv_to_hex_string agree we should
  *   get a clean “round trip” with matching hex
  ******************************/
TEST(BitVec, HexRoundTrip) {
    auto b = bv_from_hex_string("0x7f_ff_ff_ff");
    EXPECT_EQ(bv_to_hex_string(b), "0x7fffffff");
}

/***** Test: PrettyBin *****
 * Case:
 *   0x00af → 0000_0000_1010_1111
 ******************************/
TEST(BitVec, PrettyBin) {
    Bits b = bv_from_hex_string("0x00af");
    auto b16 = zero_extend(b, 16);
    EXPECT_EQ(bv_to_pretty_bin(b16, 4, '_'), "0000_0000_1010_1111");
}

/***** Test: ExtendAndSlice *****
 * Cases:
 *   - zero_extend(0xA, 8)  → "00001010"
 *   - sign_extend(0xA, 8)  → "11111010"
 *   - slice the low nibble [3:0] → "1010"
 ******************************/
TEST(BitVec, ExtendAndSlice) {
    Bits b = bv_from_hex_string("0xa");
    auto z = zero_extend(b, 8);
    EXPECT_EQ(z.size(), 8u);
    EXPECT_EQ(bv_to_pretty_bin(z), "00001010");

    auto s = sign_extend(b, 8);
    EXPECT_EQ(bv_to_pretty_bin(s), "11111010");

    auto sl = bv_slice(z, 3, 0);
    EXPECT_EQ(bv_to_pretty_bin(sl), "1010");
}

/***** Test: TwosNegate *****
 * Case:
 *   Start with 0x05 (0000 0101), widen to 8 bits then negate
 *   The result should be 0xFB
 ******************************/
TEST(BitVec, TwosNegate) {
    Bits b = bv_from_hex_string("0x05");
    b = bv_pad_left(b, 8, 0);
    auto n = twos_negate(b);
    EXPECT_EQ(bv_to_hex_string(n), "0xfb");
}

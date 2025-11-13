#include "gtest/gtest.h"
#include "core/twos.hpp"
#include "core/bitvec.hpp"

using namespace rv::core;

static void check_i32(int32_t v, const char* expect_hex) {
    auto b = encode_i32_TEMP_host(v);
    EXPECT_EQ(b.size(), 32u);
    EXPECT_EQ(bv_to_hex_string(b), std::string(expect_hex));
    EXPECT_EQ(decode_i32_to_host(b), int64_t(v));
}

TEST(TwosI32, BoundaryCases) {
    check_i32(0, "0x0");
    check_i32(13, "0xd");
    check_i32(-13, "0xfffffff3");
    check_i32(-7, "0xfffffff9");
    check_i32(0x7fffffff, "0x7fffffff");
    check_i32(int32_t(0x80000000u), "0x80000000"); // -2^31
    check_i32(-1, "0xffffffff");
}

TEST(TwosI32, PrettySnapshot) {
    auto b = encode_i32_TEMP_host(0x1234abcd);
    EXPECT_EQ(bv_to_pretty_bin(b, 4, '_'),
              "0001_0010_0011_0100_1010_1011_1100_1101");
}

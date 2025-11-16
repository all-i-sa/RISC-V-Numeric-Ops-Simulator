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

/***** boundary checks *****/
TEST(TwosI32, BoundaryCases) {
    check_i32(0,              "0x0");
    check_i32(13,             "0xd");
    check_i32(-13,            "0xfffffff3");
    check_i32(-7,             "0xfffffff9");
    check_i32(0x7fffffff,     "0x7fffffff");
    check_i32(int32_t(0x80000000u), "0x80000000");
    check_i32(-1,             "0xffffffff");
}

/***** pretty binary snapshot *****
 **********************************/
TEST(TwosI32, PrettySnapshot) {
    auto b = encode_i32_TEMP_host(0x1234abcd);
    EXPECT_EQ(
        bv_to_pretty_bin(b, 4, '_'),
        "0001_0010_0011_0100_1010_1011_1100_1101"
    );
}

/***** ncode/decode with overflow flag *****
 *******************************************/
TEST(TwosEncodeDecode, BoundaryValues) {
    struct Case {
        int64_t     value;
        std::string expected_hex;
        bool        expect_overflow;
    };

    std::vector<Case> cases = {
        // In-range values (no overflow)
        { -2147483648LL, "0x80000000", false },
        { -1,            "0xffffffff", false },
        { -13,           "0xfffffff3", false },
        { -7,            "0xfffffff9", false },
        { 0,             "0x0",        false },
        { 13,            "0xd",        false },
        { 2147483647LL,  "0x7fffffff", false },

        // Out-of-range values
        {  2147483648LL, "0x80000000", true },
        { -2147483649LL, "0x7fffffff", true }
    };

    for (const auto& c : cases) {
        auto enc = encode_twos_i32(c.value);

        EXPECT_EQ(enc.overflow, c.expect_overflow) << "value=" << c.value;
        EXPECT_EQ(enc.hex,      c.expected_hex)    << "value=" << c.value;

        if (!c.expect_overflow) {
            int64_t decoded = decode_twos_i32(enc.bits);
            EXPECT_EQ(decoded, c.value);
        }
    }
}

#pragma once

#include "core/bitvec.hpp"
#include <vector>
#include <string>

namespace rv::core {

    struct F32Fields {
        Bit  sign;      // 0 = positive, 1 = negative
        Bits exponent;  // 8 bits, unbiased
        Bits fraction;  // 23 bits
    };

    struct FpuFlags {
        bool overflow;
        bool underflow;
        bool invalid;
        bool inexact;
    };

    struct FpuResult {
        Bits                   bits;   // 32-bit IEEE-754 pattern, LSB-first
        FpuFlags               flags;
        std::vector<std::string> trace;
    };

    // Bit-level unpack: 32-bit pattern -> fields (sign, exponent, fraction).
    F32Fields unpack_f32(const Bits& bits);

    // Bit-level pack: fields -> 32-bit pattern.
    Bits pack_f32(const F32Fields& f);

    // Arithmetic stubs (we'll fill in later).
    FpuResult fadd_f32(const Bits& a, const Bits& b);
    FpuResult fsub_f32(const Bits& a, const Bits& b);
    FpuResult fmul_f32(const Bits& a, const Bits& b);

} // namespace rv::core

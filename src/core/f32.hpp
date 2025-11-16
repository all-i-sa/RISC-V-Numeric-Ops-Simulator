#pragma once

#include "core/bitvec.hpp"
#include <vector>
#include <string>

namespace rv::core {

    /***** F32Fields *****
     *   A float32 value split into parts
     *****************************
     *   sign
     *   exponent - 8 bits for the exponent
     *   fraction - 23 bits for the fraction aka mantissa
     ******************************/
    struct F32Fields {
        Bit  sign;
        Bits exponent;
        Bits fraction;
    };

    /***** FpuFlags *****
     *   Flags that describe cases/issues that can happen with floats
     *
     *   overflow  - true if the result was too large and went to +Inf
     *   underflow - true if the result was very small and went towards zero
     *   invalid   - invalid operations
     *   inexact   - true if the result HAD to be rounded
     ******************************/
    struct FpuFlags {
        bool overflow;
        bool underflow;
        bool invalid;
        bool inexact;
    };

    /***** FpuResult *****
     *   The result of a float32 operation
     *
     *   bits  - 32-bit float pattern IEEE-754
     *   flags - info/alerts
     *   trace - shows what the operation did internally.
     ******************************/
    struct FpuResult {
        Bits                     bits;
        FpuFlags                 flags;
        std::vector<std::string> trace;
    };

    /***** unpack_f32 *****
     *   Takes a 32-bit float bit  and splits it into sign, exponent,
     *   and fraction
     ******************************
     * Inputs:
     *   bits - 32 bits float32 value
     * Returns:
     *   F32Fields - sign, exponent bits, and fraction bits
     ******************************/
    F32Fields unpack_f32(const Bits& bits);

    /***** pack_f32 *****
     *   Takes sign, exponent, and fraction fields and combines them into
     *   a 32-bit float bit
     *   - Does the opposite of unpack_f32
     ******************************
     * Inputs:
     *   f - sign, exponent, and fraction
     * Returns:
     *   Bits - 32 bits float32 value
     ******************************/
    Bits pack_f32(const F32Fields& f);

    /***** fadd_f32 *****
     *   Adds two float32 values
     ******************************
     * Inputs:
     *   a - first float32 value as bits.
     *   b - second float32 value as bits.
     * Returns:
     *   FpuResult - result bits, flags, and trace.
     ******************************/
    FpuResult fadd_f32(const Bits& a, const Bits& b);

    /***** fsub_f32 *****
     *   Subtracts two float32 values
     ******************************
     * Inputs:
     *   a - first float32 value
     *   b - second float32 value
     * Returns:
     *   FpuResult - result bits, flags, and trace.
     ******************************/
    FpuResult fsub_f32(const Bits& a, const Bits& b);

    /***** fmul_f32 *****
     *   Multiplies two float32 values
     ******************************
     * Inputs:
     *   a - first float32 value
     *   b - second float32 value
     * Returns:
     *   FpuResult - result bits, flags, and trace.
     ******************************/
    FpuResult fmul_f32(const Bits& a, const Bits& b);

} // namespace rv::core

//
// Created by Allisa Warren on 11/12/25.
//
#include "core/f32.hpp"
#include "core/bitvec.hpp"
#include <cassert>

namespace rv::core {

    F32Fields unpack_f32(const Bits& bits) {
        // Ensure exactly 32 bits
        Bits b32 = zero_extend(bits, 32);

        F32Fields f{};
        f.sign = b32[31];

        // Fraction: bits 0..22 (LSB-first)
        f.fraction = Bits(23, 0);
        for (std::size_t i = 0; i < 23; ++i) {
            f.fraction[i] = b32[i];
        }

        // Exponent: bits 23..30 (8 bits)
        f.exponent = Bits(8, 0);
        for (std::size_t i = 0; i < 8; ++i) {
            f.exponent[i] = b32[23 + i];
        }

        return f;
    }

    Bits pack_f32(const F32Fields& f) {
        Bits b32(32, 0);

        // Fraction bits 0..22
        for (std::size_t i = 0; i < 23 && i < f.fraction.size(); ++i) {
            b32[i] = f.fraction[i];
        }

        // Exponent bits 23..30
        for (std::size_t i = 0; i < 8 && i < f.exponent.size(); ++i) {
            b32[23 + i] = f.exponent[i];
        }

        // Sign bit 31
        b32[31] = f.sign;

        return b32;
    }

    FpuResult make_zero_fpu_result() {
        FpuResult r;
        r.bits = Bits(32, 0);
        r.flags = FpuFlags{false, false, false, false};
        r.trace.clear();
        return r;
    }

    FpuResult fadd_f32(const Bits& a, const Bits& b) {
        (void)a;
        (void)b;
        // Stub: we'll implement full IEEE-754 later.
        FpuResult r = make_zero_fpu_result();
        r.trace.push_back("fadd_f32 stub");
        return r;
    }

    FpuResult fsub_f32(const Bits& a, const Bits& b) {
        (void)a;
        (void)b;
        FpuResult r = make_zero_fpu_result();
        r.trace.push_back("fsub_f32 stub");
        return r;
    }

    FpuResult fmul_f32(const Bits& a, const Bits& b) {
        (void)a;
        (void)b;
        FpuResult r = make_zero_fpu_result();
        r.trace.push_back("fmul_f32 stub");
        return r;
    }

} // namespace rv::core

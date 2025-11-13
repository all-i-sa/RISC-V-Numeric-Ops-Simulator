//
// Created by Allisa Warren on 11/12/25.
//
#include "core/mdu.hpp"
#include "core/bitvec.hpp"
#include "core/twos.hpp"
#include <cassert>

namespace rv::core {

        namespace {
        struct AddResult {
            Bits sum;
            Bit  carry_out;
        };

        // Generic ripple-carry adder on the low `width` bits of a and b.
        AddResult add_fixed_width(const Bits& a, const Bits& b, std::size_t width) {
            Bits sum(width, 0);
            Bit carry = 0;

            for (std::size_t i = 0; i < width; ++i) {
                Bit ai = (i < a.size()) ? a[i] : 0;
                Bit bi = (i < b.size()) ? b[i] : 0;

                Bit partial    = ai ^ bi;
                Bit s          = partial ^ carry;
                Bit carry_next = (ai & bi) | (ai & carry) | (bi & carry);

                sum[i] = s;
                carry  = carry_next;
            }

            AddResult res{ sum, carry };
            return res;
        }

        // Two's-complement negate for arbitrary width (LSB-first).
        Bits twos_negate_fixed(const Bits& v, std::size_t width) {
            Bits inv(width, 0);
            for (std::size_t i = 0; i < width; ++i) {
                Bit bit = (i < v.size()) ? v[i] : 0;
                inv[i] = bit ^ 1; // invert
            }

            Bits one(width, 0);
            one[0] = 1;

            AddResult add_res = add_fixed_width(inv, one, width);
            (void)add_res; // carry-out beyond width is discarded
            return add_res.sum;
        }
    } // anonymous namespace

    MulResult mdu_mul(MulOp op, const Bits& rs1, const Bits& rs2) {
        // For now we implement MUL (signed 32 x 32 -> low 32) fully.
        // Other MulOp variants can be added later.
        Bits rs1_32 = zero_extend(rs1, 32);
        Bits rs2_32 = zero_extend(rs2, 32);

        // Decode to (sign, magnitude) using your existing helper.
        SignMag32 sm1 = decode_i32_to_sign_and_magnitude(rs1_32);
        SignMag32 sm2 = decode_i32_to_sign_and_magnitude(rs2_32);

        Bit sign1 = sm1.sign; // 0 = non-negative, 1 = negative
        Bit sign2 = sm2.sign;
        Bit sign_res = sign1 ^ sign2; // result sign

        Bits mag1_32 = zero_extend(sm1.mag, 32); // multiplicand magnitude
        Bits mag2_32 = zero_extend(sm2.mag, 32); // multiplier magnitude

        // 64-bit product register: high 32 bits = accumulator, low 32 bits = multiplier.
        Bits p(64, 0);
        for (std::size_t i = 0; i < 32; ++i) {
            p[i] = mag2_32[i]; // low half holds multiplier
        }
        // high half initially 0

        std::vector<std::string> trace;

        // Helper to snapshot acc (hi) and mul (lo) to trace.
        auto snapshot = [&](std::size_t step) {
            Bits lo(32, 0);
            Bits hi(32, 0);
            for (std::size_t i = 0; i < 32; ++i) {
                lo[i] = p[i];
                hi[i] = p[32 + i];
            }
            std::string msg = "step " + std::to_string(step) +
                              ": acc=" + bv_to_hex_string(hi) +
                              " mul=" + bv_to_hex_string(lo);
            trace.push_back(msg);
        };

        // Shift-add unsigned multiply on magnitudes.
        for (std::size_t step = 0; step < 32; ++step) {
            // Snapshot at start of this step
            snapshot(step);

            // If LSB of multiplier (p[0]) is 1, add multiplicand into high half.
            if (p[0] == 1) {
                Bits hi(32, 0);
                for (std::size_t i = 0; i < 32; ++i) {
                    hi[i] = p[32 + i];
                }

                AddResult add_res = add_fixed_width(hi, mag1_32, 32);

                // Store sum back into high half.
                for (std::size_t i = 0; i < 32; ++i) {
                    p[32 + i] = add_res.sum[i];
                }
                // carry_out, if any, would conceptually go into bit 64,
                // but for 32x32 -> 64-bit products, true results fit in 64 bits,
                // so we can discard it safely.
            }

            // Shift the entire 64-bit register right by 1 (logical).
            // LSB-first: new bit i = old bit (i+1), MSB (bit 63) gets 0.
            for (std::size_t i = 0; i + 1 < 64; ++i) {
                p[i] = p[i + 1];
            }
            p[63] = 0;
            // After this, p has been shifted right by 1; MSB filled with 0.
        }

        // Final snapshot after all steps.
        snapshot(32);

        // At this point, p holds the unsigned 64-bit magnitude product.
        Bits mag_prod_64 = p; // 64 bits

        // Apply the sign to get the signed 64-bit product bits.
        Bits signed_prod_64(64, 0);
        if (sign_res == 0) {
            signed_prod_64 = mag_prod_64;
        } else {
            signed_prod_64 = twos_negate_fixed(mag_prod_64, 64);
        }

        // Split into hi/lo 32-bit parts.
        Bits lo(32, 0);
        Bits hi(32, 0);
        for (std::size_t i = 0; i < 32; ++i) {
            lo[i] = signed_prod_64[i];       // bits 0..31
            hi[i] = signed_prod_64[32 + i];  // bits 32..63
        }

        // Overflow flag: true if signed 64-bit product not representable as signed 32-bit.
        Bit sign32 = signed_prod_64[31];
        bool overflow = false;
        for (std::size_t i = 32; i < 64; ++i) {
            if (signed_prod_64[i] != sign32) {
                overflow = true;
                break;
            }
        }

        MulResult res{
            /*lo=*/lo,
            /*hi=*/hi,
            /*overflow=*/overflow,
            /*trace=*/trace
        };

        // For now, we ignore op and always behave like MUL (signed low 32 bits).
        // Later, Mulh/Mulhu/Mulhsu can reuse the same core product.
        (void)op;

        return res;
    }

    DivResult mdu_div(DivOp op, const Bits& rs1, const Bits& rs2) {
        (void)op; // unused for now

        // Zero-extend to 32 bits for consistency.
        Bits rs1_32 = zero_extend(rs1, 32);
        Bits rs2_32 = zero_extend(rs2, 32);
        (void)rs1_32;
        (void)rs2_32;

        // Stub: quotient and remainder both zero, no overflow, empty trace.
        Bits q(32, 0);
        Bits r(32, 0);

        DivResult res{
            /*q=*/q,
            /*r=*/r,
            /*overflow=*/false,
            /*trace=*/{}
        };
        return res;
    }

} // namespace rv::core

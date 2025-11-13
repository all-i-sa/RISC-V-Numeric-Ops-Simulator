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

        // ---------- Helpers for unsigned compare/sub/zero checks (32-bit) ----------

        int compare_unsigned_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            // Compare from MSB down to LSB
            for (int i = 31; i >= 0; --i) {
                if (a[static_cast<std::size_t>(i)] != b[static_cast<std::size_t>(i)]) {
                    return (a[static_cast<std::size_t>(i)] < b[static_cast<std::size_t>(i)]) ? -1 : 1;
                }
            }
            return 0;
        }

        Bits subtract_unsigned_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            Bits diff(32, 0);
            Bit borrow = 0;

            for (std::size_t i = 0; i < 32; ++i) {
                Bit ai = a[i];
                Bit bi = b[i];
                Bit bin = borrow;

                // diff bit: ai - bi - bin
                Bit d = ai ^ bi ^ bin;
                diff[i] = d;

                // borrow_out = (!ai & (bi | bin)) | (bi & bin)
                Bit not_ai = ai ^ 1;
                Bit borrow_out = (not_ai & (bi | bin)) | (bi & bin);
                borrow = borrow_out;
            }

            // Caller guarantees a >= b, so final borrow should be 0.
            return diff;
        }

        bool is_zero_32(const Bits& x) {
            assert(x.size() == 32);
            for (Bit bit : x) {
                if (bit != 0) return false;
            }
            return true;
        }

        bool is_all_ones_32(const Bits& x) {
            assert(x.size() == 32);
            for (Bit bit : x) {
                if (bit != 1) return false;
            }
            return true;
        }

        bool is_int_min_32(const Bits& x) {
            assert(x.size() == 32);
            // 0x80000000 → MSB (bit 31) = 1, all others 0
            if (x[31] != 1) return false;
            for (std::size_t i = 0; i < 31; ++i) {
                if (x[i] != 0) return false;
            }
            return true;
        }

        struct UnsignedDivResult {
            Bits q;  // quotient
            Bits r;  // remainder
            std::vector<std::string> trace;
        };

        UnsignedDivResult div_unsigned_32(const Bits& dividend, const Bits& divisor) {
            assert(dividend.size() == 32);
            assert(divisor.size() == 32);
            assert(!is_zero_32(divisor)); // caller must handle divide-by-zero

            Bits R(32, 0); // remainder
            Bits Q(32, 0); // quotient

            std::vector<std::string> trace;

            auto snapshot = [&](int step) {
                std::string msg = "step " + std::to_string(step) +
                                  ": R=" + bv_to_hex_string(R) +
                                  " Q=" + bv_to_hex_string(Q);
                trace.push_back(msg);
            };

            // Classic restoring division, MSB-first loop.
            for (int i = 31; i >= 0; --i) {
                // Shift R left by 1
                for (int j = 31; j >= 1; --j) {
                    R[static_cast<std::size_t>(j)] = R[static_cast<std::size_t>(j - 1)];
                }
                R[0] = 0;

                // Bring down bit i of dividend into LSB of R
                R[0] = dividend[static_cast<std::size_t>(i)];

                // If R >= divisor, subtract and set quotient bit i = 1
                int cmp = compare_unsigned_32(R, divisor);
                if (cmp >= 0) {
                    R = subtract_unsigned_32(R, divisor);
                    Q[static_cast<std::size_t>(i)] = 1;
                } else {
                    Q[static_cast<std::size_t>(i)] = 0;
                }

                snapshot(31 - i);
            }

            UnsignedDivResult res{ Q, R, trace };
            return res;
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
        Bits rs1_32 = zero_extend(rs1, 32); // dividend
        Bits rs2_32 = zero_extend(rs2, 32); // divisor

        // For now, fully implement signed DIV; other ops fall back to simple behavior.
        if (op != DivOp::Div) {
            // Minimal behavior for optional ops for now.
            Bits q(32, 0);
            Bits r(32, 0);
            DivResult res{ q, r, false, {} };
            return res;
        }

        // Decode to sign/magnitude to get absolute values.
        SignMag32 sm1 = decode_i32_to_sign_and_magnitude(rs1_32);
        SignMag32 sm2 = decode_i32_to_sign_and_magnitude(rs2_32);

        Bits mag1 = zero_extend(sm1.mag, 32);
        Bits mag2 = zero_extend(sm2.mag, 32);

        bool divisor_is_zero = is_zero_32(mag2);
        bool dividend_is_int_min = is_int_min_32(rs1_32);
        bool divisor_is_minus_one = is_all_ones_32(rs2_32); // -1 = 0xFFFFFFFF

        // Case 1: divide-by-zero
        if (divisor_is_zero) {
            // RISC-V DIV x / 0 → quotient = -1 (0xFFFFFFFF), remainder = dividend.
            Bits q(32, 1); // all ones
            Bits r = rs1_32;
            std::vector<std::string> trace;
            trace.push_back("divide-by-zero: q=-1, r=dividend");
            DivResult res{ q, r, false, trace };
            return res;
        }

        // Case 2: INT_MIN / -1 special case
        if (dividend_is_int_min && divisor_is_minus_one) {
            // RISC-V: quotient = INT_MIN, remainder = 0, overflow flag = true.
            Bits q = rs1_32;        // 0x80000000
            Bits r(32, 0);          // 0
            std::vector<std::string> trace;
            trace.push_back("INT_MIN / -1 special case");
            DivResult res{ q, r, true, trace };
            return res;
        }

        // General signed division via unsigned magnitudes.
        Bit sign1 = sm1.sign; // dividend sign
        Bit sign2 = sm2.sign; // divisor sign
        Bit sign_q = sign1 ^ sign2; // quotient sign (trunc toward zero)
        // Remainder sign follows dividend sign.

        UnsignedDivResult ures = div_unsigned_32(mag1, mag2);
        Bits q_abs = ures.q;
        Bits r_abs = ures.r;

        // Apply sign to quotient
        Bits q_signed(32, 0);
        if (sign_q == 0) {
            q_signed = q_abs;
        } else {
            q_signed = twos_negate_fixed(q_abs, 32);
        }

        // Apply sign to remainder (same sign as dividend)
        Bits r_signed(32, 0);
        if (sign1 == 0) {
            r_signed = r_abs;
        } else {
            r_signed = twos_negate_fixed(r_abs, 32);
        }

        bool overflow = false; // only INT_MIN / -1 sets overflow, handled above

        DivResult res{
            /*q=*/q_signed,
            /*r=*/r_signed,
            /*overflow=*/overflow,
            /*trace=*/ures.trace
        };
        return res;
    }


} // namespace rv::core

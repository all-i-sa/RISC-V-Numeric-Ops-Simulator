#include "core/mdu.hpp"
#include "core/bitvec.hpp"
#include "core/twos.hpp"
#include <cassert>

namespace rv::core {

    namespace {

        /***** AddResult *****
         *   The result of adding two bit vectors
         *   sum       - the bitwise sum
         *   carry_out
         ******************************/
        struct AddResult {
            Bits sum;
            Bit  carry_out;
        };

        /***** add_fixed_width *****
         *   Adds two bit vectors as unsigned numbers
         ******************************
         * Inputs:
         *   a, b  - input bits
         *   width - how many bits to keep
         * Returns:
         *   AddResult - sum bits + carry_out
         ******************************/
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

        /***** twos_negate_fixed *****
         *   Computes the 2's comp negative of a value
         *   - First flips all bits
         *   - Then adds 1 using the same adder
         *   - Any carry past the top bit is thrown away
         ******************************
         * Inputs:
         *   v     - original value bits
         *   width - how many bits to use for the result
         * Returns:
         *   Bits - negative of v in 2's comp
         ******************************/
        Bits twos_negate_fixed(const Bits& v, std::size_t width) {
            Bits inv(width, 0);
            for (std::size_t i = 0; i < width; ++i) {
                Bit bit = (i < v.size()) ? v[i] : 0;
                inv[i] = bit ^ 1;
            }

            Bits one(width, 0);
            one[0] = 1;

            AddResult add_res = add_fixed_width(inv, one, width);
            (void)add_res;

            return add_res.sum;
        }

        /***** compare_unsigned_32 *****
         *   Compares two 32-bit values as unsigned int
         ******************************
         * Returns:
         *   -1 if a < b
         *    0 if a == b
         *    1 if a > b
         ******************************/
        int compare_unsigned_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            for (int i = 31; i >= 0; --i) {
                if (a[static_cast<std::size_t>(i)] != b[static_cast<std::size_t>(i)]) {
                    return (a[static_cast<std::size_t>(i)] < b[static_cast<std::size_t>(i)]) ? -1 : 1;
                }
            }
            return 0;
        }

        /***** subtract_unsigned_32 *****
         *   Subtracts two 32-bit unsigned values: a - b.
         * *****************************
         * Returns:
         *   Bits - 32-bit diff
         ******************************/
        Bits subtract_unsigned_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            Bits diff(32, 0);
            Bit borrow = 0;

            for (std::size_t i = 0; i < 32; ++i) {
                Bit ai = a[i];
                Bit bi = b[i];
                Bit bin = borrow;

                Bit d = ai ^ bi ^ bin;
                diff[i] = d;

                Bit not_ai = ai ^ 1;
                Bit borrow_out = (not_ai & (bi | bin)) | (bi & bin);
                borrow = borrow_out;
            }

            return diff;
        }

        /***** is_zero_32 *****
         *   Checks if a 32-bit value is exactly zero
         ******************************/
        bool is_zero_32(const Bits& x) {
            assert(x.size() == 32);
            for (Bit bit : x) {
                if (bit != 0) return false;
            }
            return true;
        }

        /***** is_all_ones_32 *****
         *   Checks if a 32-bit value is all 1s
         ******************************/
        bool is_all_ones_32(const Bits& x) {
            assert(x.size() == 32);
            for (Bit bit : x) {
                if (bit != 1) return false;
            }
            return true;
        }

        /***** is_int_min_32 *****
         *   Checks if a 32-bit value is the minimum signed int
         ******************************/
        bool is_int_min_32(const Bits& x) {
            assert(x.size() == 32);
            // 0x80000000 → MSB (bit 31) = 1, all others 0
            if (x[31] != 1) return false;
            for (std::size_t i = 0; i < 31; ++i) {
                if (x[i] != 0) return false;
            }
            return true;
        }

        /***** UnsignedDivResult *****
         *   The result of doing unsigned division
         *
         *   q     - quotient bits
         *   r     - remainder bits
         *   trace
         ******************************/
        struct UnsignedDivResult {
            Bits q;
            Bits r;
            std::vector<std::string> trace;
        };

        /***** div_unsigned_32 *****
         *   Divides one 32-bit unsigned number by another
         ******************************
         * Inputs:
         *   dividend - 32-bit unsigned value
         *   divisor  - 32-bit unsigned value
         * Returns:
         *   UnsignedDivResult
         ******************************/
        UnsignedDivResult div_unsigned_32(const Bits& dividend, const Bits& divisor) {
            assert(dividend.size() == 32);
            assert(divisor.size() == 32);
            assert(!is_zero_32(divisor));

            Bits R(32, 0);
            Bits Q(32, 0);

            std::vector<std::string> trace;

            auto snapshot = [&](int step) {
                std::string msg = "step " + std::to_string(step) +
                                  ": R=" + bv_to_hex_string(R) +
                                  " Q=" + bv_to_hex_string(Q);
                trace.push_back(msg);
            };

            for (int i = 31; i >= 0; --i) {
                // Shift R left by 1.
                for (int j = 31; j >= 1; --j) {
                    R[static_cast<std::size_t>(j)] = R[static_cast<std::size_t>(j - 1)];
                }
                R[0] = 0;

                R[0] = dividend[static_cast<std::size_t>(i)];

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

    /***** mdu_mul *****
     *   Multiplies two 32-bit values
     * NOT FULLY IMPLEMENTED YET:
     *   - Right now, it behaves like MUL
     *   - The MulOp parameter is ignored right now
     ******************************/
    MulResult mdu_mul(MulOp op, const Bits& rs1, const Bits& rs2) {
        Bits rs1_32 = zero_extend(rs1, 32);
        Bits rs2_32 = zero_extend(rs2, 32);

        SignMag32 sm1 = decode_i32_to_sign_and_magnitude(rs1_32);
        SignMag32 sm2 = decode_i32_to_sign_and_magnitude(rs2_32);

        Bit sign1 = sm1.sign;
        Bit sign2 = sm2.sign;
        Bit sign_res = sign1 ^ sign2;

        Bits mag1_32 = zero_extend(sm1.mag, 32);
        Bits mag2_32 = zero_extend(sm2.mag, 32);

        Bits p(64, 0);
        for (std::size_t i = 0; i < 32; ++i) {
            p[i] = mag2_32[i];
        }

        std::vector<std::string> trace;

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

        for (std::size_t step = 0; step < 32; ++step) {
            snapshot(step);

            if (p[0] == 1) {
                Bits hi(32, 0);
                for (std::size_t i = 0; i < 32; ++i) {
                    hi[i] = p[32 + i];
                }

                AddResult add_res = add_fixed_width(hi, mag1_32, 32);

                for (std::size_t i = 0; i < 32; ++i) {
                    p[32 + i] = add_res.sum[i];
                }
            }

            for (std::size_t i = 0; i + 1 < 64; ++i) {
                p[i] = p[i + 1];
            }
            p[63] = 0;
        }

        snapshot(32);

        Bits mag_prod_64 = p; // 64 bits

        Bits signed_prod_64(64, 0);
        if (sign_res == 0) {
            signed_prod_64 = mag_prod_64;
        } else {
            signed_prod_64 = twos_negate_fixed(mag_prod_64, 64);
        }

        Bits lo(32, 0);
        Bits hi(32, 0);
        for (std::size_t i = 0; i < 32; ++i) {
            lo[i] = signed_prod_64[i];
            hi[i] = signed_prod_64[32 + i];
        }

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

        (void)op;

        return res;
    }

    /***** mdu_div *****
     *   Divides one 32-bit value by another
     *  NOT FULLY IMPLEMENTED:
     *   - only DivOp::Div signed divide is handled
     *   - Divu, Rem, Remu  return zeros for now
     *   - Handles special RISC-V rules:
     ******************************
     * Inputs:
     *   op  - which divide/remainder mode to use
     *   rs1 - dividend
     *   rs2 - divisor
     * Returns:
     *   DivResult
     ******************************/
    DivResult mdu_div(DivOp op, const Bits& rs1, const Bits& rs2) {
        Bits rs1_32 = zero_extend(rs1, 32); // dividend
        Bits rs2_32 = zero_extend(rs2, 32); // divisor

        if (op != DivOp::Div) {
            Bits q(32, 0);
            Bits r(32, 0);
            DivResult res{ q, r, false, {} };
            return res;
        }

        SignMag32 sm1 = decode_i32_to_sign_and_magnitude(rs1_32);
        SignMag32 sm2 = decode_i32_to_sign_and_magnitude(rs2_32);

        Bits mag1 = zero_extend(sm1.mag, 32);
        Bits mag2 = zero_extend(sm2.mag, 32);

        bool divisor_is_zero = is_zero_32(mag2);
        bool dividend_is_int_min = is_int_min_32(rs1_32);
        bool divisor_is_minus_one = is_all_ones_32(rs2_32);

        if (divisor_is_zero) {
            Bits q(32, 1);
            Bits r = rs1_32;
            std::vector<std::string> trace;
            trace.push_back("divide-by-zero: q=-1, r=dividend");
            DivResult res{ q, r, false, trace };
            return res;
        }

        if (dividend_is_int_min && divisor_is_minus_one) {
            Bits q = rs1_32;
            Bits r(32, 0);
            std::vector<std::string> trace;
            trace.push_back("INT_MIN / -1 special case");
            DivResult res{ q, r, true, trace };
            return res;
        }

        Bit sign1 = sm1.sign;
        Bit sign2 = sm2.sign;
        Bit sign_q = sign1 ^ sign2;

        UnsignedDivResult ures = div_unsigned_32(mag1, mag2);
        Bits q_abs = ures.q;
        Bits r_abs = ures.r;

        Bits q_signed(32, 0);
        if (sign_q == 0) {
            q_signed = q_abs;
        } else {
            q_signed = twos_negate_fixed(q_abs, 32);
        }

        Bits r_signed(32, 0);
        if (sign1 == 0) {
            r_signed = r_abs;
        } else {
            r_signed = twos_negate_fixed(r_abs, 32);
        }

        bool overflow = false;

        DivResult res{
            /*q=*/q_signed,
            /*r=*/r_signed,
            /*overflow=*/overflow,
            /*trace=*/ures.trace
        };
        return res;
    }
} // namespace rv::core

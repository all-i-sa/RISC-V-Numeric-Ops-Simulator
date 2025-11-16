#include "core/f32.hpp"
#include "core/bitvec.hpp"
#include <cassert>

namespace rv::core {

namespace {

    /***** compare_unsigned *****
     *   Compares two bit vectors as unsigned numbers
     ******************************
     * Inputs:
     *   a, b - bit vectors to compare
     * Returns:
     *   -1 if a < b
     *    0 if a == b
     *    1 if a > b
     ******************************/
    int compare_unsigned(const Bits& a, const Bits& b) {
        std::size_t width = (a.size() < b.size()) ? a.size() : b.size();
        if (width == 0) return 0;

        // Compare from MSB down to LSB
        for (int i = static_cast<int>(width) - 1; i >= 0; --i) {
            Bit ai = a[static_cast<std::size_t>(i)];
            Bit bi = b[static_cast<std::size_t>(i)];
            if (ai != bi) {
                return (ai < bi) ? -1 : 1;
            }
        }
        return 0;
    }

    /***** add_unsigned *****
     *   Adds two unsigned bit vectors together
     *   - Pads missing bits with 0.
     *   - Uses a ripple carry style add
     ******************************
     * Inputs:
     *   a, b   - input bit vectors
     *   width  - how many bits of result to keep
     *   carry_out - set to 1 if there's a carry out of the top bit
     * Returns:
     *   Bits - sum of a and b
     ******************************/
    Bits add_unsigned(const Bits& a, const Bits& b, std::size_t width, Bit& carry_out) {
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

        carry_out = carry;
        return sum;
    }

    /***** subtract_unsigned *****
     *   Computes a - b for unsigned bit vectors
     *   - Pads missing bits with 0
     ******************************
     * Inputs:
     *   a, b       - input bit vectors
     *   width      - how many bits of result to keep
     *   borrow_out - set to 1 if a < b and we had to borrow past the top bit
     * Returns:
     *   Bits - difference a - b
     ******************************/
    Bits subtract_unsigned(const Bits& a, const Bits& b, std::size_t width, Bit& borrow_out) {
        Bits diff(width, 0);
        Bit borrow = 0;

        for (std::size_t i = 0; i < width; ++i) {
            Bit ai = (i < a.size()) ? a[i] : 0;
            Bit bi = (i < b.size()) ? b[i] : 0;
            Bit bin = borrow;

            // diff bit: ai - bi - bin
            Bit d = ai ^ bi ^ bin;
            diff[i] = d;

            // borrow_out = (!ai & (bi | bin)) | (bi & bin)
            Bit not_ai = ai ^ 1;
            Bit borrow_next = (not_ai & (bi | bin)) | (bi & bin);
            borrow = borrow_next;
        }
        borrow_out = borrow;
        return diff;
    }

    /***** shift_right_logical *****
     *   Shifts a bit vector to the right by 1 bit
     ******************************
     * Inputs:
     *   v     - bit vector to change in place
     *   width - number of bits to consider
     ******************************/
    void shift_right_logical(Bits& v, std::size_t width) {
        if (width == 0) return;
        for (std::size_t i = 0; i + 1 < width; ++i) {
            v[i] = v[i + 1];
        }
        v[width - 1] = 0;
    }

    /***** shift_left_logical *****
     *   Shifts a bit vector to the left by 1 bit
     *   - The new bit at position 0 becomes 0.
     ******************************
     * Inputs:
     *   v     - bit vector to change in place
     *   width - number of bits to consider
     ******************************/
    void shift_left_logical(Bits& v, std::size_t width) {
        if (width == 0) return;
        for (std::size_t i = width - 1; i > 0; --i) {
            v[i] = v[i - 1];
        }
        v[0] = 0;
    }

    /***** bits_all_zero *****
     *   Checks if all bits in the vector are 0
     ******************************/
    bool bits_all_zero(const Bits& x) {
        for (Bit bit : x) {
            if (bit != 0) return false;
        }
        return true;
    }

    /***** bits_all_ones *****
     *   Checks if all bits in the vector are 1
     ******************************/
    bool bits_all_ones(const Bits& x) {
        for (Bit bit : x) {
            if (bit != 1) return false;
        }
        return true;
    }

} // anonymous namespace

    /***** unpack_f32 *****
     *   Takes 32 bits that represent a float32 value and splits them into:
     *   - sign bit
     *   - exponent bits (8)
     *   - fraction bits (23)
     ******************************
     * Inputs:
     *   bits - bit vector
     * Returns:
     *   F32Fields - sign, exponent, and fraction
     ******************************/
    F32Fields unpack_f32(const Bits& bits) {
        // Ensure exactly 32 bits.
        Bits b32 = zero_extend(bits, 32);

        F32Fields f{};
        f.sign = b32[31];

        f.fraction = Bits(23, 0);
        for (std::size_t i = 0; i < 23; ++i) {
            f.fraction[i] = b32[i];
        }

        f.exponent = Bits(8, 0);
        for (std::size_t i = 0; i < 8; ++i) {
            f.exponent[i] = b32[23 + i];
        }

        return f;
    }

    /***** pack_f32 *****
     *   Takes sign, exponent, and fraction and packs them into
     *   a single 32-bit float pattern
     ******************************
     * Inputs:
     *   f - F32Fields with sign, exponent, and fraction
     * Returns:
     *   Bits - 32-bit float pattern
     ******************************/
    Bits pack_f32(const F32Fields& f) {
        Bits b32(32, 0);

        for (std::size_t i = 0; i < 23 && i < f.fraction.size(); ++i) {
            b32[i] = f.fraction[i];
        }

        for (std::size_t i = 0; i < 8 && i < f.exponent.size(); ++i) {
            b32[23 + i] = f.exponent[i];
        }

        b32[31] = f.sign;

        return b32;
    }

    /***** make_zero_fpu_result *****
     *   Builds a default FpuResult with:
     *   - all bits set to 0
     *   - all flags set to false
     *   - an empty trace
     *
     * Returns:
     *   FpuResult - a blank float result
     ******************************/
    FpuResult make_zero_fpu_result() {
        FpuResult r;
        r.bits = Bits(32, 0);
        r.flags = FpuFlags{false, false, false, false};
        r.trace.clear();
        return r;
    }

    /***** fadd_f32 *****
     *   Adds two float32 values
     ******************************
     * Inputs:
     *   a - first float32 value
     *   b - second float32 value
     * Returns:
     *   FpuResult
     ******************************/
    FpuResult fadd_f32(const Bits& a, const Bits& b) {
        FpuResult out = make_zero_fpu_result();
        out.trace.push_back("fadd_f32 start");

        Bits a32 = zero_extend(a, 32);
        Bits b32 = zero_extend(b, 32);

        F32Fields fa = unpack_f32(a32);
        F32Fields fb = unpack_f32(b32);

        if (bits_all_zero(fa.exponent) && bits_all_zero(fa.fraction)) {
            out.bits = b32;
            out.trace.push_back("a is zero → return b");
            return out;
        }
        if (bits_all_zero(fb.exponent) && bits_all_zero(fb.fraction)) {
            out.bits = a32;
            out.trace.push_back("b is zero → return a");
            return out;
        }

        Bits sigA(24, 0);
        Bits sigB(24, 0);
        for (std::size_t i = 0; i < 23; ++i) {
            sigA[i] = fa.fraction[i];
            sigB[i] = fb.fraction[i];
        }
        sigA[23] = 1;
        sigB[23] = 1;

        int cmp_exp = compare_unsigned(fa.exponent, fb.exponent);
        Bits exp_big(8, 0);
        Bits exp_small(8, 0);
        Bits sig_big(24, 0);
        Bits sig_small(24, 0);
        Bit sign_big = 0;
        Bit sign_small = 0;

        if (cmp_exp >= 0) {
            exp_big    = fa.exponent;
            exp_small  = fb.exponent;
            sig_big    = sigA;
            sig_small  = sigB;
            sign_big   = fa.sign;
            sign_small = fb.sign;
        } else {
            exp_big    = fb.exponent;
            exp_small  = fa.exponent;
            sig_big    = sigB;
            sig_small  = sigA;
            sign_big   = fb.sign;
            sign_small = fa.sign;
        }

        Bits exp_tmp = exp_big;
        Bits sig_small_aligned = sig_small;

        Bits one_exp(8, 0);
        one_exp[0] = 1;

        while (compare_unsigned(exp_tmp, exp_small) > 0) {
            // Shift significand right by 1
            shift_right_logical(sig_small_aligned, 24);

            // Decrement exp_tmp by 1
            Bit borrow_e = 0;
            exp_tmp = subtract_unsigned(exp_tmp, one_exp, 8, borrow_e);
            if (borrow_e == 1) {
                break;
            }
        }

        if (sign_big == sign_small) {
            Bit carry = 0;
            Bits sig_sum = add_unsigned(sig_big, sig_small_aligned, 24, carry);

            Bits exp_res = exp_big;
            // If there was a carry out of the top bit, shift right and bump the exponent
            if (carry == 1) {
                shift_right_logical(sig_sum, 24);

                Bits one(8, 0);
                one[0] = 1;
                Bit carry_e = 0;
                exp_res = add_unsigned(exp_res, one, 8, carry_e);
                // ignore exponent overflow
            }

            F32Fields fres{};
            fres.sign = sign_big;
            fres.exponent = exp_res;
            fres.fraction = Bits(23, 0);

            for (std::size_t i = 0; i < 23; ++i) {
                fres.fraction[i] = sig_sum[i];
            }

            out.bits = pack_f32(fres);
            out.trace.push_back("fadd_f32 normal same-sign add");
            return out;
        }

        Bits sig_big_local   = sig_big;
        Bits sig_small_local = sig_small_aligned;

        int mag_cmp = compare_unsigned(sig_big_local, sig_small_local);

        Bit result_sign = sign_big;

        if (mag_cmp < 0) {
            Bits tmp = sig_big_local;
            sig_big_local = sig_small_local;
            sig_small_local = tmp;
            result_sign = sign_small;
        } else if (mag_cmp == 0) {
            F32Fields fres{};
            fres.sign     = 0;           // +0
            fres.exponent = Bits(8, 0);  // all zero
            fres.fraction = Bits(23, 0); // all zero

            out.bits = pack_f32(fres);
            out.trace.push_back("fadd_f32 different-sign: exact zero");
            return out;
        }

        Bit borrow = 0;
        Bits sig_diff = subtract_unsigned(sig_big_local, sig_small_local, 24, borrow);

        Bits exp_res = exp_big;

        if (bits_all_zero(sig_diff)) {
            F32Fields fres{};
            fres.sign     = 0;
            fres.exponent = Bits(8, 0);
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fadd_f32 different-sign: diff zero");
            return out;
        }

        // Shift left until the top bit of the 24-bit value is 1,
        // or until exponent underflows
        while (sig_diff[23] == 0 && !bits_all_zero(sig_diff)) {
            shift_left_logical(sig_diff, 24);

            Bit borrow_e = 0;
            exp_res = subtract_unsigned(exp_res, one_exp, 8, borrow_e);
            if (borrow_e == 1) {
                // We ran out of exponent; this would fall into subnormal/underflow.
                break;
            }
        }

        // Pack result
        F32Fields fres{};
        fres.sign     = result_sign;
        fres.exponent = exp_res;
        fres.fraction = Bits(23, 0);

        for (std::size_t i = 0; i < 23; ++i) {
            fres.fraction[i] = sig_diff[i];
        }

        out.bits = pack_f32(fres);
        out.trace.push_back("fadd_f32 different-sign subtract");
        return out;
    }

    /***** fsub_f32 *****
     *   Subtracts two float32 values: a - b.
     ******************************
     * Inputs:
     *   a - first float32 value
     *   b - second float32 value
     *
     * Returns:
     *   FpuResult - result bits, flags, and trace from fadd_f32.
     ******************************/
    FpuResult fsub_f32(const Bits& a, const Bits& b) {
        Bits b32 = zero_extend(b, 32);
        Bits b_neg = b32;
        b_neg[31] = b32[31] ^ 1; // flip sign bit

        return fadd_f32(a, b_neg);
    }

    /***** fmul_f32 *****
     *   Multiplies two float32 values
     ******************************
     * Inputs:
     *   a - first float32 value
     *   b - second float32 value
     * Returns:
     *   FpuResult
     ******************************/
    FpuResult fmul_f32(const Bits& a, const Bits& b) {
        FpuResult out = make_zero_fpu_result();
        out.trace.push_back("fmul_f32 start");

        Bits a32 = zero_extend(a, 32);
        Bits b32 = zero_extend(b, 32);

        F32Fields fa = unpack_f32(a32);
        F32Fields fb = unpack_f32(b32);

        Bit sign_res = fa.sign ^ fb.sign;

        bool expA_zero = bits_all_zero(fa.exponent);
        bool expB_zero = bits_all_zero(fb.exponent);
        bool expA_ones = bits_all_ones(fa.exponent);
        bool expB_ones = bits_all_ones(fb.exponent);

        bool fracA_zero = bits_all_zero(fa.fraction);
        bool fracB_zero = bits_all_zero(fb.fraction);

        bool a_is_zero = expA_zero && fracA_zero;
        bool b_is_zero = expB_zero && fracB_zero;
        bool a_is_inf  = expA_ones && fracA_zero;
        bool b_is_inf  = expB_ones && fracB_zero;
        bool a_is_nan  = expA_ones && !fracA_zero;
        bool b_is_nan  = expB_ones && !fracB_zero;

        Bits nan_bits = bv_from_hex_string("0x7fc00000");

        if (a_is_nan || b_is_nan) {
            out.bits = nan_bits;
            out.flags.invalid = true;
            out.trace.push_back("fmul_f32: NaN operand");
            return out;
        }

        if ((a_is_inf && b_is_zero) || (b_is_inf && a_is_zero)) {
            out.bits = nan_bits;
            out.flags.invalid = true;
            out.trace.push_back("fmul_f32: 0 * inf invalid");
            return out;
        }

        if (a_is_inf || b_is_inf) {
            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            for (std::size_t i = 0; i < 8; ++i) {
                fres.exponent[i] = 1; // all ones → exponent = 255
            }
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: inf result");
            return out;
        }

        if (a_is_zero || b_is_zero) {
            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: zero result");
            return out;
        }

        Bits expA = fa.exponent;
        Bits expB = fb.exponent;

        Bits expA9(9, 0);
        Bits expB9(9, 0);
        for (std::size_t i = 0; i < 8; ++i) {
            expA9[i] = expA[i];
            expB9[i] = expB[i];
        }

        Bit carry9 = 0;
        Bits exp_sum9 = add_unsigned(expA9, expB9, 9, carry9);

        Bits thresh382(9, 0);
        thresh382[0] = 0;
        thresh382[1] = 1;
        thresh382[2] = 1;
        thresh382[3] = 1;
        thresh382[4] = 1;
        thresh382[5] = 1;
        thresh382[6] = 1;
        thresh382[7] = 0;
        thresh382[8] = 1;

        if (compare_unsigned(exp_sum9, thresh382) >= 0) {
            out.flags.overflow = true;

            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            for (std::size_t i = 0; i < 8; ++i) {
                fres.exponent[i] = 1; // 0xFF → +∞ or -∞
            }
            fres.fraction = Bits(23, 0);

            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: pre-check exponent overflow");
            return out;
        }

        Bit carry_add = 0;
        Bits exp_sum = add_unsigned(expA, expB, 8, carry_add);

        Bits bias(8, 0);

        for (std::size_t i = 0; i < 7; ++i) {
            bias[i] = 1;
        }
        bias[7] = 0;

        Bit borrow_bias = 0;
        Bits exp_tmp = subtract_unsigned(exp_sum, bias, 8, borrow_bias);

        if (borrow_bias == 1) {
            out.flags.underflow = true;
            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: exponent underflow before normalization");
            return out;
        }

        Bits sigA(24, 0);
        Bits sigB(24, 0);
        for (std::size_t i = 0; i < 23; ++i) {
            sigA[i] = fa.fraction[i];
            sigB[i] = fb.fraction[i];
        }
        if (!expA_zero) sigA[23] = 1;
        if (!expB_zero) sigB[23] = 1;

        Bits prod(48, 0);
        Bits multiplicand(48, 0);
        for (std::size_t i = 0; i < 24; ++i) {
            multiplicand[i] = sigA[i];
        }
        Bits multiplier = sigB;

        for (std::size_t step = 0; step < 24; ++step) {
            if (multiplier[0] == 1) {
                Bit carry_p = 0;
                Bits tmp = add_unsigned(prod, multiplicand, 48, carry_p);
                prod = tmp;
            }
            shift_right_logical(multiplier, 24);
            shift_left_logical(multiplicand, 48);
        }

        out.trace.push_back("fmul_f32: after significand multiply");

        bool high = (prod[47] == 1);
        Bits exp_res = exp_tmp;

        if (high) {
            Bits one(8, 0);
            one[0] = 1;
            Bit carry_e = 0;
            exp_res = add_unsigned(exp_res, one, 8, carry_e);
            if (carry_e == 1) {
                out.flags.overflow = true;
                F32Fields fres{};
                fres.sign = sign_res;
                fres.exponent = Bits(8, 0);
                for (std::size_t i = 0; i < 8; ++i) {
                    fres.exponent[i] = 1; // all ones
                }
                fres.fraction = Bits(23, 0);
                out.bits = pack_f32(fres);
                out.trace.push_back("fmul_f32: exponent overflow after normalization");
                return out;
            }
        }

        std::size_t shift = high ? 24 : 23;
        Bits sig_res(24, 0);
        for (std::size_t i = 0; i < 24; ++i) {
            std::size_t idx = i + shift;
            if (idx < 48) {
                sig_res[i] = prod[idx];
            }
        }

        bool exp_all_zero = bits_all_zero(exp_res);
        bool exp_all_ones = bits_all_ones(exp_res);

        if (exp_all_zero) {
            out.flags.underflow = true;
            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: underflow to zero");
            return out;
        }

        if (exp_all_ones) {
            out.flags.overflow = true;
            F32Fields fres{};
            fres.sign = sign_res;
            fres.exponent = Bits(8, 0);
            for (std::size_t i = 0; i < 8; ++i) {
                fres.exponent[i] = 1;
            }
            fres.fraction = Bits(23, 0);
            out.bits = pack_f32(fres);
            out.trace.push_back("fmul_f32: overflow to inf");
            return out;
        }

        F32Fields fres{};
        fres.sign = sign_res;
        fres.exponent = exp_res;
        fres.fraction = Bits(23, 0);
        for (std::size_t i = 0; i < 23; ++i) {
            fres.fraction[i] = sig_res[i];
        }

        out.bits = pack_f32(fres);
        out.trace.push_back("fmul_f32: normal finite result");
        return out;
    }

} // namespace rv::core

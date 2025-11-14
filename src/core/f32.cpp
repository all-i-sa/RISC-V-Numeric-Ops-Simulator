#include "core/f32.hpp"
#include "core/bitvec.hpp"
#include <cassert>

namespace rv::core {

namespace {

    // Generic unsigned compare: returns -1 if a < b, 0 if equal, 1 if a > b.
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

    // Unsigned add with fixed width, LSB-first.
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

    // Unsigned subtract: a - b for fixed width, LSB-first.
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

    // Logical right shift by 1 (LSB-first) over [0,width).
    void shift_right_logical(Bits& v, std::size_t width) {
        if (width == 0) return;
        for (std::size_t i = 0; i + 1 < width; ++i) {
            v[i] = v[i + 1];
        }
        v[width - 1] = 0;
    }

    // Logical left shift by 1 (LSB-first) over [0,width).
    void shift_left_logical(Bits& v, std::size_t width) {
        if (width == 0) return;
        for (std::size_t i = width - 1; i > 0; --i) {
            v[i] = v[i - 1];
        }
        v[0] = 0;
    }

    bool bits_all_zero(const Bits& x) {
        for (Bit bit : x) {
            if (bit != 0) return false;
        }
        return true;
    }

    bool bits_all_ones(const Bits& x) {
        for (Bit bit : x) {
            if (bit != 1) return false;
        }
        return true;
    }

} // anonymous namespace

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

// Basic add: handles normal finite numbers,
// same sign (addition) and different sign (subtraction) for our test cases.
FpuResult fadd_f32(const Bits& a, const Bits& b) {
    FpuResult out = make_zero_fpu_result();
    out.trace.push_back("fadd_f32 start");

    // Work with exactly 32 bits.
    Bits a32 = zero_extend(a, 32);
    Bits b32 = zero_extend(b, 32);

    F32Fields fa = unpack_f32(a32);
    F32Fields fb = unpack_f32(b32);

    // Quick zero checks: if one is exactly +0, return the other.
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

    // Build 24-bit significands with implicit leading 1 for normalized numbers.
    Bits sigA(24, 0);
    Bits sigB(24, 0);
    for (std::size_t i = 0; i < 23; ++i) {
        sigA[i] = fa.fraction[i];
        sigB[i] = fb.fraction[i];
    }
    sigA[23] = 1; // implicit leading 1
    sigB[23] = 1;

    // Choose the operand with larger exponent as "big".
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

    // Align smaller significand to the bigger exponent by shifting right.
    Bits exp_tmp = exp_big;      // copy to walk down to exp_small
    Bits sig_small_aligned = sig_small;

    Bits one_exp(8, 0);
    one_exp[0] = 1;

    while (compare_unsigned(exp_tmp, exp_small) > 0) {
        // shift significand right (logical) by 1
        shift_right_logical(sig_small_aligned, 24);

        // decrement exp_tmp by 1
        Bit borrow_e = 0;
        exp_tmp = subtract_unsigned(exp_tmp, one_exp, 8, borrow_e);
        if (borrow_e == 1) {
            // underflowing exponent; for our simple tests this shouldn't happen
            break;
        }
    }

    // Same-sign addition path
    if (sign_big == sign_small) {
        Bit carry = 0;
        Bits sig_sum = add_unsigned(sig_big, sig_small_aligned, 24, carry);

        Bits exp_res = exp_big;
        // If there was a carry out of the MSB of significand, normalize by shifting right and increment exponent.
        if (carry == 1) {
            shift_right_logical(sig_sum, 24);

            // increment exponent
            Bits one(8, 0);
            one[0] = 1;
            Bit carry_e = 0;
            exp_res = add_unsigned(exp_res, one, 8, carry_e);
            // We ignore exponent overflow here for our limited tests.
        }

        // Pack result: sign = sign_big, exponent = exp_res, fraction = low 23 bits of sig_sum (drop implicit 1).
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

    // Different signs: perform magnitude subtraction.
    Bits sig_big_local   = sig_big;
    Bits sig_small_local = sig_small_aligned;

    // Compare magnitudes after alignment.
    int mag_cmp = compare_unsigned(sig_big_local, sig_small_local);

    Bit result_sign = sign_big;
    if (mag_cmp < 0) {
        // |small| > |big|: swap roles so we always subtract smaller from bigger.
        Bits tmp = sig_big_local;
        sig_big_local = sig_small_local;
        sig_small_local = tmp;
        result_sign = sign_small;
    } else if (mag_cmp == 0) {
        // Magnitudes cancel exactly → result is zero.
        F32Fields fres{};
        fres.sign     = 0;           // +0
        fres.exponent = Bits(8, 0);  // all zero
        fres.fraction = Bits(23, 0); // all zero

        out.bits = pack_f32(fres);
        out.trace.push_back("fadd_f32 different-sign: exact zero");
        return out;
    }

    // Now we know |big| > |small|, so big - small is positive magnitude.
    Bit borrow = 0;
    Bits sig_diff = subtract_unsigned(sig_big_local, sig_small_local, 24, borrow);
    // Expect borrow == 0 here.

    // Normalize: ensure MSB (bit 23) is 1, unless result is zero.
    Bits exp_res = exp_big;

    if (bits_all_zero(sig_diff)) {
        // Guard, though mag_cmp == 0 should have handled this.
        F32Fields fres{};
        fres.sign     = 0;
        fres.exponent = Bits(8, 0);
        fres.fraction = Bits(23, 0);
        out.bits = pack_f32(fres);
        out.trace.push_back("fadd_f32 different-sign: diff zero");
        return out;
    }

    // Shift left until leading 1 is in bit 23, or exponent underflows to 0.
    while (sig_diff[23] == 0 && !bits_all_zero(sig_diff)) {
        shift_left_logical(sig_diff, 24);

        Bit borrow_e = 0;
        exp_res = subtract_unsigned(exp_res, one_exp, 8, borrow_e);
        if (borrow_e == 1) {
            // exponent underflowed; for now we stop (subnormal/underflow region).
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

FpuResult fsub_f32(const Bits& a, const Bits& b) {
    // a - b = a + (-b)  → flip sign bit of b and reuse fadd_f32
    Bits b32 = zero_extend(b, 32);
    Bits b_neg = b32;
    b_neg[31] = b32[31] ^ 1; // flip sign bit

    return fadd_f32(a, b_neg);
}

FpuResult fmul_f32(const Bits& a, const Bits& b) {
    FpuResult out = make_zero_fpu_result();
    out.trace.push_back("fmul_f32 start");

    // Work with exactly 32 bits for each operand.
    Bits a32 = zero_extend(a, 32);
    Bits b32 = zero_extend(b, 32);

    F32Fields fa = unpack_f32(a32);
    F32Fields fb = unpack_f32(b32);

    Bit sign_res = fa.sign ^ fb.sign;

    // Classify operands.
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

    // Canonical quiet NaN pattern.
    Bits nan_bits = bv_from_hex_string("0x7fc00000");

    // NaN propagation.
    if (a_is_nan || b_is_nan) {
        out.bits = nan_bits;
        out.flags.invalid = true;
        out.trace.push_back("fmul_f32: NaN operand");
        return out;
    }

    // 0 * inf → NaN, invalid.
    if ((a_is_inf && b_is_zero) || (b_is_inf && a_is_zero)) {
        out.bits = nan_bits;
        out.flags.invalid = true;
        out.trace.push_back("fmul_f32: 0 * inf invalid");
        return out;
    }

    // inf * finite non-zero → inf.
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

    // Zero * finite → zero (with sign = xor of signs).
    if (a_is_zero || b_is_zero) {
        F32Fields fres{};
        fres.sign = sign_res;
        fres.exponent = Bits(8, 0);
        fres.fraction = Bits(23, 0);
        out.bits = pack_f32(fres);
        out.trace.push_back("fmul_f32: zero result");
        return out;
    }

    // ---- Finite non-zero multiply path ----

    Bits expA = fa.exponent;
    Bits expB = fb.exponent;

    // Pre-check for exponent overflow using a 9-bit sum:
    // True overflow (to ±inf) occurs if biased exponents satisfy:
    //   expA + expB > 381  (since E_res = (expA-127) + (expB-127) > 127)
    //
    // We compute expA + expB in 9 bits and compare to 382.
    Bits expA9(9, 0);
    Bits expB9(9, 0);
    for (std::size_t i = 0; i < 8; ++i) {
        expA9[i] = expA[i];
        expB9[i] = expB[i];
    }

    Bit carry9 = 0;
    Bits exp_sum9 = add_unsigned(expA9, expB9, 9, carry9);

    // 382 decimal → binary 101111110 → bits LSB-first:
    // index:  8 7 6 5 4 3 2 1 0
    // bits:   1 0 1 1 1 1 1 1 0
    // so: [0]=0,[1]=1,[2]=1,[3]=1,[4]=1,[5]=1,[6]=1,[7]=0,[8]=1
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
        // Guaranteed exponent overflow → ±inf.
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


    // Exponent math: sum = expA + expB, then subtract bias (127).
    Bit carry_add = 0;
    Bits exp_sum = add_unsigned(expA, expB, 8, carry_add);

    Bits bias(8, 0);
    // 127 = 0b01111111 → bits 0..6 = 1, bit7 = 0
    for (std::size_t i = 0; i < 7; ++i) {
        bias[i] = 1;
    }
    bias[7] = 0;

    Bit borrow_bias = 0;
    Bits exp_tmp = subtract_unsigned(exp_sum, bias, 8, borrow_bias);

    // If exponent is already too small before normalization → underflow to zero.
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

    // Build 24-bit significands with implicit leading 1 for normals.
    Bits sigA(24, 0);
    Bits sigB(24, 0);
    for (std::size_t i = 0; i < 23; ++i) {
        sigA[i] = fa.fraction[i];
        sigB[i] = fb.fraction[i];
    }
    if (!expA_zero) sigA[23] = 1;
    if (!expB_zero) sigB[23] = 1;

    // Multiply significands: 24 x 24 → 48 bits via shift-add.
    Bits prod(48, 0);
    Bits multiplicand(48, 0);
    for (std::size_t i = 0; i < 24; ++i) {
        multiplicand[i] = sigA[i];
    }
    Bits multiplier = sigB; // 24 bits

    for (std::size_t step = 0; step < 24; ++step) {
        if (multiplier[0] == 1) {
            Bit carry_p = 0;
            Bits tmp = add_unsigned(prod, multiplicand, 48, carry_p);
            prod = tmp;
            // We ignore carry_p beyond bit 47 for our limited range.
        }
        // Shift multiplier right by 1 bit.
        shift_right_logical(multiplier, 24);
        // Shift multiplicand left by 1 bit.
        shift_left_logical(multiplicand, 48);
    }

    out.trace.push_back("fmul_f32: after significand multiply");

    // Normalize: significand product is in [2^46, 2^48), so top bit is at 46 or 47.
    bool high = (prod[47] == 1);
    Bits exp_res = exp_tmp;

    if (high) {
        // Need to divide by 2 once more → increment exponent.
        Bits one(8, 0);
        one[0] = 1;
        Bit carry_e = 0;
        exp_res = add_unsigned(exp_res, one, 8, carry_e);
        if (carry_e == 1) {
            // Exponent overflowed beyond representable range → +∞/-∞.
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

    // Extract 24-bit significand from 48-bit product.
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
        // Underflow into subnormal/zero. For now, flush to zero and set underflow.
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
        // Exponent saturated to all ones → overflow to infinity.
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

    // Normal finite result.
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

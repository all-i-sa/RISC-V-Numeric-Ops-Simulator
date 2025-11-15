#include "core/twos.hpp"
#include "core/bitvec.hpp"
#include <cassert>


namespace rv::core {

        /***** make_fixed_i32_from_sign_and_magnitude *****
        *   Builds a 32-bit two's-complement value from a sign bit and magnitude bits
        *   - If sign == 0, the value is > 0 so just use magnitude
        *   - If sign == 1, the value is < 0 so use 2's comp
        ******************************
        * Inputs:
        *   sign      - 0
        *   magnitude - abs value
        * Returns:
        *   Bits - 32-bit 2's comp
        ******************************/
    Bits make_fixed_i32_from_sign_and_magnitude(Bit sign, Bits magnitude) {
        // extend magnitude to 32 bits with zeros.
        Bits mag = zero_extend(magnitude, 32);
        if (sign == 0) {
            return mag;
        } else {
            // For negative: two's-negate the positive magnitude (width 32)
            return twos_negate(mag);
        }
    }

        /***** encode_i32_TEMP_host *****
        *   Encodes a signed 32-bit integer into 32 bits of 2's comp
        *   - Temp helper that uses basic shifts
        *   - LSB
        ******************************
        * Inputs:
        *   v - signed 32-bit value
        * Returns:
        *   Bits - bit vector of length 32 with v in 2's comp
        ******************************/
    Bits encode_i32_TEMP_host(int32_t v) {
        Bits b;
        b.resize(32, 0);
        uint32_t u = static_cast<uint32_t>(v); // two's comp
        for (int i = 0; i < 32; ++i) {
            b[i] = (u >> i) & 0x1;
        }
        return b;
    }

        /***** decode_i32_to_host *****
        *   Decodes a 2's comp bit vector into a signed int
        *   - Sign-extends or trims to exactly 32 bits.
        ******************************
        * Inputs:
        *   b - bit vector
        * Returns:
        *   int64_t - decoded signed value
        ******************************/
    int64_t decode_i32_to_host(const Bits& b) {
        if (b.empty()) return 0;
        Bits w = b;
        if (w.size() < 32) w = sign_extend(w, 32);
        else if (w.size() > 32) w = bv_slice(w, 31, 0);

        Bit sign = w[31];

        auto sum_bits = [](const Bits& x)->int64_t {
            int64_t acc = 0;
            for (std::size_t i = 0; i < x.size(); ++i) {
                if (x[i]) acc += (int64_t(1) << i);
            }
            return acc;
        };

        if (sign == 0) {
            return sum_bits(w);
        } else {
            Bits mag = twos_negate(w); // now magnitude in positive form
            int64_t m = sum_bits(mag);
            return -m;
        }
    }

/* -------------------- helpers to make sure we are working with 32 bits -------------------- */


    /***** ensure_i32_width *****
     *   Makes sure a bit vector is treated as a 32-bit signed value.
     ******************************
     * Inputs:
     *   in - original bit vector.
     * Returns:
     *   Bits - bit vector that is exactly 32 bits.
     ******************************/
    static Bits ensure_i32_width(const Bits& in) {
        if (in.empty()) return zero_extend(Bits{0}, 32);
        if (in.size() < 32) return sign_extend(in, 32);
        if (in.size() > 32) return bv_slice(in, 31, 0);
        return in;
    }

    /***** decode_i32_to_sign_and_magnitude *****
    *   Takes a 32-bit 2's comp bit vector and breaks it into
    *   a sign bit and magnitude bits
    ******************************
    * Inputs:
    *   b32 - 32-bit vector in 2's comp
    * Returns:
    *   SignMag32 - struct with sign bit and magnitude bits
    ******************************/
    SignMag32 decode_i32_to_sign_and_magnitude(const Bits& b32_in) {
        Bits w = ensure_i32_width(b32_in);
        Bit sign = w[31];

        Bits mag;
        if (sign == 0) {
            mag = trim_leading(w);
        } else {
            Bits pos = twos_negate(w);
            mag = trim_leading(pos);
        }
        if (mag.empty()) mag = Bits{0};
        return SignMag32{sign, mag};
    }

    /***** encode_i32_from_sign_and_magnitude *****
     *   Builds a 32-bit 2's comp bit vector from a sign bit and mag
     ******************************
     * Inputs:
     *   sign
     *   magnitude
     * Returns:
     *   Bits
     ******************************/
    Bits encode_i32_from_sign_and_magnitude(Bit sign, const Bits& magnitude) {
        Bits mag32 = zero_extend(magnitude, 32);
        if (sign == 0) {
            return mag32;
        } else {
            return twos_negate(mag32);
        }
    }

    EncodeI32Result encode_twos_i32(int64_t value) {
    EncodeI32Result res{};

    // Signed 32-bit bounds
    const int64_t MIN_I32 = -2147483648LL; // -2^31
    const int64_t MAX_I32 =  2147483647LL; //  2^31 - 1

    // Overflow flag: true if value is outside signed 32-bit range
    res.overflow = (value < MIN_I32) || (value > MAX_I32);

    int32_t narrowed = static_cast<int32_t>(value);

    // Build a 32-bit 2's comp bit vector
    res.bits = encode_i32_TEMP_host(narrowed);

    // Pretty hex str
    res.hex = bv_to_hex_string(res.bits);

    return res;
}

    int64_t decode_twos_i32(const Bits& b32) {
    return decode_i32_to_host(b32);
}

} // namespace rv::core


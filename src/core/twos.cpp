#include "core/twos.hpp"

namespace rv::core {

Bits make_fixed_i32_from_sign_and_magnitude(Bit sign, Bits magnitude) {
    // Clamp/extend magnitude to 32 bits with zeros.
    Bits mag = zero_extend(magnitude, 32);
    if (sign == 0) {
        return mag;
    } else {
        // For negative: two's-negate the positive magnitude (width 32)
        return twos_negate(mag);
    }
}

// TEMP: Uses host ints to build the 32-bit pattern.
// Allowed for today; we’ll replace with pure construction once the adder exists.
Bits encode_i32_TEMP_host(int32_t v) {
    Bits b;
    b.resize(32, 0);
    uint32_t u = static_cast<uint32_t>(v); // two's rep
    for (int i = 0; i < 32; ++i) {
        b[i] = (u >> i) & 0x1;
    }
    return b;
}

// Decode signed 32-bit Bits (two's complement) to host int64_t (for tests only).
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
        Bits mag = twos_negate(w); // now magnitude in positive
        int64_t m = sum_bits(mag);
        return -m;
    }
}

/* -------------------- PURE bit-level helpers (width-fixed to 32) -------------------- */

// Keep this file-local helper near the pure functions
static Bits ensure_i32_width(const Bits& in) {
    if (in.empty()) return zero_extend(Bits{0}, 32);
    if (in.size() < 32) return sign_extend(in, 32);
    if (in.size() > 32) return bv_slice(in, 31, 0);
    return in;
}

SignMag32 decode_i32_to_sign_and_magnitude(const Bits& b32_in) {
    Bits w = ensure_i32_width(b32_in);
    Bit sign = w[31];

    Bits mag;
    if (sign == 0) {
        // Non-negative: magnitude is the value itself (unsigned)
        mag = trim_leading(w);
    } else {
        // Negative: two's-negate to get magnitude
        Bits pos = twos_negate(w);   // width-preserving (32)
        mag = trim_leading(pos);
    }
    if (mag.empty()) mag = Bits{0};
    return SignMag32{sign, mag};
}

Bits encode_i32_from_sign_and_magnitude(Bit sign, const Bits& magnitude) {
    Bits mag32 = zero_extend(magnitude, 32);  // clamp/extend to 32 bits
    if (sign == 0) {
        return mag32;
    } else {
        return twos_negate(mag32);            // width-preserving two's-negate
    }
}

} // namespace rv::core

#include "core/bitvec.hpp"
#include <cctype>

namespace rv::core {

// Hex helper: Converts a single hex char.

static uint8_t hex_nibble_from_char(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    throw std::invalid_argument("Invalid hex digit");
}

static char char_from_nibble(uint8_t v) {
    static const char* lut = "0123456789abcdef";
    if (v > 15) throw std::logic_error("nibble out of range");
    return lut[v];
}

/* Trims leading zeros (MSB side)
 * LSB is at index 0 so MSB is at back of vector
 */
Bits trim_leading(const Bits& b_in) {
    if (b_in.empty()) return Bits{0};
    // drop MSB zeros (remember LSB-first)
    std::size_t i = b_in.size();
    while (i > 1 && b_in[i-1] == 0) { --i; }
    Bits out(b_in.begin(), b_in.begin() + i);
    return out;
}
/* Hex to Bits
 * No native integers
 * Builds LSB-first (right to left)
 * */
Bits bv_from_hex_string(std::string hex) {
    // strip 0x
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0) {
        hex = hex.substr(2);
    }
    // allow underscores in literals (e.g., 0x7f_ff)
    hex.erase(std::remove(hex.begin(), hex.end(), '_'), hex.end());

    if (hex.empty()) return Bits{0};

    Bits out;
    out.reserve(hex.size() * 4);

    // Build LSB-first: last hex char contributes highest nibble
    for (auto it = hex.rbegin(); it != hex.rend(); ++it) {
        uint8_t nib = hex_nibble_from_char(*it);
        for (int i = 0; i < 4; ++i) {
            out.push_back( (nib >> i) & 0x1 );
        }
    }
    return trim_leading(out);
}

/* Bits to Hex (human readable)
 * Multiples of 4 bits
 * Outputs digits in LSB to MSB order then reverses string so MSB is first
 * */
std::string bv_to_hex_string(const Bits& b_in, bool prefix0x) {
    Bits b = b_in;
    if (b.empty()) b = Bits{0};

    // pad up to nibble multiple
    std::size_t rem = b.size() % 4;
    if (rem != 0) {
        b.insert(b.end(), 4 - rem, 0);
    }
    // from LSB nibble upward (storage is LSB-first)
    std::string s;
    for (std::size_t i = 0; i < b.size(); i += 4) {
        uint8_t nib = (b[i] & 1)
                    | ((b[i+1] & 1) << 1)
                    | ((b[i+2] & 1) << 2)
                    | ((b[i+3] & 1) << 3);
        s.push_back(char_from_nibble(nib));
    }
    // reverse to MSB-first for human-readable hex
    std::reverse(s.begin(), s.end());

    // trim leading zeros (but keep at least one)
    std::size_t nz = 0;
    while (nz + 1 < s.size() && s[nz] == '0') ++nz;
    if (nz) s.erase(0, nz);

    return prefix0x ? std::string("0x") + s : s;
}
/* Pad LHS (MSB) and slice
 * combines extend or truncate
 * */
Bits bv_pad_left(const Bits& b, std::size_t width, Bit fill) {
    if (b.size() >= width) return Bits(b.begin(), b.begin() + width);
    Bits out = b;
    out.reserve(width);
    out.insert(out.end(), width - b.size(), fill);
    return out;
}

Bits bv_slice(const Bits& b, std::size_t hi_inclusive, std::size_t lo_inclusive) {
    if (lo_inclusive > hi_inclusive) throw std::invalid_argument("slice: lo>hi");
    if (hi_inclusive >= b.size()) throw std::out_of_range("slice: hi out of range");
    return Bits(b.begin() + lo_inclusive, b.begin() + hi_inclusive + 1);
}
/* Pretty binary printing
 * MSB to LSB
* */
std::string bv_to_pretty_bin(const Bits& b_in, std::size_t group, char sep) {
    Bits b = b_in;
    if (b.empty()) b = Bits{0};
    // Print MSB->LSB
    std::string s;
    s.reserve(b.size() + (group ? b.size() / group : 0));
    std::size_t cnt = 0;
    for (std::size_t i = b.size(); i-- > 0; ) {
        s.push_back(b[i] ? '1' : '0');
        ++cnt;
        if (group && i != 0 && (cnt % group == 0)) s.push_back(sep);
    }
    return s;
}
/* Extensions (unsigned and signed widening)
 * zero extend to pad MSB side with 0's to extend to width or truncates if already wider
 * sign extend copies the current MSB (sign bit) when padding
 * */
Bits zero_extend(const Bits& b, std::size_t width) {
    return bv_pad_left(b, width, 0);
}

Bits sign_extend(const Bits& b, std::size_t width) {
    Bit sign = b.empty() ? 0 : b.back();
    return bv_pad_left(b, width, sign);
}

/*  Two's complement negate
 * implements -x as bitwise invert + 1, propagating a ripple to carry LSB -> MSB
 * Final carry is scarded
 * */
Bits twos_negate(Bits b) {
    if (b.empty()) return Bits{0};
    // invert
    for (auto& bit : b) bit ^= 1;

    // add one (ripple)
    Bit carry = 1;
    for (std::size_t i = 0; i < b.size(); ++i) {
        Bit sum = (b[i] ^ carry);
        carry = (b[i] & carry);
        b[i] = sum;
        if (!carry) break;
    }
    // overflow carry beyond MSB is dropped (fixed width)
    return b;
}

} // namespace rv::core

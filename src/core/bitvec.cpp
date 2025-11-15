#include "core/bitvec.hpp"
#include <cctype>

namespace rv::core {

/***** hex_nibble_from_char *****
 *   Converts a single hex character into 4-bit value
 *   - Throws std::invalid_argument if the character is not a valid hex digit
 ******************************
 * Inputs:
 *   c - a single character to convert
 * Returns:
 *   uint8_t - the numeric value of the hex digit
 ******************************/

static uint8_t hex_nibble_from_char(char c) {
    if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(10 + (c - 'a'));
    throw std::invalid_argument("Invalid hex digit");
}
/***** char_from_nibble *****
 *   Converts a 4-bit value into a hex character
 *   - Uses lowercase hex digits
 *   - Throws std::logic_error if the value is larger than 15
 ******************************
 * Inputs:
 *   v - numeric value 0-15
 * Returns:
 *   char - matching hex character
 ******************************/
static char char_from_nibble(uint8_t v) {
    static const char* lut = "0123456789abcdef";
    if (v > 15) throw std::logic_error("nibble out of range");
    return lut[v];
}

/***** trim_leading *****
 *   Removes leading zeros from the MSB side of a bit vector.
 *   - Bits are stored LSB-first, so this looks at the back of the vector.
 *   - Keeps at least one bit so zero is represented as {0}, not an empty vector.
 ******************************
 * Inputs:
 *   b_in - original bit vector
 * Returns:
 *   Bits - trimmed bit vector with no extra zeros on the MSB side
 ******************************/
Bits trim_leading(const Bits& b_in) {
    if (b_in.empty()) return Bits{0};
    // drop MSB zeros (remember LSB-first)
    std::size_t i = b_in.size();
    while (i > 1 && b_in[i-1] == 0) { --i; }
    Bits out(b_in.begin(), b_in.begin() + i);
    return out;
}

/***** bv_from_hex_string *****
 *   Converts a hex string into a bit vector
 *   - Builds the result LSB first by walking the string from right to left
 *   - Trims leading zeros on the MSB side but keeps at least one bit
 *****************************
 * Inputs:
 *   hex - string
 * Returns:
 *   Bits - bit vector representing the hex value LSB-first.
 ******************************/
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

/***** bv_to_hex_string *****
 *   Converts a bit vector into a hex string
 *   - Pads with zeros so the length is a multiple of 4
 *   - Builds hex digits nibble-by-nibble from LSB to MSB then reverses.
 *   - Trims leading zero hex digits but keeps at least one
 *****************************
 * Inputs:
 *   b_in     - bit vector to convert
 *   prefix0x - if true add "0x" at the front
 * Returns:
 *   std::string - hex string
 ******************************/
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
/***** bv_pad_left *****
 *   Make sure a bit vector has the right width by padding on the MSB side
 *   - If the vector is already at least width bits, it is truncated
 *     to that width
 *   - If it is shorter, it is padded on the MSB side with the given fill bit
 *****************************
 * Inputs:
 *   b      - original bit vector
 *   width  -  width
 *   fill   - bit value used for padding
 * Returns:
 *   Bits - vector exactly width bits wide
 ******************************/
Bits bv_pad_left(const Bits& b, std::size_t width, Bit fill) {
    if (b.size() >= width) return Bits(b.begin(), b.begin() + width);
    Bits out = b;
    out.reserve(width);
    out.insert(out.end(), width - b.size(), fill);
    return out;
}
/***** bv_slice *****
 *   Returns a range of bits from a bit vector (LSB-first)
 *   - Uses LSB-first indexing: bit 0 is the LSB
 *   - Takes high index and low index
 *   - The slice is taken from lo_inclusive up to hi_inclusive
 ******************************
 * Inputs:
 *   b             - original bit vector
 *   hi_inclusive  - highest bit index to include
 *   lo_inclusive  - lowest bit index to include
 * Returns:
 *   Bits - the requested bit range as a new bit vector
 ******************************/
Bits bv_slice(const Bits& b, std::size_t hi_inclusive, std::size_t lo_inclusive) {
    if (lo_inclusive > hi_inclusive) throw std::invalid_argument("slice: lo>hi");
    if (hi_inclusive >= b.size()) throw std::out_of_range("slice: hi out of range");
    return Bits(b.begin() + lo_inclusive, b.begin() + hi_inclusive + 1);
}
/***** bv_to_pretty_bin *****
 *   Turns a bit vector into a human-readable binary string
 *   - Can group bits into chunks
 *   - Can insert a separator character between groups
 *   - Still uses LSB-first internally, but prints in the normal MSB-first order
 ******************************
 * Inputs:
 *   b      - bit vector to format
 *   group  - group size (0 means no grouping)
 *   sep    - separator character between groups
 * Returns:
 *   std::string - formatted binary string
 ******************************/
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
/***** zero_extend *****
 *   Extends a bit vector to a given width by adding zeros
 *   on the MSB side.
 ******************************
 * Inputs:
 *   b      - original bit vector
 *   width  - needed width
 * Returns:
 *   Bits - bit vector of length width
 ******************************/
Bits zero_extend(const Bits& b, std::size_t width) {
    return bv_pad_left(b, width, 0);
}
/***** sign_extend *****
 *   Extends a bit vector to a given width by copying the sign bit
 *   - If the top bit is 0, pads with 0's
 *   - If the top bit is 1, pads with 1's
 ******************************
 * Inputs:
 *   b      - original bit vector
 *   width  - need width
 * Returns:
 *   Bits - bit vector of length width
 ******************************/
Bits sign_extend(const Bits& b, std::size_t width) {
    Bit sign = b.empty() ? 0 : b.back();
    return bv_pad_left(b, width, sign);
}

/***** twos_negate *****
 *   Computes the two's-complement of a bit vector
 *   - Inverts all bits.
 *   - Adds 1 using a ripple-carry adder
 *   - Any carry after the MSB is dropped
 ******************************
 * Inputs:
 *   b - bit vector to negate
 * Returns:
 *   Bits - new bit vector with negated value
 ******************************/
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

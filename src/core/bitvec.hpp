#pragma once        // include "once" guard to prevent double definitions
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace rv::core {

    using Bit  = uint8_t;              // 0 or 1
    using Bits = std::vector<Bit>;     // LSB is index 0, so bits[0] is 2^0

    /***** bv_from_hex_string *****
     * Builds a bit vector from a hex string.
     *   - The string can start with "0x" or not.
     *   - Each hex digit is turned into 4 bits.
     *   - The result is LSB-first (index 0 is the least significant bit).
     ******************************
     * Inputs:
     *   hex  - hex string
     * Returns:
     *   Bits - a bit vector holding the value of the hex string.
     ******************************/
    Bits bv_from_hex_string(std::string hex);

    /***** bv_to_hex_string *****
     *   Turns a bit vector into a hex string.
     *   - Works with LSB-first bit vectors
     ******************************
     * Inputs:
     *   b        - bit vector to convert
     *   prefix0x - if true, prefix the string with 0x
     * Returns:
     *   std::string - a hex string
     ******************************/
    std::string bv_to_hex_string(const Bits& b, bool prefix0x = true);

    /***** bv_pad_left *****
     *   Makes sure a bit vector has the correct width. It will pad
     *   on the MSB side (left)
     *   - If the vector is already wide enough, it will be returned as is
     *   - If it is too short, new bits are added to fill it
     ******************************
     * Inputs:
     *   b      - original bit vector
     *   width  - minimum number of bits needed
     *   fill   - bit value (0 or 1 depenging on pos or neg) to pad with on the MSB side
     * Returns:
     *   Bits - a bit vector that with the correct width
     ******************************/
    Bits bv_pad_left(const Bits& b, std::size_t width, Bit fill);

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
    Bits bv_slice(const Bits& b, std::size_t hi_inclusive, std::size_t lo_inclusive);

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
    std::string bv_to_pretty_bin(const Bits& b, std::size_t group = 0, char sep = '_');

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
    Bits zero_extend(const Bits& b, std::size_t width);

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
    Bits sign_extend(const Bits& b, std::size_t width);

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
    Bits twos_negate(Bits b);

    /***** bit_width *****
     *   Returns how many bits are in the vector
     ******************************
     * Inputs:
     *   b - bit vector
     * Returns:
     *   std::size_t - number of bits
     ******************************/
    inline std::size_t bit_width(const Bits& b) { return b.size(); }

    /***** trim_leading *****
     *   Removes leading zeros on the MSB side
     *   - Keeps at least 1 bit
     *   - So the value zero is always represented as a single 0 bit
     *     not an empty vector.
     ******************************
     * Inputs:
     *   b - original bit vector
     * Returns:
     *   Bits - trimmed bit vector
     ******************************/
    Bits trim_leading(const Bits& b);

} // namespace rv::core

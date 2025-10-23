#pragma once        // include "once" guard to prevent double definitations
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>

namespace rv::core {

    using Bit  = uint8_t;              // 0 or 1
    using Bits = std::vector<Bit>;     // LSB is index 0, thus bit vector bits[0] is 2^0

    /* ---- CONSTRUCTION & CONVERSION  ----
    * Construct bit vector from hex string and convert a bit vector back
    * to hex string. No converting native integers. Parse each digit.
    */
    Bits bv_from_hex_string(std::string hex);        // accepts "0x" or not
    std::string bv_to_hex_string(const Bits& b, bool prefix0x = true);

    // ---- SHAPE & SLICE ----
    Bits bv_pad_left(const Bits& b, std::size_t width, Bit fill); // ensure width criteria is met by width
                                                                  // extending
    Bits bv_slice(const Bits& b, std::size_t hi_inclusive, std::size_t lo_inclusive); // LSB-first indexing and
                                                                                      // contiguous bit range

    /* ---- PRETTY PRINTING ----
     * Turn the bit vector into human friendly binary string nibbly
     */
    std::string bv_to_pretty_bin(const Bits& b, std::size_t group = 4, char sep = '_');

    // ---- EXTENSIONS ----
    Bits zero_extend(const Bits& b, std::size_t width); // grow to width by padding MSB side with zeros
    Bits sign_extend(const Bits& b, std::size_t width); // grow to edith by padding MSB side with copies
                                                        // of the current MSB (sign bit)

    /* ---- COMPUTE TWO'S COMPLEMENT ----
     * compute 2's comp using the current width of vector b
     */
    Bits twos_negate(Bits b); // invert + add one (ripple), drop carry beyond MSB

    // ---- LIL' HELPERS ----
    inline std::size_t bit_width(const Bits& b) { return b.size(); } // current bit count
    Bits trim_leading(const Bits& b); // drop MSB zeros, keep at least 1 bit so zero isn't represented by empty
                                      // vector

} // namespace rv::core

#pragma once
#include "core/bitvec.hpp"
#include <cstdint>
#include <string>

namespace rv::core {

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
    Bits encode_i32_TEMP_host(int32_t v);

     /***** decode_i32_to_host *****
    *   Decodes a 2's comp bit vector into a signed int
    *   - Sign-extends or trims to exactly 32 bits.
    ******************************
    * Inputs:
    *   b - bit vector
    * Returns:
    *   int64_t - decoded signed value
    ******************************/
    int64_t decode_i32_to_host(const Bits& b); // interpret Bits as signed 32-bit

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
    Bits make_fixed_i32_from_sign_and_magnitude(Bit sign, Bits magnitude);

     /***** SignMag32 *****
    *   Struct holding a sign bit and mag bits
    *    sign - 0 or 1
    *    mag  - abs value
    ******************************/
    struct SignMag32 {
     Bit  sign;  // 0 or 1
     Bits mag;
    };

     /***** decode_i32_to_sign_and_magnitude *****
    *   Takes a 32-bit 2's comp bit vector and breaks it into
    *   a sign bit and magnitude bits
    ******************************
    * Inputs:
    *   b32 - 32-bit vector in 2's comp
    * Returns:
    *   SignMag32 - struct with sign bit and magnitude bits
    ******************************/
    SignMag32 decode_i32_to_sign_and_magnitude(const Bits& b32);

    /***** encode_i32_from_sign_and_magnitude *****
     *   Builds a 32-bit 2's comp bit vector from a sign bit and mag
     ******************************
     * Inputs:
     *   sign
     *   magnitude
     * Returns:
     *   Bits
     ******************************/
     Bits encode_i32_from_sign_and_magnitude(Bit sign, const Bits& magnitude);

 // -------------------------------------------------------------
 // Main functions I use to turn numbers into 32-bit bits and back
 // -------------------------------------------------------------

     /***** EncodeI32Result *****
     *   The result of encoding a signed integer into 32-bit two's-complement form
     *
     *   bits     - 32-bit 2's comp
     *   hex      - pretty hex string
     *   overflow - true if the og value did not fit in 32-bit signed range
     ******************************/
     struct EncodeI32Result {
      Bits        bits;      // 32-bit 2's comop
      std::string hex;       // Pretty hex string
      bool        overflow;
     };

     /***** encode_twos_i32 *****
      *   Encodes a signed int into 32 bits of 2's comp and reports
      *   if the value overflows the 32-bit signed range
      ******************************
      * Inputs:
      *   value - signed integer to encode.
      * Returns:
      *   EncodeI32Result - bits, hex string, and overflow flag
      ******************************/
     EncodeI32Result encode_twos_i32(int64_t value);

     /***** decode_twos_i32 *****
      *   Decodes a 32-bit two's-complement bit vector into a signed int
      ******************************
      * Inputs:
      *   b32 - 32-bit vector in 2's comp
      * Returns:
      *   int64_t - decoded signed value
      ******************************/
     int64_t decode_twos_i32(const Bits& b32);

} // namespace rv::core

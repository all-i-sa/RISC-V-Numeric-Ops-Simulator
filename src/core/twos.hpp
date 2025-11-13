#pragma once
#include "core/bitvec.hpp"
#include <cstdint>
#include <string>

namespace rv::core {

    /**Encode a signed 32-bit host integer into a 32-bit two's-complement bit vector.
     * TEMPORARY helper for tests/CLI: this uses host bit shifts to build the representation.
     * Resulting bit vector is fixed width 32 and stored LSB-first (bits[0] = 2^0).
     * @param v  Signed 32-bit integer value on the host (int32_t).
     * @return   Bits of length 32 holding v in two's-complement form, LSB-first.
     *
     * @note This is a convenience shim so we can write tests immediately. The plan is to
     *       replace it with a “pure bit-ops” constructor once the ripple-carry adder exists.
     * @example
     *   auto b = encode_i32_TEMP_host(-13);
     *   // b.size() == 32, b encodes 0xFFFF'FFF3 (LSB-first).
     */

    Bits encode_i32_TEMP_host(int32_t v);   // 32-bit two's complement, LSB-first

    /**
     * @brief Decode a two's-complement bit vector (interpreted as signed 32-bit) to a host integer.
     *
     * Accepts any width; internally it sign-extends or truncates to exactly 32 bits, then interprets
     * as two's-complement and returns the corresponding host integer.
     * Input bits are assumed LSB-first (bits[0] = 2^0).
     *
     * @param b  Bit vector (LSB-first). If size < 32, it is sign-extended; if size > 32, truncated to [31:0].
     * @return   Signed 64-bit host integer with the decoded numeric value. (int64_t to avoid UB on INT_MIN.)
     *
     * @example
     *   int64_t x = decode_i32_to_host(encode_i32_TEMP_host(-7)); // x == -7
     */
    int64_t decode_i32_to_host(const Bits& b); // interpret Bits as signed 32-bit

    /**
     * @brief Build a fixed-width (32-bit) two's-complement representation from sign and magnitude bits.
     *
     * Produces a 32-bit, LSB-first bit vector. The magnitude is treated as an *unsigned* positive
     * quantity and zero-extended to 32 bits. If sign == 0, the result is just that magnitude.
     * If sign == 1, the result is the two's-complement negation of that magnitude (width-preserving).
     *
     * @param sign       0 for non-negative, 1 for negative.
     * @param magnitude  LSB-first magnitude bits (unsigned). Will be zero-extended/clamped to 32 bits.
     * @return           32-bit two's-complement value as Bits (LSB-first).
     *
     * @pre  Magnitude should represent |value| (no sign embedded).
     * @post Result always has size() == 32.
     *
     * @example
     *   // +13
     *   Bits mag = bv_from_hex_string("0xd");      // 1101 (LSB-first internally)
     *   auto pos = make_fixed_i32_from_sign_and_magnitude(0, mag); // encodes +13
     *
     *   // -13
     *   auto neg = make_fixed_i32_from_sign_and_magnitude(1, mag); // encodes -13
     */
    Bits make_fixed_i32_from_sign_and_magnitude(Bit sign, Bits magnitude);

 // --- Pure two's-complement helpers (bit-level only; width-fixed to 32) ---
 struct SignMag32 {
  Bit  sign;  // 0 = non-negative, 1 = negative
  Bits mag;   // LSB-first magnitude bits (no sign), trimmed but at least 1 bit
 };

 /**
  * @brief Decode a 32-bit two's-complement vector into (sign, magnitude) bits.
  *        Pure bit-ops: if negative, two's-negate to get magnitude.
  *        Returns trimmed magnitude (keeps at least 1 bit).
  */
 SignMag32 decode_i32_to_sign_and_magnitude(const Bits& b32);

 /**
  * @brief Encode (sign, magnitude) into a 32-bit two's-complement vector.
  *        Pure bit-ops: zero-extend mag to 32, two's-negate if sign==1.
  *        Equivalent to make_fixed_i32_from_sign_and_magnitude.
  */
 Bits encode_i32_from_sign_and_magnitude(Bit sign, const Bits& magnitude);

 // -------------------------------------------------------------
 // Official project-facing encode/decode API (32-bit two's complement)
 // -------------------------------------------------------------

 struct EncodeI32Result {
  Bits        bits;      // 32-bit two's-complement representation (LSB-first)
  std::string hex;       // Pretty hex string, e.g., "0x0000000D"
  bool        overflow;  // 1 if value outside [-2^31, 2^31-1]
 };

 /**
  * @brief Encode a signed integer into 32-bit two's-complement, with overflow flag.
  *
  * The input is treated as a mathematical integer. If it is outside the signed
  * 32-bit range [-2^31, 2^31-1], overflow is set to true. In all cases, a 32-bit
  * two's-complement bit pattern is produced.
  */
 EncodeI32Result encode_twos_i32(int64_t value);

 /**
  * @brief Decode a 32-bit two's-complement bit vector into a signed integer.
  *
  * Expects exactly 32 bits, LSB-first. Interprets them as a signed two's-complement
  * integer and returns a host int64_t (to safely hold INT_MIN).
  */
 int64_t decode_twos_i32(const Bits& b32);



} // namespace rv::core

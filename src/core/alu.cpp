#include "core/alu.hpp"
#include <cassert>

namespace rv::core {

    namespace {

        /***** Add32Result *****
         *   Helper struct for a 32-bit add
         *   sum
         *   carry_out
         **************************/
        struct Add32Result {
            Bits sum;
            Bit  carry_out;
        };

        /***** add_32 *****
         *   Adds two 32-bit values
         ************************
         * Inputs:
         *   a, b - 32-bit bit vectors
         * Output:
         *   Add32Result
         **************************/
        Add32Result add_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            Bits sum(32, 0);
            Bit carry = 0;

            for (std::size_t i = 0; i < 32; ++i) {
                Bit ai = a[i];
                Bit bi = b[i];

                Bit partial    = ai ^ bi;
                Bit s          = partial ^ carry;
                Bit carry_next = (ai & bi) | (ai & carry) | (bi & carry);

                sum[i] = s;
                carry  = carry_next;
            }

            Add32Result res{ sum, carry };
            return res;
        }

        /***** twos_negate_32 *****
         *   Computes -v in two’s-comp
         *   - Flip all bits
         *   - Add 1 using adder
         **************************/
        Bits twos_negate_32(const Bits& v) {
            assert(v.size() == 32);

            Bits inv(32, 0);
            for (std::size_t i = 0; i < 32; ++i) {
                inv[i] = v[i] ^ 1; // flip bit: 0→1, 1→0
            }

            Bits one(32, 0);
            one[0] = 1; // LSB = 1

            Add32Result add_res = add_32(inv, one);
            return add_res.sum; // stays 32 bits wide
        }

        /***** compute_zero_flag *****
         *   Checks if all bits in r are zero
         ************************
         * Output:
         *   - Returns 1 if r is exactly zero
         *   - or returns 0
         **************************/
        Bit compute_zero_flag(const Bits& r) {
            for (Bit bit : r) {
                if (bit != 0) {
                    return 0; // not zero
                }
            }
            return 1; // all bits zero
        }

    } // anonymous namespace

    /***** alu_execute ****
     *   Runs one ALU operation on two inputs.
     ************************
     * Inputs:
     *   a  - first operand
     *   b  - second operand
     *   op - which operation to do
     * Output:
     *   AluResult
     **************************/
    AluResult alu_execute(const Bits& a, const Bits& b, AluOp op) {
        Bits a32 = zero_extend(a, 32);
        Bits b32 = zero_extend(b, 32);

        Bits result(32, 0);
        AluFlags flags{0, 0, 0, 0};

        switch (op) {
            case AluOp::Add: {
                Add32Result add_res = add_32(a32, b32);
                result = add_res.sum;

                Bit sign_a = a32[31];
                Bit sign_b = b32[31];
                Bit sign_r = result[31];

                flags.N = sign_r;
                flags.Z = compute_zero_flag(result);
                flags.C = add_res.carry_out;
                flags.V = ((sign_a == sign_b) && (sign_r != sign_a)) ? 1 : 0;
                break;
            }

            case AluOp::Sub: {
                Bits neg_b = twos_negate_32(b32);
                Add32Result add_res = add_32(a32, neg_b);
                result = add_res.sum;

                Bit sign_a = a32[31];
                Bit sign_b = b32[31];
                Bit sign_r = result[31];

                flags.N = sign_r;
                flags.Z = compute_zero_flag(result);
                flags.C = add_res.carry_out;
                flags.V = ((sign_a != sign_b) && (sign_r != sign_a)) ? 1 : 0;
                break;
            }

            default:
                result = a32;
                flags.N = result[31];
                flags.Z = compute_zero_flag(result);
                flags.C = 0;
                flags.V = 0;
                break;
        }

        AluResult res{ result, flags };
        return res;
    }
} // namespace rv::core

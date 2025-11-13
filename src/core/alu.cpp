//
// Created by Allisa Warren on 11/12/25.
//
#include "core/alu.hpp"
#include <cassert>

namespace rv::core {

    namespace {
        struct Add32Result {
            Bits sum;
            Bit  carry_out;
        };

        // 32-bit ripple-carry adder: a + b (no initial carry-in).
        Add32Result add_32(const Bits& a, const Bits& b) {
            assert(a.size() == 32);
            assert(b.size() == 32);

            Bits sum(32, 0);
            Bit carry = 0;

            for (std::size_t i = 0; i < 32; ++i) {
                Bit ai = a[i];
                Bit bi = b[i];

                // Full adder:
                Bit partial   = ai ^ bi;
                Bit s         = partial ^ carry;
                Bit carry_next = (ai & bi) | (ai & carry) | (bi & carry);

                sum[i] = s;
                carry  = carry_next;
            }

            Add32Result res{ sum, carry };
            return res;
        }

        // Two's-complement negate for a 32-bit vector: ~v + 1
        Bits twos_negate_32(const Bits& v) {
            assert(v.size() == 32);

            Bits inv(32, 0);
            for (std::size_t i = 0; i < 32; ++i) {
                inv[i] = v[i] ^ 1; // flip bit: 0->1, 1->0
            }

            Bits one(32, 0);
            one[0] = 1; // LSB = 1

            Add32Result add_res = add_32(inv, one);
            return add_res.sum; // width stays 32
        }

        Bit compute_zero_flag(const Bits& r) {
            for (Bit bit : r) {
                if (bit != 0) {
                    return 0; // not zero
                }
            }
            return 1; // all bits zero
        }
    } // anonymous namespace

    AluResult alu_execute(const Bits& a, const Bits& b, AluOp op) {
        // Allow any width <= 32 and zero-extend to exactly 32 bits.
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
                // ADD overflow: sign(rs1) == sign(rs2) and sign(result) != sign(rs1)
                flags.V = ((sign_a == sign_b) && (sign_r != sign_a)) ? 1 : 0;
                break;
            }

            case AluOp::Sub: {
                // rs1 - rs2 = rs1 + (~rs2 + 1)
                Bits neg_b = twos_negate_32(b32);
                Add32Result add_res = add_32(a32, neg_b);
                result = add_res.sum;

                Bit sign_a = a32[31];
                Bit sign_b = b32[31];
                Bit sign_r = result[31];

                flags.N = sign_r;
                flags.Z = compute_zero_flag(result);
                // For subtraction via a + (~b + 1), carry_out = 1 means "no borrow"
                flags.C = add_res.carry_out;
                // SUB overflow: sign(rs1) != sign(rs2) and sign(result) != sign(rs1)
                flags.V = ((sign_a != sign_b) && (sign_r != sign_a)) ? 1 : 0;
                break;
            }

            default:
                // For now, for shift ops (Sll/Srl/Sra) we just pass through 'a32'.
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

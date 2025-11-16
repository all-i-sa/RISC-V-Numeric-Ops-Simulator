#pragma once

#include "core/bitvec.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace rv::core {

    /***** MulOp *****
     *   Different types of multiplication
     ******************************
     *   Mul   - normal multiply low 32 bits
     *   Mulh  - signed * signed high 32 bits
     *   Mulhu - unsigned * unsigned high 32 bits
     *   Mulhsu- signed * unsigned high 32 bits
     ******************************/
    enum class MulOp {
        Mul,
        Mulh,
        Mulhu,
        Mulhsu
    };

    /***** DivOp *****
     *   Different types of diviision
     *
     * Values:
     *   Div  - signed division
     *   Divu - unsigned division
     *   Rem  - signed remainder
     *   Remu - unsigned remainder
     ******************************/
    enum class DivOp {
        Div,
        Divu,
        Rem,
        Remu
    };

    /***** MulResult *****
     *   The result of a multiply
     *
     *   lo       - low 32 bits of the 64-bit product
     *   hi       - high 32 bits
     *   overflow - true if the full 64-bit product does not fit in signed 32 bits
     *   trace
     ******************************/
    struct MulResult {
        Bits                     lo;
        Bits                     hi;
        bool                     overflow;
        std::vector<std::string> trace;
    };

    /***** DivResult *****
     *   The result of a divide and remainder
     *
     *   q        - quotient (32 bits)
     *   r        - remainder (32 bits)
     *   overflow
     *   trace
     ******************************/
    struct DivResult {
        Bits                     q;
        Bits                     r;
        bool                     overflow;
        std::vector<std::string> trace;
    };

    /***** mdu_mul *****
     *   Multiplies two 32-bit values given as bit vectors
     ******************************
     * Inputs:
     *   op  - which multiply mode to use (Mul, Mulh, ...)
     *   rs1 - first operand
     *   rs2 - second operand
     * Returns:
     *   MulResult - result
     ******************************/
    MulResult mdu_mul(MulOp op, const Bits& rs1, const Bits& rs2);

    /***** mdu_div *****
     *   Divides one 32-bit value
     ******************************
     * Inputs:
     *   op  - which divide mode to use (Div, Divu, ...)
     *   rs1 - dividend bits
     *   rs2 - divisor bits
     * Returns:
     *   DivResult - result
     ******************************/
    DivResult mdu_div(DivOp op, const Bits& rs1, const Bits& rs2);

} // namespace rv::core

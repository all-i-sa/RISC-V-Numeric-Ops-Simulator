#pragma once

#include "core/bitvec.hpp"

namespace rv::core {

    /***** AluOp *****
     *   A list of operations the ALU knows how to do
     *   Add - a + b
     *   Sub - a - b
     *   Sll - shift left logical
     *   Srl - shift right logical
     *   Sra - shift right arithmetic
     ******************************/
    enum class AluOp {
        Add,
        Sub,
        Sll,
        Srl,
        Sra
    };

    /***** AluFlags *****
     *   status flags
     *   N - Negative: 1 if the result looks negative in 2's comp
     *   Z - Zero:     1 if the result is exactly 0.
     *   C - Carry:    1 if there was a carry out
     *   V - Overflow: 1 if the signed result doesn't fit in 32 bits.
     ******************************/
    struct AluFlags {
        Bit N;
        Bit Z;
        Bit C;
        Bit V;
    };

    /***** AluResult *****
     *   The output of one ALU operation
     *   result - the 32-bit result
     *   flags  - the status flags
     ******************************/
    struct AluResult {
        Bits     result;
        AluFlags flags;
    };

    /***** alu_execute *****
     *   Runs one ALU operation on two 32-bit inputs
     *****************************
     * Inputs:
     *   a  - first operand
     *   b  - second operand
     *   op - which ALU operation to do
     * Output:
     *     - result
     *     - flags
     ******************************/
    AluResult alu_execute(const Bits& a, const Bits& b, AluOp op);

} // namespace rv::core

#pragma once

#include "core/bitvec.hpp"

namespace rv::core {

    // ALU operation selector
    enum class AluOp {
        Add,
        Sub,
        Sll,
        Srl,
        Sra
    };

    // ALU flags: Negative, Zero, Carry, Overflow
    struct AluFlags {
        Bit N;
        Bit Z;
        Bit C;
        Bit V;
    };

    struct AluResult {
        Bits     result; // always 32 bits for now
        AluFlags flags;
    };

    // Main ALU entry point (we'll implement logic tomorrow)
    AluResult alu_execute(const Bits& a, const Bits& b, AluOp op);

} // namespace rv::core

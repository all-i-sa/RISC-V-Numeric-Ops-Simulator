#pragma once

#include "core/bitvec.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace rv::core {

    // Multiply operation selector (for future extension)
    enum class MulOp {
        Mul,    // low 32 bits (required)
        Mulh,   // signed x signed high 32 (optional)
        Mulhu,  // unsigned x unsigned high 32 (optional)
        Mulhsu  // signed x unsigned high 32 (optional)
    };

    // Divide/remainder operation selector (for future extension)
    enum class DivOp {
        Div,   // signed division (required)
        Divu,  // unsigned division (optional)
        Rem,   // signed remainder (optional)
        Remu   // unsigned remainder (optional)
    };

    struct MulResult {
        Bits                    lo;       // low 32 bits
        Bits                    hi;       // high 32 bits (for *H variants; zero if unused)
        bool                    overflow; // true if 64-bit product doesn't fit signed 32
        std::vector<std::string> trace;   // per-step textual trace (optional for now)
    };

    struct DivResult {
        Bits                    q;        // quotient (32 bits)
        Bits                    r;        // remainder (32 bits)
        bool                    overflow; // for INT_MIN / -1 edge in signed DIV
        std::vector<std::string> trace;   // per-step textual trace
    };

    // Multiply unit entry point (we’ll implement shift-add later)
    MulResult mdu_mul(MulOp op, const Bits& rs1, const Bits& rs2);

    // Divide/remainder unit entry point (we’ll implement restoring/non-restoring later)
    DivResult mdu_div(DivOp op, const Bits& rs1, const Bits& rs2);

} // namespace rv::core

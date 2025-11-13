#pragma once

#include "core/bitvec.hpp"
#include <cstdint>

namespace rv::core {

    enum class ShiftOp {
        Sll,
        Srl,
        Sra
    };

    // Simple interface: shift a 32-bit value by shamt (0–31)
    Bits shifter_execute(const Bits& value, uint32_t shamt, ShiftOp op);

} // namespace rv::core

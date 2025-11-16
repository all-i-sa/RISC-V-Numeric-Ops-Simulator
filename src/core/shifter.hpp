#pragma once

#include "core/bitvec.hpp"
#include <cstdint>

namespace rv::core {

    /***** ShiftOp *****
     *   A small list of shift types
     *   Sll - shift left logical
     *   Srl - shift right logical
     *   Sra - shift right arithmetic
     ****************************/
    enum class ShiftOp {
        Sll,
        Srl,
        Sra
    };

    /***** shifter_execute *****
     *   Shifts a 32-bit value by n amount
     **************************
     * Inputs:
     *   value - bits for the value we want to shift
     *   shamt - how many positions to shift
     *   op    - which kind of shift to use
     * Output:
     *   A new vector holding the shifted 32-bit result
     ****************************/
    Bits shifter_execute(const Bits& value, uint32_t shamt, ShiftOp op);

} // namespace rv::core

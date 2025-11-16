#include "core/shifter.hpp"
#include <cassert>

namespace rv::core {

    /***** shifter_execute *****
     *   Shifts a 32-bit value by n
     **************************
     * Inputs:
     *   value - 32 bit vector
     *   shamt - how many positions to shift
     *   op    - which kind of shift:
     * Output:
     *   A new 32-bit bit vector
     ****************************/
    Bits shifter_execute(const Bits& value, uint32_t shamt, ShiftOp op) {
        assert(value.size() == 32);

        uint32_t s = shamt & 31u;

        Bits out(32, 0);

        switch (op) {
            case ShiftOp::Sll: {
                for (std::size_t i = 0; i < 32; ++i) {
                    std::size_t dest = i + s;
                    if (dest < 32) {
                        out[dest] = value[i];
                    }
                }
                break;
            }

            case ShiftOp::Srl: {
                for (std::size_t i = 0; i < 32; ++i) {
                    std::size_t src = i + s;
                    if (src < 32) {
                        out[i] = value[src];
                    } else {
                        out[i] = 0;
                    }
                }
                break;
            }

            case ShiftOp::Sra: {
                Bit sign = value[31];
                for (std::size_t i = 0; i < 32; ++i) {
                    std::size_t src = i + s;
                    if (src < 32) {
                        out[i] = value[src];
                    } else {
                        out[i] = sign;
                    }
                }
                break;
            }
        }

        return out;
    }

} // namespace rv::core

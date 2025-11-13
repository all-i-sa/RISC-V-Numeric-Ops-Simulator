#include "core/shifter.hpp"
#include <cassert>

namespace rv::core {

    Bits shifter_execute(const Bits& value, uint32_t shamt, ShiftOp op) {
        assert(value.size() == 32);

        // RISC-V only uses low 5 bits of shamt for 32-bit shifts
        uint32_t s = shamt & 31u;

        Bits out(32, 0);

        switch (op) {
            case ShiftOp::Sll: {
                // Logical left: shift towards MSB, fill with zeros.
                for (std::size_t i = 0; i < 32; ++i) {
                    std::size_t dest = i + s;
                    if (dest < 32) {
                        out[dest] = value[i];
                    }
                }
                break;
            }

            case ShiftOp::Srl: {
                // Logical right: shift towards LSB, fill with zeros.
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
                // Arithmetic right: like SRL but fill with sign bit.
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

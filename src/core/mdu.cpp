//
// Created by Allisa Warren on 11/12/25.
//
#include "core/mdu.hpp"
#include "core/bitvec.hpp"
#include <cassert>

namespace rv::core {

    MulResult mdu_mul(MulOp op, const Bits& rs1, const Bits& rs2) {
        (void)op; // unused for now

        // For now, just zero-extend inputs to 32 bits for sanity, but we won't use them.
        Bits rs1_32 = zero_extend(rs1, 32);
        Bits rs2_32 = zero_extend(rs2, 32);
        (void)rs1_32;
        (void)rs2_32;

        // Stub: return zeros, no overflow, empty trace.
        Bits lo(32, 0);
        Bits hi(32, 0);

        MulResult res{
            /*lo=*/lo,
            /*hi=*/hi,
            /*overflow=*/false,
            /*trace=*/{}
        };
        return res;
    }

    DivResult mdu_div(DivOp op, const Bits& rs1, const Bits& rs2) {
        (void)op; // unused for now

        // Zero-extend to 32 bits for consistency.
        Bits rs1_32 = zero_extend(rs1, 32);
        Bits rs2_32 = zero_extend(rs2, 32);
        (void)rs1_32;
        (void)rs2_32;

        // Stub: quotient and remainder both zero, no overflow, empty trace.
        Bits q(32, 0);
        Bits r(32, 0);

        DivResult res{
            /*q=*/q,
            /*r=*/r,
            /*overflow=*/false,
            /*trace=*/{}
        };
        return res;
    }

} // namespace rv::core

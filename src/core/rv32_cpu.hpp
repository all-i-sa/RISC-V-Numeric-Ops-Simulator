#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace rv::cpu {

    /***** CpuState *****
     *   The snapshot of the CPU at a moment in time
     *
     *   regs[32] - 32 general purpose registers
     *   pc       - program counter
     *   mem      - memory
     *
     * Constructor: CpuState(mem_size)
     *     - Creates a CPU with mem_size bytes of memory
     *     - Memory starts out zero-initialized
     ******************************/
    struct CpuState {
        uint32_t regs[32];
        uint32_t pc;
        std::vector<uint8_t> mem;

        CpuState(std::size_t mem_size = 1024);
    };

    /***** reset *****
     *   Resets the CPU to a clean state
     *   - Sets all registers to 0
     *   - Sets the pc to 0
     *   - Fills memory with zeros
     ******************************
     * Input:
     *   s - the CpuState to reset
     ******************************/
    void reset(CpuState& s);

    /***** load_program *****
     *   Loads a program into memory
     ******************************
     * Inputs:
     *   s         - the CpuState whose memory we are writing to
     *   words     - vector of 32-bit instructions to store
     *   base_addr - byte address where the first instruction should go
     ******************************/
    void load_program(CpuState& s, const std::vector<uint32_t>& words, uint32_t base_addr = 0);

    /***** step *****
     *   Runs a single instruction at s.pc
     ******************************
     * Input:
     *   s - the current CPU state
     ******************************/
    void step(CpuState& s);

    /***** run *****
     *   Keeps calling steps in a loop
     *
     *   - Runs up to max_steps instructions.
     *   - Stops early if pc moves to an address that is outside memory
     ******************************
     * Inputs:
     *   s         - the CPU state to run
     *   max_steps - limit to stop looping
     ******************************/
    void run(CpuState& s, std::size_t max_steps = 1000);

} // namespace rv::cpu

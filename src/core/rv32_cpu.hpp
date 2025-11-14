#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace rv::cpu {

    struct CpuState {
        uint32_t regs[32];   // x0..x31
        uint32_t pc;         // program counter (byte address)
        std::vector<uint8_t> mem; // simple flat memory

        CpuState(std::size_t mem_size = 1024);
    };

    // Reset registers and PC to 0, clear memory.
    void reset(CpuState& s);

    // Load a program (array of 32-bit instructions) into memory at a given address.
    void load_program(CpuState& s, const std::vector<uint32_t>& words, uint32_t base_addr = 0);

    // Execute a single instruction at s.pc (single-cycle model).
    void step(CpuState& s);

    // Run until max_steps or until PC points to an invalid area (simple stop).
    void run(CpuState& s, std::size_t max_steps = 1000);

} // namespace rv::cpu

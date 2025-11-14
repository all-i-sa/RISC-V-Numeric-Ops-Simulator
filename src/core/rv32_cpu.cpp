//
// Created by Allisa Warren on 11/13/25.
//
#include "core/rv32_cpu.hpp"
#include <cassert>

namespace rv::cpu {

    CpuState::CpuState(std::size_t mem_size)
        : regs{0}, pc(0), mem(mem_size, 0) {}

    void reset(CpuState& s) {
        for (int i = 0; i < 32; ++i) {
            s.regs[i] = 0;
        }
        s.pc = 0;
        std::fill(s.mem.begin(), s.mem.end(), 0);
    }

    void load_program(CpuState& s, const std::vector<uint32_t>& words, uint32_t base_addr) {
        // Little-endian: write each 32-bit word as 4 bytes.
        uint32_t addr = base_addr;
        for (uint32_t w : words) {
            assert(addr + 3 < s.mem.size());
            s.mem[addr + 0] = static_cast<uint8_t>(w & 0xFF);
            s.mem[addr + 1] = static_cast<uint8_t>((w >> 8) & 0xFF);
            s.mem[addr + 2] = static_cast<uint8_t>((w >> 16) & 0xFF);
            s.mem[addr + 3] = static_cast<uint8_t>((w >> 24) & 0xFF);
            addr += 4;
        }
        s.pc = base_addr;
    }

    static uint32_t load_u32(const CpuState& s, uint32_t addr) {
        assert(addr + 3 < s.mem.size());
        uint32_t b0 = s.mem[addr + 0];
        uint32_t b1 = s.mem[addr + 1];
        uint32_t b2 = s.mem[addr + 2];
        uint32_t b3 = s.mem[addr + 3];
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    static int32_t sign_extend_imm(uint32_t x, int bits) {
        // Sign-extend "bits"-wide immediate stored in low bits of x.
        int32_t shift = 32 - bits;
        return (static_cast<int32_t>(x) << shift) >> shift;
    }

    void step(CpuState& s) {
        assert((s.pc % 4) == 0);
        uint32_t instr = load_u32(s, s.pc);

        uint32_t opcode = instr & 0x7F;
        uint32_t rd     = (instr >> 7) & 0x1F;
        uint32_t funct3 = (instr >> 12) & 0x07;
        uint32_t rs1    = (instr >> 15) & 0x1F;
        uint32_t rs2    = (instr >> 20) & 0x1F;
        uint32_t funct7 = (instr >> 25) & 0x7F;

        uint32_t next_pc = s.pc + 4;

        auto read_reg = [&](uint32_t idx) -> uint32_t {
            if (idx == 0) return 0;
            assert(idx < 32);
            return s.regs[idx];
        };

        auto write_reg = [&](uint32_t idx, uint32_t value) {
            if (idx == 0) return; // x0 is hard-wired to 0
            assert(idx < 32);
            s.regs[idx] = value;
        };

        switch (opcode) {
            case 0x13: { // OP-IMM (e.g., ADDI)
                int32_t imm = sign_extend_imm(instr >> 20, 12);
                uint32_t val1 = read_reg(rs1);
                switch (funct3) {
                    case 0x0: { // ADDI
                        uint32_t res = val1 + static_cast<uint32_t>(imm);
                        write_reg(rd, res);
                        break;
                    }
                    default:
                        // TODO: implement other OP-IMM variants
                        break;
                }
                break;
            }

            case 0x33: { // OP (ADD/SUB/etc.)
                uint32_t val1 = read_reg(rs1);
                uint32_t val2 = read_reg(rs2);
                if (funct3 == 0x0) {
                    if (funct7 == 0x00) {
                        // ADD
                        write_reg(rd, val1 + val2);
                    } else if (funct7 == 0x20) {
                        // SUB
                        write_reg(rd, val1 - val2);
                    }
                } else {
                    // TODO: other arithmetic/logical ops
                }
                break;
            }

            default:
                // TODO: handle other opcodes (loads, stores, branches, jumps, LUI/AUIPC)
                break;
        }

        s.pc = next_pc;
    }

    void run(CpuState& s, std::size_t max_steps) {
        for (std::size_t i = 0; i < max_steps; ++i) {
            step(s);
        }
    }

} // namespace rv::cpu

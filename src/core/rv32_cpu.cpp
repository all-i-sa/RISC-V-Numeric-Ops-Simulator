#include "core/rv32_cpu.hpp"
#include <cassert>

namespace rv::cpu {

    /***** CpuState constructor *****
     *   Creates a CPU with a given amount of memory
     ******************************/
    CpuState::CpuState(std::size_t mem_size)
        : regs{0}, pc(0), mem(mem_size, 0) {}

    /***** reset *****
     *   Puts the CPU back into a clean starting state
     ******************************/
    void reset(CpuState& s) {
        for (int i = 0; i < 32; ++i) {
            s.regs[i] = 0;
        }
        s.pc = 0;
        std::fill(s.mem.begin(), s.mem.end(), 0);
    }

    /***** load_program *****
     *   Loads a list of 32 bit instructions into memory
     ******************************/
    void load_program(CpuState& s, const std::vector<uint32_t>& words, uint32_t base_addr) {
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

    /***** load_u32 (helper) *****
     *   Reads a 32-bit little-endian word from memory
     *******************************
     * Input:
     *   addr - byte address in memory
     * Returns:
     *   32-bit value made from mem
     ******************************/
    static uint32_t load_u32(const CpuState& s, uint32_t addr) {
        assert(addr + 3 < s.mem.size());
        uint32_t b0 = s.mem[addr + 0];
        uint32_t b1 = s.mem[addr + 1];
        uint32_t b2 = s.mem[addr + 2];
        uint32_t b3 = s.mem[addr + 3];
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    /***** store_u32 (helper) *****
     *   Writes a 32-bit value into memory in little endian format
     *******************************
     * Input:
     *   addr  - where to store it
     *   value - the 32 bit value to write
     ******************************/
    static void store_u32(CpuState& s, uint32_t addr, uint32_t value) {
        assert(addr + 3 < s.mem.size());
        s.mem[addr + 0] = static_cast<uint8_t>(value & 0xFF);
        s.mem[addr + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        s.mem[addr + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        s.mem[addr + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    /***** sign_extend_imm (helper) *****
     *   Sign-extends an imm
     ******************************/
    static int32_t sign_extend_imm(uint32_t x, int bits) {
        int32_t shift = 32 - bits;
        return (static_cast<int32_t>(x) << shift) >> shift;
    }

    /***** step *****
     *   Runs a single instruction at s.pc
     ******************************/
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
            if (idx == 0) return; // x0 stays 0
            assert(idx < 32);
            s.regs[idx] = value;
        };

        switch (opcode) {

            case 0x13: { // OP-IMM
                int32_t imm = sign_extend_imm(instr >> 20, 12);
                uint32_t val1 = read_reg(rs1);

                switch (funct3) {
                    case 0x0: { // ADDI
                        uint32_t res = val1 + static_cast<uint32_t>(imm);
                        write_reg(rd, res);
                        break;
                    }
                    case 0x7: { // ANDI
                        uint32_t res = val1 & static_cast<uint32_t>(imm);
                        write_reg(rd, res);
                        break;
                    }
                    case 0x6: { // ORI
                        uint32_t res = val1 | static_cast<uint32_t>(imm);
                        write_reg(rd, res);
                        break;
                    }
                    case 0x4: { // XORI
                        uint32_t res = val1 ^ static_cast<uint32_t>(imm);
                        write_reg(rd, res);
                        break;
                    }
                    case 0x1: { // SLLI
                        uint32_t shamt = (instr >> 20) & 0x1F;
                        uint32_t res   = val1 << shamt;
                        write_reg(rd, res);
                        break;
                    }
                    case 0x5: { // SRLI / SRAI
                        uint32_t shamt = (instr >> 20) & 0x1F;
                        if (funct7 == 0x00) {
                            // SRLI
                            uint32_t res = val1 >> shamt;
                            write_reg(rd, res);
                        } else if (funct7 == 0x20) {
                            // SRAI
                            int32_t sval = static_cast<int32_t>(val1);
                            int32_t sres = sval >> static_cast<int32_t>(shamt);
                            write_reg(rd, static_cast<uint32_t>(sres));
                        }
                        break;
                    }
                    default:
                        // other op-imm but are not implemented yet
                        break;
                }
                break;
            }
            case 0x33: { // OP
                uint32_t val1 = read_reg(rs1);
                uint32_t val2 = read_reg(rs2);

                switch (funct3) {
                    case 0x0: { // ADD/SUB
                        if (funct7 == 0x00) {
                            // ADD
                            write_reg(rd, val1 + val2);
                        } else if (funct7 == 0x20) {
                            // SUB
                            write_reg(rd, val1 - val2);
                        }
                        break;
                    }
                    case 0x7: { // AND
                        write_reg(rd, val1 & val2);
                        break;
                    }
                    case 0x6: { // OR
                        write_reg(rd, val1 | val2);
                        break;
                    }
                    case 0x4: { // XOR
                        write_reg(rd, val1 ^ val2);
                        break;
                    }
                    case 0x1: { // SLL
                        uint32_t shamt = val2 & 0x1F;
                        write_reg(rd, val1 << shamt);
                        break;
                    }
                    case 0x5: { // SRL / SRA
                        uint32_t shamt = val2 & 0x1F;
                        if (funct7 == 0x00) {
                            // SRL
                            write_reg(rd, val1 >> shamt);
                        } else if (funct7 == 0x20) {
                            // SRA
                            int32_t sval = static_cast<int32_t>(val1);
                            int32_t sres = sval >> static_cast<int32_t>(shamt);
                            write_reg(rd, static_cast<uint32_t>(sres));
                        }
                        break;
                    }
                    default:
                        // Other OP not implemented yet
                        break;
                }
                break;
            }

            // lw
            case 0x03: { // LOAD
                int32_t imm = sign_extend_imm(instr >> 20, 12);
                uint32_t base = read_reg(rs1);
                uint32_t addr = base + static_cast<uint32_t>(imm);

                switch (funct3) {
                    case 0x2: { // load word
                        uint32_t val = load_u32(s, addr);
                        write_reg(rd, val);
                        break;
                    }
                    default:
                        // Other loads not implemented yet
                        break;
                }
                break;
            }

            // STORE
            case 0x23: { // STORE
                uint32_t imm_11_5 = instr >> 25;
                uint32_t imm_4_0  = (instr >> 7) & 0x1F;
                uint32_t imm_u    = (imm_11_5 << 5) | imm_4_0;
                int32_t imm       = sign_extend_imm(imm_u, 12);

                uint32_t base = read_reg(rs1);
                uint32_t addr = base + static_cast<uint32_t>(imm);
                uint32_t val  = read_reg(rs2);

                switch (funct3) {
                    case 0x2: { // sw
                        store_u32(s, addr, val);
                        break;
                    }
                    default:
                        // Other stores not implemented yet
                        break;
                }
                break;
            }

            // Branch
            case 0x63: { // branch
                uint32_t imm_12   = (instr >> 31) & 0x1;
                uint32_t imm_10_5 = (instr >> 25) & 0x3F;
                uint32_t imm_4_1  = (instr >> 8) & 0xF;
                uint32_t imm_11   = (instr >> 7) & 0x1;

                uint32_t imm_u = (imm_12 << 12)
                               | (imm_11 << 11)
                               | (imm_10_5 << 5)
                               | (imm_4_1 << 1);

                int32_t offset = sign_extend_imm(imm_u, 13);

                uint32_t val1 = read_reg(rs1);
                uint32_t val2 = read_reg(rs2);

                bool take = false;
                switch (funct3) {
                    case 0x0: // BEQ
                        take = (val1 == val2);
                        break;
                    case 0x1: // BNE
                        take = (val1 != val2);
                        break;
                    default:
                        // Other branches not implemented yet
                        break;
                }

                if (take) {
                    next_pc = s.pc + static_cast<uint32_t>(offset);
                }
                break;
            }

            // JAL
            case 0x6F: {
                uint32_t pc0 = s.pc;
                uint32_t imm_20    = (instr >> 31) & 0x1;
                uint32_t imm_10_1  = (instr >> 21) & 0x3FF;
                uint32_t imm_11    = (instr >> 20) & 0x1;
                uint32_t imm_19_12 = (instr >> 12) & 0xFF;

                uint32_t imm_u = (imm_20 << 20)
                               | (imm_19_12 << 12)
                               | (imm_11 << 11)
                               | (imm_10_1 << 1);

                int32_t offset = sign_extend_imm(imm_u, 21);

                write_reg(rd, pc0 + 4);

                next_pc = pc0 + static_cast<uint32_t>(offset);
                break;
            }

            // JALR
            case 0x67: {
                uint32_t pc0 = s.pc;

                int32_t imm = sign_extend_imm(instr >> 20, 12);
                uint32_t base = read_reg(rs1);

                uint32_t target = base + static_cast<uint32_t>(imm);
                target &= ~1u; // LSB

                write_reg(rd, pc0 + 4);

                next_pc = target;
                break;
            }

            case 0x17: {
                uint32_t pc0 = s.pc;
                uint32_t imm20 = instr & 0xFFFFF000u;

                uint32_t offset = imm20;
                write_reg(rd, pc0 + offset);
                break;
            }

            // LUI
            case 0x37: {
                uint32_t imm20 = instr & 0xFFFFF000u;
                write_reg(rd, imm20);
                break;
            }

            default:
                // opcodes not handled yet
                break;
        }

        // move PC to the next instruction
        s.pc = next_pc;
    }

    /***** run *****
     *   Repeatedly calls steps up to max_steps
     ******************************/
    void run(CpuState& s, std::size_t max_steps) {
        for (std::size_t i = 0; i < max_steps; ++i) {
            step(s);
        }
    }

} // namespace rv::cpu

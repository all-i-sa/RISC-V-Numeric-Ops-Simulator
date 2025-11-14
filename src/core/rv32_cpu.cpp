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

    static void store_u32(CpuState& s, uint32_t addr, uint32_t value) {
        assert(addr + 3 < s.mem.size());
        s.mem[addr + 0] = static_cast<uint8_t>(value & 0xFF);
        s.mem[addr + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        s.mem[addr + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        s.mem[addr + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
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
        if (idx == 0) return; // x0 stays 0
        assert(idx < 32);
        s.regs[idx] = value;
    };

    switch (opcode) {

        // OP-IMM: addi, andi, ori, xori, slli, srli, srai
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
                        // SRLI (logical)
                        uint32_t res = val1 >> shamt;
                        write_reg(rd, res);
                    } else if (funct7 == 0x20) {
                        // SRAI (arithmetic)
                        int32_t sval = static_cast<int32_t>(val1);
                        int32_t sres = sval >> static_cast<int32_t>(shamt);
                        write_reg(rd, static_cast<uint32_t>(sres));
                    }
                    break;
                }
                default:
                    // Unimplemented OP-IMM variant
                    break;
            }
            break;
        }

        // OP: add, sub, and, or, xor, sll, srl, sra
        case 0x33: { // OP (register-register)
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
                    // Unimplemented OP variant
                    break;
            }
            break;
        }

        // LOAD: lw
        case 0x03: { // LOAD
            int32_t imm = sign_extend_imm(instr >> 20, 12);
            uint32_t base = read_reg(rs1);
            uint32_t addr = base + static_cast<uint32_t>(imm);

            switch (funct3) {
                case 0x2: { // LW
                    uint32_t val = load_u32(s, addr);
                    write_reg(rd, val);
                    break;
                }
                default:
                    // Unimplemented load type
                    break;
            }
            break;
        }

        // STORE: sw
        case 0x23: { // STORE
            // S-type immediate
            uint32_t imm_11_5 = instr >> 25;
            uint32_t imm_4_0  = (instr >> 7) & 0x1F;
            uint32_t imm_u    = (imm_11_5 << 5) | imm_4_0;
            int32_t imm       = sign_extend_imm(imm_u, 12);

            uint32_t base = read_reg(rs1);
            uint32_t addr = base + static_cast<uint32_t>(imm);
            uint32_t val  = read_reg(rs2);

            switch (funct3) {
                case 0x2: { // SW
                    store_u32(s, addr, val);
                    break;
                }
                default:
                    // Unimplemented store type
                    break;
            }
            break;
        }
                // BRANCH: beq, bne (B-type)
        case 0x63: { // BRANCH
            // B-type immediate:
            // imm[12] at bit 31
            // imm[10:5] at bits 30:25
            // imm[4:1] at bits 11:8
            // imm[11] at bit 7
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
                    // other branches (BLT/BGE/BLTU/BGEU) not implemented
                    break;
            }

            if (take) {
                next_pc = s.pc + static_cast<uint32_t>(offset);
            }
            break;
        }
        // JAL: jump and link (J-type)
        case 0x6F: {
            uint32_t pc0 = s.pc;

            // J-type immediate layout:
            // imm[20]   at bit 31
            // imm[10:1] at bits 30:21
            // imm[11]   at bit 20
            // imm[19:12]at bits 19:12
            uint32_t imm_20    = (instr >> 31) & 0x1;
            uint32_t imm_10_1  = (instr >> 21) & 0x3FF;
            uint32_t imm_11    = (instr >> 20) & 0x1;
            uint32_t imm_19_12 = (instr >> 12) & 0xFF;

            uint32_t imm_u = (imm_20 << 20)
                           | (imm_19_12 << 12)
                           | (imm_11 << 11)
                           | (imm_10_1 << 1);

            int32_t offset = sign_extend_imm(imm_u, 21);

            // rd gets return address (pc + 4)
            write_reg(rd, pc0 + 4);

            // pc jumps to pc + offset
            next_pc = pc0 + static_cast<uint32_t>(offset);
            break;
        }
        // JALR: jump and link register (I-type)
        case 0x67: {
            uint32_t pc0 = s.pc;

            // I-type immediate in bits 31:20
            int32_t imm = sign_extend_imm(instr >> 20, 12);
            uint32_t base = read_reg(rs1);

            uint32_t target = base + static_cast<uint32_t>(imm);
            target &= ~1u; // clear LSB

            // rd gets return address (pc + 4)
            write_reg(rd, pc0 + 4);

            next_pc = target;
            break;
        }
        // AUIPC: rd = pc + (imm20 << 12)
        case 0x17: {
            uint32_t pc0 = s.pc;
            uint32_t imm20 = instr & 0xFFFFF000u; // bits 31:12 already in place

            uint32_t offset = imm20; // already << 12
            write_reg(rd, pc0 + offset);
            break;
        }
        // LUI: rd = imm20 << 12
        case 0x37: {
            uint32_t imm20 = instr & 0xFFFFF000u; // bits 31:12 already in place
            write_reg(rd, imm20);
            break;
        }
        default:
            // Other opcodes (branches, jumps, etc.) will be added later.
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

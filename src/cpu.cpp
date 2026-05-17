// cpu.cpp — CPU fetch / decode / execute.
//
// Single-cycle reference implementation. Memory is byte-addressable with
// big-endian word storage (matches the MIPS standard).

#include "mips.h"

#include <cstdint>
#include <stdexcept>

namespace mips {
namespace {

uint32_t mem_read_word(const std::array<uint8_t, MEM_SIZE> &mem, uint32_t addr) {
    if (addr + 4 > MEM_SIZE) throw std::runtime_error("memory read out of range");
    return (uint32_t(mem[addr]) << 24) | (uint32_t(mem[addr + 1]) << 16) |
           (uint32_t(mem[addr + 2]) << 8) | uint32_t(mem[addr + 3]);
}

void mem_write_word(std::array<uint8_t, MEM_SIZE> &mem, uint32_t addr, uint32_t value) {
    if (addr + 4 > MEM_SIZE) throw std::runtime_error("memory write out of range");
    mem[addr]     = static_cast<uint8_t>((value >> 24) & 0xff);
    mem[addr + 1] = static_cast<uint8_t>((value >> 16) & 0xff);
    mem[addr + 2] = static_cast<uint8_t>((value >> 8) & 0xff);
    mem[addr + 3] = static_cast<uint8_t>(value & 0xff);
}

int16_t sign_extend_imm(uint16_t imm) {
    return static_cast<int16_t>(imm);
}

}  // namespace

// ---------------------------------------------------------------------------

void CPU::load(const std::vector<Word> &program) {
    for (std::size_t i = 0; i < program.size(); ++i) {
        mem_write_word(mem, TEXT_BASE + static_cast<uint32_t>(i * 4), program[i]);
    }
    regs[0] = 0;
    pc      = TEXT_BASE;
    halted  = false;
    cycles  = 0;
}

Word CPU::fetch() const {
    return mem_read_word(mem, pc);
}

bool CPU::step() {
    if (halted) return false;
    Word instr = fetch();
    uint32_t opcode = (instr >> 26) & 0x3f;
    uint32_t rs     = (instr >> 21) & 0x1f;
    uint32_t rt     = (instr >> 16) & 0x1f;
    uint32_t rd     = (instr >> 11) & 0x1f;
    uint32_t shamt  = (instr >> 6)  & 0x1f;
    uint32_t funct  = instr & 0x3f;
    int16_t  imm    = sign_extend_imm(static_cast<uint16_t>(instr & 0xffff));
    uint32_t target = instr & 0x03ffffff;
    uint32_t next_pc = pc + 4;

    switch (opcode) {
    case 0: {
        // R-type
        switch (funct) {
        case 0x20: regs[rd] = regs[rs] + regs[rt]; break;            // add
        case 0x22: regs[rd] = regs[rs] - regs[rt]; break;            // sub
        case 0x24: regs[rd] = regs[rs] & regs[rt]; break;            // and
        case 0x25: regs[rd] = regs[rs] | regs[rt]; break;            // or
        case 0x26: regs[rd] = regs[rs] ^ regs[rt]; break;            // xor
        case 0x27: regs[rd] = ~(regs[rs] | regs[rt]); break;         // nor
        case 0x2a: regs[rd] = (regs[rs] < regs[rt]) ? 1 : 0; break;  // slt
        case 0x00:
            regs[rd] = static_cast<int32_t>(
                static_cast<uint32_t>(regs[rt]) << shamt);            // sll
            break;
        case 0x02:
            regs[rd] = static_cast<int32_t>(
                static_cast<uint32_t>(regs[rt]) >> shamt);            // srl
            break;
        case 0x08:                                                    // jr
            next_pc = static_cast<uint32_t>(regs[rs]);
            break;
        default:
            throw std::runtime_error("R-type funct not supported: " + std::to_string(funct));
        }
        break;
    }
    case 0x08: regs[rt] = regs[rs] + imm; break;                      // addi
    case 0x0c: regs[rt] = regs[rs] & (imm & 0xffff); break;           // andi (zero-ext)
    case 0x0d: regs[rt] = regs[rs] | (imm & 0xffff); break;           // ori
    case 0x0e: regs[rt] = regs[rs] ^ (imm & 0xffff); break;           // xori
    case 0x0a: regs[rt] = (regs[rs] < imm) ? 1 : 0; break;            // slti
    case 0x0f: regs[rt] = static_cast<int32_t>(static_cast<uint32_t>(imm) << 16); break; // lui
    case 0x23: regs[rt] = static_cast<int32_t>(                       // lw
                   mem_read_word(mem, static_cast<uint32_t>(regs[rs] + imm))); break;
    case 0x2b: mem_write_word(mem,                                    // sw
                   static_cast<uint32_t>(regs[rs] + imm),
                   static_cast<uint32_t>(regs[rt])); break;
    case 0x20: {                                                      // lb
        uint32_t addr = static_cast<uint32_t>(regs[rs] + imm);
        if (addr >= MEM_SIZE) throw std::runtime_error("lb: addr out of range");
        regs[rt] = static_cast<int8_t>(mem[addr]);
        break;
    }
    case 0x28: {                                                      // sb
        uint32_t addr = static_cast<uint32_t>(regs[rs] + imm);
        if (addr >= MEM_SIZE) throw std::runtime_error("sb: addr out of range");
        mem[addr] = static_cast<uint8_t>(regs[rt] & 0xff);
        break;
    }
    case 0x04:                                                        // beq
        if (regs[rs] == regs[rt]) next_pc = pc + 4 + (static_cast<int32_t>(imm) << 2);
        break;
    case 0x05:                                                        // bne
        if (regs[rs] != regs[rt]) next_pc = pc + 4 + (static_cast<int32_t>(imm) << 2);
        break;
    case 0x02:                                                        // j
        next_pc = (pc & 0xf0000000) | (target << 2);
        break;
    case 0x03:                                                        // jal
        regs[31] = static_cast<int32_t>(pc + 4);
        next_pc = (pc & 0xf0000000) | (target << 2);
        break;
    default:
        throw std::runtime_error("opcode not supported: " + std::to_string(opcode));
    }

    regs[0] = 0;  // $zero is always 0 regardless of writes
    pc = next_pc;
    ++cycles;
    return true;
}

void CPU::run(uint64_t max_cycles) {
    // Halt when PC walks off the end of loaded text. We track that by reading
    // the next word; if it's zero AND there's no surrounding program, treat
    // a long run of zeros as halt. Simpler heuristic: stop when PC == 0
    // again (which jr $ra to zero achieves) or after max_cycles.
    uint64_t start = cycles;
    while (!halted && cycles - start < max_cycles) {
        if (pc >= MEM_SIZE - 4) { halted = true; break; }
        Word next = mem_read_word(mem, pc);
        if (next == 0 && pc != TEXT_BASE) {
            // ran into a NOP at end-of-program; stop unless it's intentional
            // (sll $zero,$zero,0 == 0 is a valid nop, but typically appears
            // because we walked past the last real instruction).
            halted = true;
            break;
        }
        step();
    }
}

}  // namespace mips

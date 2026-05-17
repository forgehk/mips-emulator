// mips.h — MIPS emulator public interface.
//
// Author: @forgehk.

#ifndef MIPS_H
#define MIPS_H

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mips {

constexpr std::size_t MEM_SIZE   = 64 * 1024;   // 64 KB
constexpr std::size_t NUM_REGS   = 32;
constexpr uint32_t    TEXT_BASE  = 0x0000;      // where assembled code lives

// Encoded 32-bit MIPS instruction.
using Word = uint32_t;

// Register name <-> number helpers.
int reg_number(const std::string &name);
std::string reg_name(int n);

// Assemble a source program (lines of text) into a vector of 32-bit words.
// Throws std::runtime_error on syntax/label errors.
std::vector<Word> assemble(const std::vector<std::string> &lines);

// Convenience: assemble from a single string with newlines.
std::vector<Word> assemble(const std::string &source);

// CPU state.
struct CPU {
    std::array<int32_t, NUM_REGS> regs{};
    std::array<uint8_t, MEM_SIZE> mem{};
    uint32_t pc = TEXT_BASE;
    bool     halted = false;
    uint64_t cycles = 0;

    // Load program into memory at TEXT_BASE, big-endian-friendly storage.
    void load(const std::vector<Word> &program);

    // Fetch the word at the current PC (does NOT advance PC).
    Word fetch() const;

    // Execute a single instruction (advances PC).
    // Returns false if the program tried to step past the loaded text.
    bool step();

    // Run until halted or out of program.
    void run(uint64_t max_cycles = 1'000'000);
};

}  // namespace mips

#endif  // MIPS_H

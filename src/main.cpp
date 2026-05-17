// main.cpp — `mips` CLI entrypoint.

#include "mips.h"

#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

const char *kUsage = R"(mips — MIPS-32 instruction emulator

Usage:
  mips run [--trace] [--dump START,END] <file.s>

Flags:
  --trace        print PC, instruction, and selected registers each cycle
  --dump A,B     hex-dump memory bytes [A, B) after execution

Examples:
  mips run examples/sum_first_10.s
  mips run --trace examples/factorial.s
)";

std::string read_file(const std::string &path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("could not open: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

void print_registers(const mips::CPU &cpu) {
    for (int i = 0; i < 32; ++i) {
        if (cpu.regs[i] == 0 && i != 0) continue;
        std::cout << "  " << mips::reg_name(i) << " = " << cpu.regs[i] << "\n";
    }
}

}  // namespace

int main(int argc, char **argv) {
    if (argc < 3 || std::string(argv[1]) != "run") {
        std::cout << kUsage;
        return 2;
    }
    bool trace = false;
    bool do_dump = false;
    uint32_t dump_a = 0, dump_b = 0;
    std::string path;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--trace") { trace = true; }
        else if (arg == "--dump" && i + 1 < argc) {
            std::string spec = argv[++i];
            auto comma = spec.find(',');
            if (comma == std::string::npos) { std::cerr << "bad --dump spec\n"; return 2; }
            dump_a = std::stoul(spec.substr(0, comma), nullptr, 0);
            dump_b = std::stoul(spec.substr(comma + 1), nullptr, 0);
            do_dump = true;
        }
        else if (!arg.empty() && arg[0] != '-') { path = arg; }
        else { std::cerr << "unknown option: " << arg << "\n"; return 2; }
    }
    if (path.empty()) { std::cerr << kUsage; return 2; }

    try {
        auto program = mips::assemble(read_file(path));
        mips::CPU cpu;
        cpu.load(program);

        if (trace) {
            while (!cpu.halted && cpu.pc + 4 <= mips::MEM_SIZE) {
                mips::Word ins = cpu.fetch();
                if (ins == 0 && cpu.pc != mips::TEXT_BASE) break;
                std::cout << "pc=0x" << std::hex << std::setw(4) << std::setfill('0') << cpu.pc
                          << "  ins=0x" << std::setw(8) << ins << std::dec << "\n";
                cpu.step();
            }
        } else {
            cpu.run();
        }

        std::cout << "program halted after " << cpu.cycles << " cycles\n";
        print_registers(cpu);

        if (do_dump) {
            std::cout << "memory[0x" << std::hex << dump_a
                      << "..0x" << dump_b << "]:\n";
            for (uint32_t a = dump_a; a < dump_b && a < mips::MEM_SIZE; a += 16) {
                std::cout << "  0x" << std::setw(4) << std::setfill('0') << a << ": ";
                for (uint32_t j = 0; j < 16 && a + j < dump_b; ++j) {
                    std::cout << std::setw(2) << static_cast<int>(cpu.mem[a + j]) << " ";
                }
                std::cout << "\n";
            }
            std::cout << std::dec;
        }
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

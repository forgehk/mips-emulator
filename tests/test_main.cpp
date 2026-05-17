// test_main.cpp — minimal self-checking test harness for the MIPS emulator.

#include "mips.h"

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

int g_failures = 0;
int g_total    = 0;

void check_eq(const std::string &name, int32_t got, int32_t want) {
    ++g_total;
    if (got != want) {
        std::cerr << "  ✗ " << name << ": got " << got << ", want " << want << "\n";
        ++g_failures;
    } else {
        std::cout << "  ✓ " << name << "\n";
    }
}

mips::CPU run_program(const std::string &src) {
    auto prog = mips::assemble(src);
    mips::CPU cpu;
    cpu.load(prog);
    cpu.run();
    return cpu;
}

void test_addi_and_add() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 5
        addi $t1, $zero, 7
        add  $v0, $t0, $t1
    )");
    check_eq("addi+add", cpu.regs[2], 12);
}

void test_sub() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 10
        addi $t1, $zero, 3
        sub  $v0, $t0, $t1
    )");
    check_eq("sub", cpu.regs[2], 7);
}

void test_and_or_xor_nor() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 12
        addi $t1, $zero, 10
        and  $s0, $t0, $t1
        or   $s1, $t0, $t1
        xor  $s2, $t0, $t1
        nor  $s3, $t0, $t1
    )");
    check_eq("and 12&10", cpu.regs[16], 12 & 10);
    check_eq("or  12|10", cpu.regs[17], 12 | 10);
    check_eq("xor 12^10", cpu.regs[18], 12 ^ 10);
    check_eq("nor",       cpu.regs[19], ~(12 | 10));
}

void test_slt() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 5
        addi $t1, $zero, 10
        slt  $v0, $t0, $t1
        slt  $v1, $t1, $t0
    )");
    check_eq("slt 5<10",  cpu.regs[2], 1);
    check_eq("slt 10<5",  cpu.regs[3], 0);
}

void test_sll_srl() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 1
        sll  $v0, $t0, 4
        addi $t1, $zero, 64
        srl  $v1, $t1, 2
    )");
    check_eq("sll 1<<4", cpu.regs[2], 16);
    check_eq("srl 64>>2", cpu.regs[3], 16);
}

void test_sum_loop() {
    // Sum 1..10 with a loop, classic textbook problem.
    auto cpu = run_program(R"(
        addi $t0, $zero, 0
        addi $t1, $zero, 0
        addi $t2, $zero, 10
loop:
        addi $t0, $t0, 1
        add  $t1, $t1, $t0
        bne  $t0, $t2, loop
        add  $v0, $t1, $zero
    )");
    check_eq("sum 1..10", cpu.regs[2], 55);
}

void test_lw_sw() {
    auto cpu = run_program(R"(
        addi $s0, $zero, 256
        addi $t0, $zero, 42
        sw   $t0, 0($s0)
        lw   $v0, 0($s0)
    )");
    check_eq("sw then lw", cpu.regs[2], 42);
}

void test_unconditional_jump() {
    auto cpu = run_program(R"(
        addi $v0, $zero, 1
        j    end
        addi $v0, $zero, 999
end:
    )");
    check_eq("j skips", cpu.regs[2], 1);
}

void test_beq() {
    auto cpu = run_program(R"(
        addi $t0, $zero, 3
        addi $t1, $zero, 3
        beq  $t0, $t1, hit
        addi $v0, $zero, 999
        j    end
hit:
        addi $v0, $zero, 1
end:
    )");
    check_eq("beq taken", cpu.regs[2], 1);
}

void test_zero_reg_stays_zero() {
    auto cpu = run_program(R"(
        addi $zero, $zero, 99
        add  $v0, $zero, $zero
    )");
    check_eq("$zero unchanged", cpu.regs[2], 0);
}

void test_invalid_register_name_throws() {
    ++g_total;
    try {
        mips::reg_number("$nope");
        std::cerr << "  ✗ invalid register did not throw\n";
        ++g_failures;
    } catch (const std::runtime_error &) {
        std::cout << "  ✓ invalid register throws\n";
    }
}

}  // namespace

int main() {
    std::cout << "running mips-emulator tests\n";
    test_addi_and_add();
    test_sub();
    test_and_or_xor_nor();
    test_slt();
    test_sll_srl();
    test_sum_loop();
    test_lw_sw();
    test_unconditional_jump();
    test_beq();
    test_zero_reg_stays_zero();
    test_invalid_register_name_throws();
    std::cout << "\n" << (g_total - g_failures) << "/" << g_total << " passed\n";
    return g_failures == 0 ? 0 : 1;
}

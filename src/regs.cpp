// regs.cpp — register name <-> number mapping.

#include "mips.h"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace mips {

static const std::unordered_map<std::string, int> kRegByName = {
    {"$zero", 0}, {"$0", 0},
    {"$at", 1},   {"$1", 1},
    {"$v0", 2},   {"$2", 2},
    {"$v1", 3},   {"$3", 3},
    {"$a0", 4},   {"$4", 4},
    {"$a1", 5},   {"$5", 5},
    {"$a2", 6},   {"$6", 6},
    {"$a3", 7},   {"$7", 7},
    {"$t0", 8},   {"$8", 8},
    {"$t1", 9},   {"$9", 9},
    {"$t2", 10},  {"$10", 10},
    {"$t3", 11},  {"$11", 11},
    {"$t4", 12},  {"$12", 12},
    {"$t5", 13},  {"$13", 13},
    {"$t6", 14},  {"$14", 14},
    {"$t7", 15},  {"$15", 15},
    {"$s0", 16},  {"$16", 16},
    {"$s1", 17},  {"$17", 17},
    {"$s2", 18},  {"$18", 18},
    {"$s3", 19},  {"$19", 19},
    {"$s4", 20},  {"$20", 20},
    {"$s5", 21},  {"$21", 21},
    {"$s6", 22},  {"$22", 22},
    {"$s7", 23},  {"$23", 23},
    {"$t8", 24},  {"$24", 24},
    {"$t9", 25},  {"$25", 25},
    {"$k0", 26},  {"$26", 26},
    {"$k1", 27},  {"$27", 27},
    {"$gp", 28},  {"$28", 28},
    {"$sp", 29},  {"$29", 29},
    {"$fp", 30},  {"$30", 30},
    {"$ra", 31},  {"$31", 31},
};

static const char *kCanonical[32] = {
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3",
    "$t0",   "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0",   "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8",   "$t9", "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
};

int reg_number(const std::string &name) {
    auto it = kRegByName.find(name);
    if (it == kRegByName.end()) {
        throw std::runtime_error("unknown register: " + name);
    }
    return it->second;
}

std::string reg_name(int n) {
    if (n < 0 || n >= 32) return "$??";
    return kCanonical[n];
}

}  // namespace mips

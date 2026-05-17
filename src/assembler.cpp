// assembler.cpp — convert MIPS assembly text into 32-bit instruction words.
//
// Two-pass design:
//   pass 1: collect labels and their byte addresses.
//   pass 2: emit instruction words, resolving labels for branches/jumps.
//
// Author: @forgehk.

#include "mips.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace mips {
namespace {

// ---- opcode/funct tables ---------------------------------------------------

struct RType { uint8_t funct; };
struct IType { uint8_t opcode; };
struct JType { uint8_t opcode; };

// R-type (opcode = 0)
const std::unordered_map<std::string, RType> kRType = {
    {"add", {0x20}}, {"sub", {0x22}}, {"and", {0x24}}, {"or",  {0x25}},
    {"xor", {0x26}}, {"nor", {0x27}}, {"slt", {0x2a}}, {"sll", {0x00}},
    {"srl", {0x02}}, {"jr",  {0x08}},
};

// I-type
const std::unordered_map<std::string, IType> kIType = {
    {"addi", {0x08}}, {"andi", {0x0c}}, {"ori",  {0x0d}}, {"xori", {0x0e}},
    {"slti", {0x0a}}, {"lw",   {0x23}}, {"sw",   {0x2b}}, {"lb",   {0x20}},
    {"sb",   {0x28}}, {"beq",  {0x04}}, {"bne",  {0x05}}, {"lui",  {0x0f}},
};

// J-type
const std::unordered_map<std::string, JType> kJType = {
    {"j",   {0x02}},
    {"jal", {0x03}},
};

// ---- tokenization ----------------------------------------------------------

std::string trim(std::string s) {
    auto issp = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && issp(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && issp(static_cast<unsigned char>(s.back())))  s.pop_back();
    return s;
}

std::string strip_comment(const std::string &s) {
    auto h = s.find('#');
    if (h == std::string::npos) return s;
    return s.substr(0, h);
}

std::vector<std::string> split_operands(const std::string &operand_str) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : operand_str) {
        if (c == ',' || std::isspace(static_cast<unsigned char>(c))) {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

int32_t parse_imm(const std::string &s) {
    if (s.empty()) throw std::runtime_error("expected immediate");
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return static_cast<int32_t>(std::stoul(s.substr(2), nullptr, 16));
    }
    return std::stoi(s);
}

// Parse "imm($reg)" addressing — returns (imm, regnum).
std::pair<int32_t, int> parse_mem(const std::string &s) {
    auto open = s.find('(');
    auto close = s.find(')');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        throw std::runtime_error("expected imm($reg), got: " + s);
    }
    std::string imm_part = s.substr(0, open);
    std::string reg_part = s.substr(open + 1, close - open - 1);
    int32_t imm = imm_part.empty() ? 0 : parse_imm(imm_part);
    return {imm, reg_number(reg_part)};
}

// ---- encoders --------------------------------------------------------------

Word enc_r(int rs, int rt, int rd, int shamt, int funct) {
    return (0u << 26) | ((rs & 0x1f) << 21) | ((rt & 0x1f) << 16) |
           ((rd & 0x1f) << 11) | ((shamt & 0x1f) << 6) | (funct & 0x3f);
}

Word enc_i(int opcode, int rs, int rt, int16_t imm) {
    return ((opcode & 0x3f) << 26) | ((rs & 0x1f) << 21) | ((rt & 0x1f) << 16) |
           (static_cast<uint16_t>(imm) & 0xffff);
}

Word enc_j(int opcode, uint32_t target_word_addr) {
    return ((opcode & 0x3f) << 26) | (target_word_addr & 0x03ffffff);
}

}  // namespace

// ----------------------------------------------------------------------------

std::vector<Word> assemble(const std::vector<std::string> &lines) {
    // PASS 1: collect labels and produce a clean list of non-empty lines.
    std::unordered_map<std::string, uint32_t> labels;
    struct Cleaned { std::string mnemonic; std::vector<std::string> ops; uint32_t addr; };
    std::vector<Cleaned> program;
    uint32_t addr = TEXT_BASE;

    for (const auto &raw : lines) {
        std::string s = trim(strip_comment(raw));
        if (s.empty()) continue;

        // Pull off any leading "label:" tags.
        while (true) {
            auto colon = s.find(':');
            if (colon == std::string::npos) break;
            std::string lbl = trim(s.substr(0, colon));
            if (lbl.empty()) break;
            labels[lbl] = addr;
            s = trim(s.substr(colon + 1));
        }
        if (s.empty()) continue;

        // mnemonic + rest
        std::istringstream iss(s);
        std::string mnemonic;
        iss >> mnemonic;
        std::string rest;
        std::getline(iss, rest);
        rest = trim(rest);

        auto ops = split_operands(rest);
        program.push_back({mnemonic, ops, addr});
        addr += 4;
    }

    // PASS 2: encode.
    std::vector<Word> out;
    out.reserve(program.size());
    for (const auto &line : program) {
        const std::string &m = line.mnemonic;

        if (auto it = kRType.find(m); it != kRType.end()) {
            uint8_t funct = it->second.funct;
            if (m == "sll" || m == "srl") {
                // sll $rd, $rt, shamt
                if (line.ops.size() != 3)
                    throw std::runtime_error(m + ": need rd, rt, shamt");
                int rd = reg_number(line.ops[0]);
                int rt = reg_number(line.ops[1]);
                int shamt = parse_imm(line.ops[2]);
                out.push_back(enc_r(0, rt, rd, shamt, funct));
            } else if (m == "jr") {
                if (line.ops.size() != 1) throw std::runtime_error("jr: need rs");
                int rs = reg_number(line.ops[0]);
                out.push_back(enc_r(rs, 0, 0, 0, funct));
            } else {
                if (line.ops.size() != 3)
                    throw std::runtime_error(m + ": need rd, rs, rt");
                int rd = reg_number(line.ops[0]);
                int rs = reg_number(line.ops[1]);
                int rt = reg_number(line.ops[2]);
                out.push_back(enc_r(rs, rt, rd, 0, funct));
            }
            continue;
        }

        if (auto it = kIType.find(m); it != kIType.end()) {
            uint8_t op = it->second.opcode;
            if (m == "lw" || m == "sw" || m == "lb" || m == "sb") {
                // lw $rt, imm($rs)
                if (line.ops.size() != 2)
                    throw std::runtime_error(m + ": need rt, imm($rs)");
                int rt = reg_number(line.ops[0]);
                auto [imm, rs] = parse_mem(line.ops[1]);
                out.push_back(enc_i(op, rs, rt, static_cast<int16_t>(imm)));
            } else if (m == "beq" || m == "bne") {
                // beq $rs, $rt, label
                if (line.ops.size() != 3)
                    throw std::runtime_error(m + ": need rs, rt, label");
                int rs = reg_number(line.ops[0]);
                int rt = reg_number(line.ops[1]);
                int32_t target;
                auto lit = labels.find(line.ops[2]);
                if (lit != labels.end()) {
                    int32_t off = (static_cast<int32_t>(lit->second) -
                                   (static_cast<int32_t>(line.addr) + 4)) / 4;
                    target = off;
                } else {
                    target = parse_imm(line.ops[2]);
                }
                out.push_back(enc_i(op, rs, rt, static_cast<int16_t>(target)));
            } else if (m == "lui") {
                if (line.ops.size() != 2)
                    throw std::runtime_error("lui: need rt, imm");
                int rt = reg_number(line.ops[0]);
                out.push_back(enc_i(op, 0, rt, static_cast<int16_t>(parse_imm(line.ops[1]))));
            } else {
                // addi/andi/ori/xori/slti: $rt, $rs, imm
                if (line.ops.size() != 3)
                    throw std::runtime_error(m + ": need rt, rs, imm");
                int rt = reg_number(line.ops[0]);
                int rs = reg_number(line.ops[1]);
                int16_t imm = static_cast<int16_t>(parse_imm(line.ops[2]));
                out.push_back(enc_i(op, rs, rt, imm));
            }
            continue;
        }

        if (auto it = kJType.find(m); it != kJType.end()) {
            uint8_t op = it->second.opcode;
            if (line.ops.size() != 1)
                throw std::runtime_error(m + ": need target");
            uint32_t target_addr;
            auto lit = labels.find(line.ops[0]);
            if (lit != labels.end()) target_addr = lit->second;
            else                    target_addr = static_cast<uint32_t>(parse_imm(line.ops[0]));
            out.push_back(enc_j(op, target_addr >> 2));
            continue;
        }

        if (m == "nop") {
            out.push_back(0);  // sll $zero, $zero, 0
            continue;
        }

        throw std::runtime_error("unknown mnemonic: " + m);
    }
    return out;
}

std::vector<Word> assemble(const std::string &source) {
    std::vector<std::string> lines;
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return assemble(lines);
}

}  // namespace mips

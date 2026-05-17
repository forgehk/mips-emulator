# mips-emulator

> A small MIPS-32 instruction emulator in modern C++. Parses MIPS assembly, decodes R/I/J-type instructions, and executes them against a register file and 64KB of byte-addressable memory.

[![C++](https://img.shields.io/badge/C++-17-blue.svg)]() [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## What it does

You hand it a program in MIPS assembly, it runs:

```asm
# examples/sum_first_10.s
# Sum the integers 1..10 into $v0.

addi $t0, $zero, 0      # i = 0
addi $t1, $zero, 0      # sum = 0
addi $t2, $zero, 10     # limit = 10

loop:
addi $t0, $t0, 1        # i++
add  $t1, $t1, $t0      # sum += i
bne  $t0, $t2, loop     # while i != limit

add  $v0, $t1, $zero    # return sum in $v0
```

```bash
$ ./mips run examples/sum_first_10.s
program halted after 32 cycles
$v0 = 55
```

Built as a self-study of MIPS architecture and single-cycle datapath design.

---

## Supported instructions

The instruction set is the textbook MIPS subset most courses cover:

| Type | Instructions |
|---|---|
| **R-type** | `add`, `sub`, `and`, `or`, `xor`, `nor`, `slt`, `sll`, `srl`, `jr` |
| **I-type** | `addi`, `andi`, `ori`, `xori`, `slti`, `lw`, `sw`, `lb`, `sb`, `beq`, `bne`, `lui` |
| **J-type** | `j`, `jal` |

Plus pseudo-instructions:
- `li $rt, imm` → `addi $rt, $zero, imm` (small) or `lui` + `ori` (large)
- `move $rd, $rs` → `add $rd, $rs, $zero`
- `nop` → `sll $zero, $zero, 0`

---

## Architecture

```
┌──────────────────────────────┐
│       MIPS assembly text     │
└─────────────┬────────────────┘
              ▼
┌──────────────────────────────┐
│        assembler             │  → resolves labels, emits 32-bit instructions
└─────────────┬────────────────┘
              ▼
┌──────────────────────────────┐
│        CPU + memory          │  → 32 registers, 64KB memory, PC, instruction
│                              │     register; fetch / decode / execute loop
└──────────────────────────────┘
```

The emulator is a single-cycle reference implementation — no pipelining, no caching, no MMU. Clean and pedagogical. Easy to extend if you want to add a 5-stage pipeline or branch prediction as a follow-on assignment.

---

## Build & run

```bash
git clone https://github.com/forgehk/mips-emulator
cd mips-emulator
make
./mips run examples/sum_first_10.s

# trace mode: print PC, instruction, and registers each cycle
./mips run --trace examples/sum_first_10.s

# dump memory after running
./mips run examples/memory_demo.s --dump 0x100,0x120
```

Run the test suite:

```bash
make test
```

---

## Register reference

| Name | Number | Convention |
|---|---|---|
| `$zero` | 0 | Always zero |
| `$at` | 1 | Assembler temporary |
| `$v0`-`$v1` | 2-3 | Return values |
| `$a0`-`$a3` | 4-7 | Arguments |
| `$t0`-`$t7` | 8-15 | Temporaries (caller-saved) |
| `$s0`-`$s7` | 16-23 | Saved (callee-saved) |
| `$t8`-`$t9` | 24-25 | Temporaries |
| `$k0`-`$k1` | 26-27 | Kernel reserved |
| `$gp` | 28 | Global pointer |
| `$sp` | 29 | Stack pointer |
| `$fp` | 30 | Frame pointer |
| `$ra` | 31 | Return address |

---

## Example programs

The `examples/` directory has runnable programs covering:

- **sum_first_10.s** — for-loop summation
- **factorial.s** — recursive `jal` / `jr $ra` factorial
- **memory_demo.s** — `lw` / `sw` to stack and heap regions
- **branch_demo.s** — `beq`, `bne`, `slt` based control flow
- **shift_logic.s** — `sll`, `srl`, `and`, `or`, `xor`, `nor`

---

## Why I built this

Building a working emulator is the best way to internalize what each instruction *actually* does at the hardware level. This is that emulator, kept minimal and pedagogical.

It's also a useful interview artifact because it demonstrates:

- Real systems-level C++ (memory model, bit manipulation, instruction decoding).
- A small but complete compiler/runtime pipeline (text → assembled binary → executed result).
- Discipline around register conventions and ABI rules.

---

## Roadmap

- [x] R/I/J-type decode and execute
- [x] Label resolution in the assembler
- [x] Trace + memory dump modes
- [x] Test suite covering each instruction
- [ ] 5-stage pipeline simulator (separate executable)
- [ ] Cache model with hit/miss stats
- [ ] Floating-point coprocessor (cop1) — `add.s`, `mul.s`, etc.
- [ ] Linker for multi-file programs

---

## License

[MIT](LICENSE)

---

*Built by [@forgehk](https://github.com/forgehk) — [DarkForge AI](https://darkforgeai.com)*

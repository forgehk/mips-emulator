# Store a value in memory and read it back.
# Demonstrates lw / sw with a base register.

addi $s0, $zero, 256    # base = 0x100
addi $t0, $zero, 42     # value to store

sw   $t0, 0($s0)        # mem[256] = 42
lw   $v0, 0($s0)        # load it back into $v0

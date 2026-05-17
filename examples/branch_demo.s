# Demonstrate beq / bne / slt branching.
# Computes max($a0, $a1) into $v0.

addi $a0, $zero, 17     # arg0 = 17
addi $a1, $zero, 42     # arg1 = 42

slt  $t0, $a0, $a1      # $t0 = ($a0 < $a1) ? 1 : 0
beq  $t0, $zero, a0_bigger

add  $v0, $a1, $zero    # $v0 = $a1
j    done

a0_bigger:
add  $v0, $a0, $zero    # $v0 = $a0

done:

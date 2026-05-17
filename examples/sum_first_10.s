# Sum the integers 1..10 into $v0.
# Expected: $v0 = 55.

addi $t0, $zero, 0      # i = 0
addi $t1, $zero, 0      # sum = 0
addi $t2, $zero, 10     # limit = 10

loop:
addi $t0, $t0, 1        # i++
add  $t1, $t1, $t0      # sum += i
bne  $t0, $t2, loop     # while i != limit

add  $v0, $t1, $zero    # return sum in $v0

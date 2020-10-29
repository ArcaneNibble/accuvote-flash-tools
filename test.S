.section .crt0, "ax"
.syntax unified
.arm

.global _start
_start:
    movs sp, 0xa4000000
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    movs r2, 0
1:
    cmp r0, r1
    beq 1f
    stmia r0!, {r2}
    b 1b
1:
    ldr r0, =entry
    bx r0

.pool

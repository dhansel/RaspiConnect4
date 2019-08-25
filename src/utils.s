
;@ Wait for at least r0 cycles
.global busywait
busywait:
        push {r1}
        mov r0, r0, ASR #1
bloop:  ;@ eor r1, r0, r1
        subs r0,r0,#1
        bne bloop
        
        ;@ quit
        pop {r1}
        bx lr   ;@ Return 



;@ strlen
.global strlen
strlen:
    push {r1,r2}
    mov r1, r0 
    mov r0, #0
1:
    ldrb r2, [r1]
    cmp r2,#0
    beq 2f
    add r0, #1
    add r1, #1
    b 1b
2:
    pop {r1,r2}
    bx lr


;@  busy-wait a fixed amount of time in us.
;@ r0: number of micro-seconds to sleep
;@
.global usleep
usleep:
    push {r1,r2,r3}

    mov r1, #0x2000
    lsl r1, #16
    orr r1, #0x3000
    ;@ now r1 = 0x20003000 (system timer address)

    add r3, r1, #0x4   ;@ (r3 = system timer lower 32 bits)
    ldr r2, [r3]

    add r0, r0, r2

1:
    ldr r1, [r3]
    cmp r1, r0
    blt 1b  ;@ busy loop
    

    pop {r1,r2,r3}
    bx lr


.global time_microsec
time_microsec:
    push {r1,r3}

    mov r1, #0x2000
    lsl r1, #16
    orr r1, #0x3000
    ;@ now r1 = 0x20003000 (system timer address)

    add r3, r1, #0x4   ;@ (r3 = system timer lower 32 bits)
    ldr r0, [r3]
    pop {r1,r3}
    bx lr
    

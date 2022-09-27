@ ninth/native.s
@ Copyright (C) 2022 J. M. Spivey

	.syntax unified
        .thumb

        @@ r0: stack pointer SP
        @@ r1: accumulator ACC
        @@ r2: scratch, or current token
        @@ r3: current definition
        @@ r4: instruction pointer IP
        @@ r5: pointer for r-stack RP
        @@ r6: stack base
        @@ r7: base of mem array

@@@ r0 always points to an empty stack slot whose current content is
@@@ cached in r1: so to push a new item, we must first 'flush the
@@@ cache' by storing this cached item in r0; then decrement r0 by 4,
@@@ and set r1 to the new item.

@@@ Macros for common actions
        
        .macro action lab
        .global \lab
        .thumb_func
\lab:    
        .endm
	
	.macro spush
        @@ Push the stack
        str r1, [r0]
        subs r0, #4
        @@ Now set r1 to the new top item
	.endm

	.macro spop
        @@ Pop the stack
        adds r0, #4
        ldr r1, [r0]
        .endm
        
	.macro getarg reg
        @@ Fetch an argument following the current token
	ldrh \reg, [r4]
        adds r4, #2
        .endm

        
@@@ Main entry point

        .global run
        .thumb_func
run:
        push {r3-r7, lr}
	sub sp, #8
        str r0, [sp, #0]        @ Save token for main

        ldr r0, =sbase          @ sbase in r6
        ldr r6, [r0]
        ldr r7, =mem            @ mem in r7
        
	action A_QUIT
quit:
	@@ (Re)start the main program
	movs r4, #0
        ldr r0, =rbase          @ RP in r5, initially rbase
        ldr r5, [r0]
	mov r0, r6              @ SP in r0, initially sbase

        ldr r2, [sp, #0]        @ Get MAIN
	b reswitch
        .pool

        action A_NOP
next:
        @@ Execute next token
        cmp r0, r6              @ Check for stack underflow
        bls 1f
        bl underflow
        b quit
1:
        ldrh r2, [r4]           @ Get next token from IP
	adds r4, #2

reswitch: 
	@@ Current token in r2
        lsls r2, #2             @ Get definition in r3
        ldr r3, [r7, r2]

        ldr r2, =tracing
	ldr r2, [r2]
        cmp r2, #0
        beq 2f
        push {r0-r3}
	str r1, [r0]
        mov r1, r5
        mov r2, r3
        bl trace
        pop {r0-r3}
2:      

        ldr r2, [r3, #4]        @ Get action routine
        bx r2                   @ and branch to it

        action A_ENTER
	@@ Call a defined word
        subs r5, #4             @ Save return address
        str r4, [r5]
        ldr r4, [r3, #8]        @ Load IP with address of body
        b next

	action A_EXIT
        @@ Return from a defined word
	ldr r4, [r5]            @ Pop return address from r-stack
        adds r5, #4
        b next

        action A_EXECUTE
	@@ Interpret a token from the stack
	movs r2, r1             @ Pop token into r2
	spop
	b reswitch              @ Go to decode it

        action A_CALL
        @@ Call a native-code primitive as SP = f(SP)
        str r1, [r0]            @ Flush the ACC
        ldr r2, [r3, #8]        @ Get the function address
        blx r2                  @ Call it
        ldr r1, [r0]            @ Fill the ACC again
        b next

        action A_CONST
	action A_VAR
	@@ Push a constant or variable address
	spush
        ldr r1, [r3, #8]        @ Use data field as value
        b next

	action A_BRANCH0
	@@ Conditional branch if stack top is zero
	getarg r2               @ Displacement from following arg
        cmp r1, #0              @ If the stack top is nonzero
        bne 1f
	sxth r2, r2             @ Sign extend the displacement
	lsls r2, #1             @ Multiply by 2
        adds r4, r2             @ Add to the IP
1:
	spop                    @ Pop the test value
        b next
        
	action A_BRANCH
	@@ Unconditional branch
	getarg r2               @ Get following displacement
	sxth r2, r2             @ Sign extend
        lsls r2, #1             @ Multiply by 2
        adds r4, r2             @ Add to RP
        b next

	action A_LIT
	@@ 16-bit literal
        spush                   @ Push on stack
	getarg r1               @ Get following argument in ACC
	sxth r1, r1             @ Sign extend
        b next

        action A_LIT2
	@@ 32-bit literal in two halves
        spush                   @ Push on stack
        ldrh r1, [r4]           @ Least significant bits
        ldrh r2, [r4, #2]       @ Most significant bits
        adds r4, #4             @ Advance IP
        lsls r2, #16            @ Shift MSB
        orr r1, r2              @ Combine into ACC
        b next

	action A_UNKNOWN
	@@ A word that has not been defined:
        @@ push its name and invoke 'unknown'
        spush
        add r1, r3, #12         @ Fetch the name
        ldr r2, =UNKNOWN        @ Get token for UNKNOWN
        ldr r2, [r2]            
	b reswitch              @ Go to dispatch
        .pool

	action A_DONE
	@@ Exit the interpreter
	add sp, #8
        pop {r3-r7, pc}


@@@ Macros for action groups

        .macro const lab, val
        @@ Complete action for a fixed constant
	action \lab
        spush                   @ Push the stack
        movs r1, #\val          @ Set ACC to the value
        bl next
        .endm

        const A_ZERO, 0
        const A_ONE, 1
        const A_TWO, 2
        const A_THREE, 3
        const A_FOUR, 4

	.macro binary lab, op
        @@ Action for a binary operation
	action \lab
        adds r0, #4             @ Prepare to pop
        ldr r2, [r0]            @ Fetch first operand
        \op r1, r2, r1          @ Combine with ACC
        @@ The assembler exploits commutativity for us when possible
        bl next
	.endm

	binary A_ADD, adds
        binary A_SUB, subs
        binary A_MUL, muls
	binary A_AND, ands
        binary A_OR, orrs
        binary A_XOR, eors

	.macro shift lab, op
        @@ Action for a shift instruction
        action \lab
        movs r2, r1             @ Put shift amount in r2
        spop                    @ Pop stack into ACC
        \op r1, r2              @ Shift the ACC
        bl next
        .endm

        shift A_LSL, lsls
	shift A_ASR, asrs
        shift A_LSR, lsrs

        .macro compare lab, op
        @@ Action for a comparison with Boolean result
        action \lab
	adds r0, #4             @ Prepare to pop the stack
        ldr r2, [r0]            @ Fetch the first operand
        movs r3, #1             @ Set tentative result
        cmp r2, r1              @ Compare operands
        \op 1f                  @ Branch if true
        movs r3, #0             @ Reset result to false
1:
        movs r1, r3             @ Put result in ACC
        bl next
        .endm

	compare A_EQ, beq
        compare A_LESS, blt
	compare A_ULESS, blo    @ Unsigned < comparison

	.macro get lab, op
        @@ A load operation, replacing address by contents
        action \lab
        \op r1, [r1]            @ Load ACC from address in ACC
        bl next
        .endm

        get A_GET, ldr
        get A_CHGET, ldrb

        action A_TOKGET
        @@ Get a 16-bit signed token
        ldrh r1, [r1]           @ Get 16-bit value
        sxth r1, r1             @ Sign extend
        bl next

	.macro put lab, op
        @@ A store operation, saving value into address
        action \lab
	ldr r2, [r0, #4]        @ Get value in r2
        \op r2, [r1]            @ Store to address in ACC
        adds r0, #8             @ Pop two values
        ldr r1, [r0]            @ Refill ACC
        bl next
        .endm

        put A_PUT, str
        put A_CHPUT, strb
        put A_TOKPUT, strh

	action A_INC
        @@ Increment the stack top
        adds r1, r1, #1         @ Incerement ACC
        bl next

        action A_DEC
        @@ Decrement the stack top
        subs r1, r1, #1         @ Decrement ACC
        bl next
        
	action A_DUP
	@@ Duplicate the top item
        str r1, [r0]            @ Save ACC as second value
        subs r0, #4             @ Push the stack
        bl next
        
        action A_QDUP
        @@ Duplicate if non-zero
        cmp r1, #0              @ Test ACC
        beq 1f                  @ If non-zero
        str r1, [r0]            @ Save ACC as second value
        subs r0, #4             @ Push the stack
1:
        bl next

	action A_OVER
	@@ Copy the second item on the stack
	spush                   @ Push the stack
        ldr r1, [r0, #8]        @ Set ACC to prev. second item
        bl next

        action A_PICK
	@@ Copy the item at a specified index
        adds r1, #1             @ Fix up indexing
        lsls r1, #2             @ Multiply index by 4
        ldr r1, [r0, r1]        @ Replace index with value in ACC
        bl next

        action A_ROT
        @@ Pull up the third item on the stack
        ldr r2, [r0, #4]        @ Save second item as temp
        str r1, [r0, #4]        @ Replace it with ACC
        ldr r1, [r0, #8]        @ Set ACC to third item
        str r2, [r0, #8]        @ Store temp in place of third
        bl next

	action A_POP
        @@ Pop the stack
	spop
        bl next

	action A_SWAP
	@@ Swap the top two items
	ldr r2, [r0, #4]        @ Save second item as temp
        str r1, [r0, #4]        @ Replace with ACC
        movs r1, r2             @ Move temp into ACC
        bl next

        action A_TUCK
	@@ Insert a copy of the top item below the second
        ldr r2, [r0, #4]        @ Save second item as temp
        str r1, [r0, #4]        @ Replace with ACC
        str r2, [r0]            @ Store temp as new second item
        subs r0, #4             @ Push the stack
        bl next
        
	action A_NIP
        @@ Squeeze out the second item
        adds r0, #4             @ Top item remains in ACC
        bl next

	action A_RPOP
	@@ Pop the r-stack onto the stack
	spush                   @ Push the stack
        ldr r1, [r5]            @ Load ACC from RP
        adds r5, #4             @ Increment RP
        bl next

	action A_RPUSH
	@@ Push the top item onto the r-stack
        subs r5, #4             @ Push the r-stack
        str r1, [r5]            @ Save ACC in r-stack
	spop                    @ Pop the stack
        bl next

	action A_RP
        @@ Fetch the value of rp
        spush                   @ Push the stack
        mov r1, r5              @ Set ACC to RP
        bl next
        
	action A_LOCALS
        @@ Allocate space for n locals
        getarg r2               @ Get n
        cmp r2, #0              @ Now transfer n items from stack to r-stack
        beq 2f
1:
	subs r5, #4             @ Push the r-stack
        str r1, [r5]            @ Store ACC as new top
	spop                    @ Pop the stack
        subs r2, #1             @ Decrement count
        bne 1b                  @ Repeat of non-zero
2:
        bl next

        action A_GETLOC
	@@ Fetch a local variable onto the stack
	getarg r2               @ Fetch following argument
	spush                   @ Push the stack
        lsls r2, #2             @ Multiply index by 4
        ldr r1, [r5, r2]        @ Load from the r-stack
        bl next

        action A_SETLOC
	@@ Set a local variable from the stack
	getarg r2               @ Fetch following argument
        lsls r2, #2             @ Multiply by 4
        str r1, [r5, r2]        @ Store ACC into r-stack
	spop                    @ Pop the stack
        bl next
        
        action A_POPLOCS
	@@ Pop n locals from the r-stack
	getarg r2               @ Fetch following argument
        lsls r2, #2             @ Multiply by 4
        adds r5, r2             @ Add to RP
        bl next

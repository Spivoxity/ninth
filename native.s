# native.s

	.syntax unified
        .thumb

        @@ r0: stack pointer
        @@ r1: accumulator
        @@ r2: scratch
        @@ r3: current definition
        @@ r4: instruction pointer
        @@ r5: pointer for r-stack
        @@ r6: stack base
        @@ r7: base of mem array

@@@ r0 always points to an empty stack slot whose current content is
@@@ cached in r1: so to push a new item, we must first 'flush the
@@@ cache' by storing this cached item in r0; then decrement r0 by 4,
@@@ and set r1 to the new item.

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
        ldr r0, =rbase          @ rp in r5
        ldr r5, [r0]
	movs r0, r6             @ sp in r6, initially sbase

        ldr r2, [sp, #0]        @ Get main
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
        ldrh r2, [r4]           @ Get next token
	adds r4, #2

reswitch: 
	@@ Token in r2
        lsls r2, #2             @ Get definition in r3
        ldr r3, [r7, r2]
        ldr r2, [r3, #4]        @ Get action routine
        bx r2                   @ and branch to it

        action A_ENTER
	@@ Call a defined word
        subs r5, #4             @ Save return address
        str r4, [r5]
        ldr r4, [r3, #8]        @ Get address of body
        b next

	action A_EXIT
        @@ Return from a defined word
	ldr r4, [r5]            @ Pop return address from r-stack
        adds r5, #4
        b next

        action A_EXECUTE
	@@ Interpret a token from the stack
	movs r2, r1
	spop
	b reswitch

        action A_CALL
        @@ Call a native-code primitive as sp = f(sp)
        str r1, [r0]            @ Flush the cache
        ldr r2, [r3, #8]        @ Get the function address
        blx r2                  @ Call it
        ldr r1, [r0]            @ Fill the cache again
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
        cmp r1, #0              @ If the stack top is nonzer
        bne 1f
	sxth r2, r2             @ Sign extend the displacement
	lsls r2, #1             @ Multiply by 2
        adds r4, r2             @ Add to the ip
1:
	spop                    @ Pop the test value
        b next
        
	action A_BRANCH
	@@ Unconditional branch
	getarg r2
	sxth r2, r2
        lsls r2, #1
        adds r4, r2
        b next

	action A_LIT
	@@ 16-bit literal
        spush
	getarg r1
	sxth r1, r1
        b next

        action A_LIT2
	@@ 32-bit literal in two halves
        spush
        ldrh r1, [r4]
        ldrh r2, [r4, #2]
        adds r4, #4
        lsls r2, #16
        adds r1, r2
        b next

	action A_UNKNOWN
	@@ A word that has not been defined:
        @@ push its name and invoke 'unknown'
        spush
        add r1, r3, #12         @ Fetch the name
        ldr r2, =UNKNOWN        @ Get token for UNKNOWN
        ldr r2, [r2]            
	b reswitch
        .pool

	action A_DONE
	@@ Exit the interpreter
	add sp, #8
        pop {r3-r7, pc}         


        .macro const lab, val
        @@ Complete action for a fixed constant
	action \lab
        spush
        movs r1, #\val
        bl next
        .endm

	.macro binary lab, op
        @@ Action for a binary operation
	action \lab
        adds r0, #4
        ldr r2, [r0]
        \op r1, r2, r1
        @@ The assembler exploits commutativity for us when possible
        bl next
	.endm

	.macro shift lab, op
        @@ Action for a shift instruction
        action \lab
        movs r2, r1
        spop
        \op r1, r2
        bl next
        .endm

        .macro compare lab, op
        @@ Action for a comparison with Boolean result
        action \lab
	adds r0, #4
        ldr r2, [r0]
        movs r3, #1
        cmp r2, r1
        \op 1f
        movs r3, #0
1:
        movs r1, r3
        bl next
        .endm

	.macro get lab, op
        @@ A load operation, replacing address by contents
        action \lab
        \op r1, [r1]
        bl next
        .endm

	.macro put lab, op
        @@ A store operation, saving value into address
        action \lab
	ldr r2, [r0, #4]
        \op r2, [r1]
        adds r0, #8
        ldr r1, [r0]
        bl next
        .endm


        const A_ZERO, 0
        const A_ONE, 1
        const A_TWO, 2
        const A_THREE, 3
        const A_FOUR, 4
	binary A_ADD, adds
        binary A_SUB, subs
        binary A_MUL, muls
	compare A_EQ, beq
        compare A_LESS, blt
	compare A_ULESS, blo    @ Unsigned < comparison
	binary A_AND, ands
        binary A_OR, orrs
        binary A_XOR, eors
        shift A_LSL, lsls
	shift A_ASR, asrs
        shift A_LSR, lsrs
        get A_GET, ldr
        get A_CHGET, ldrb
        put A_PUT, str
        put A_CHPUT, strb
        put A_TOKPUT, strh

        action A_TOKGET
        @@ Get a 16-bit signed token
        ldrh r1, [r1]
        sxth r1, r1
        bl next

	action A_INC
        @@ Increment the stack top
        adds r1, r1, #1
        bl next

        action A_DEC
        @@ Decrement the stack top
        subs r1, r1, #1
        bl next
        
	action A_DUP
	@@ Duplicate the top item
        str r1, [r0]
        subs r0, #4
        bl next
        
        action A_QDUP
        @@ Duplicate if non-zero
        cmp r1, #0
        beq 1f
        str r1, [r0]
        subs r0, #4
1:
        bl next

	action A_OVER
	@@ Copy the second item on the stack
	spush
        ldr r1, [r0, #8]
        bl next

        action A_PICK
	@@ Copy the item at a specified index
        adds r1, #1
        lsls r1, #2
        ldr r1, [r0, r1]
        bl next

        action A_ROT
        @@ Pull up the third item on the stack
        ldr r2, [r0, #4]
        str r1, [r0, #4]
        ldr r1, [r0, #8]
        str r2, [r0, #8]
        bl next

	action A_POP
        @@ Pop the stack
	spop
        bl next

	action A_SWAP
	@@ Swap the top two items
	ldr r2, [r0, #4]
        str r1, [r0, #4]
        movs r1, r2
        bl next

        action A_TUCK
	@@ Insert a copy of the top item below the second
        ldr r2, [r0, #4]
        str r1, [r0, #4]
        str r2, [r0]
        subs r0, #4
        bl next
        
	action A_NIP
        @@ Squeeze out the second item
        adds r0, #4
        bl next

	action A_RPOP
	@@ Pop the r-stack onto the stack
	spush
        ldr r1, [r5]
        adds r5, #4
        bl next

	action A_RPUSH
	@@ Push the top item onto the r-stack
        subs r5, #4
        str r1, [r5]
	spop
        bl next

        action A_RAT
	@@ Fetch the top of the r-stack
        spush
        ldr r1, [r5]
        bl next

	action A_LOCALS
        @@ Allocate space for n locals
        getarg r2               @ Get n
        cmp r2, #0              @ Now transfer n items from stack to r-stack
        beq 2f
1:
	subs r5, #4
        str r1, [r5]
	spop
        subs r2, #1
        bne 1b
2:
        bl next

        action A_GETLOC
	@@ Fetch a local variable onto the stack
	getarg r2
	spush
        lsls r2, #2
        ldr r1, [r5, r2]
        bl next

        action A_SETLOC
	@@ Set a local variable from the stack
	getarg r2
        lsls r2, #2
        str r1, [r5, r2]
	spop
        bl next
        
        action A_POPLOCS
	@@ Pop n locals from the r-stack
	getarg r2
        lsls r2, #2
        adds r5, r2
        bl next

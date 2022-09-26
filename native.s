# native.s

	.syntax unified
        .thumb

        @@ r0: stack pointer
        @@ r1: accumulator
        @@ r2: scratch
        @@ r3: current definition
        @@ r4: instruction pointer
        @@ r5: pointer for rstack
        @@ r6: stack base
        @@ r7: base of mem array

        .global run
        .thumb_func
run:
        push {r3-r7, lr}
	sub sp, #8
        str r0, [sp, #0]        @ Save token for main

        ldr r0, =sbase          @ sbase in r6
        ldr r6, [r0]
        ldr r7, =mem            @ mem in r7
        
        .global A_QUIT
        .thumb_func
A_QUIT:
quit:
	movs r4, #0
        ldr r0, =rbase          @ rp in r5
        ldr r5, [r0]
	movs r0, r6             @ sp in r6, initially sbase

        ldr r2, [sp, #0]        @ Get main
	b reswitch
        .pool

        .global A_NOP
        .thumb_func
A_NOP:
	.thumb_func
next:
        @@ Underflow check
        cmp r0, r6
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

	push {r0-r3}
	str r1, [r0]
        movs r1, r5
	movs r2, r3
        @bl trace                
        pop {r0-r3}

        ldr r2, [r3, #4]        @ Get action routine
        bx r2                   @ and branch to it

        .global A_ENTER
        .thumb_func
A_ENTER:
	@@ Call a defined word
        subs r5, #4             @ Save return address
        str r4, [r5]
        ldr r4, [r3, #8]        @ Get address of body
        b next

	.global A_EXIT
        .thumb_func
A_EXIT: 
        @@ Return from a defined word
	ldr r4, [r5]            @ Pop return address from rstack
        adds r5, #4
        b next

        .global A_EXECUTE
        .thumb_func
A_EXECUTE:
	@@ Interpret a token from the stack
	movs r2, r1
        adds r0, #4
        ldr r1, [r0]
	b reswitch

        .global A_CALL
        .thumb_func
A_CALL:
        @@ Call a native-code primitive
        str r1, [r0]
        ldr r2, [r3, #8]
        blx r2
        ldr r1, [r0]
        b next

        .global A_CONST
        .thumb_func
A_CONST:
	.global A_VAR
	.thumb_func
A_VAR:  
	@@ Push a constant or vatiable address
        str r1, [r0]
        subs r0, #4
        ldr r1, [r3, #8]        @ Use data field as value
        b next

	.global A_UNKNOWN
        .thumb_func
A_UNKNOWN:
	@@ A word that has not been defined
        str r1, [r0]            @ Save acc
        subs r0, #4
        add r1, r3, #12         @ Fetch the name
        ldr r2, =UNKNOWN        @ Get token for UNKNOWN
        ldr r2, [r2]            
	b reswitch
        .pool

	.global A_DONE
        .thumb_func
A_DONE:
	add sp, #8              @ Return from run()
        pop {r3-r7, pc}         


	.macro action lab
        .global \lab
        .thumb_func
\lab:    
        .endm
        
	.macro spush
        str r1, [r0]
        subs r0, #4
	.endm

	.macro spop
        adds r0, #4
        ldr r1, [r0]
        .endm
        
	.macro rpush reg
        .endm

	.macro getarg reg
	ldrh \reg, [r4]
        adds r4, #2
        .endm

        .macro const lab, val
	action \lab
        spush
        movs r1, #\val
        bl next
        .endm

	.macro binary lab, op
	action \lab
        adds r0, #4
        ldr r2, [r0]
        \op r1, r2, r1
        bl next
	.endm

	.macro shift lab, op
        action \lab
        movs r2, r1
        spop
        \op r1, r2
        bl next
        .endm

        .macro compare lab, op
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
        action \lab
        \op r1, [r1]
        bl next
        .endm

	.macro put lab, op
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

	action A_INC
        adds r1, r1, #1
        bl next

        action A_DEC
        subs r1, r1, #1
        bl next
        
	compare A_EQ, beq
        compare A_LESS, blt
	compare A_ULESS, blo
        
	binary A_AND, ands
        shift A_LSL, lsls
	shift A_ASR, asrs
        shift A_LSR, lsrs
        binary A_OR, orrs
        binary A_XOR, eors

        get A_GET, ldr
        get A_CHGET, ldrb

        action A_TOKGET
        ldrh r1, [r1]
        sxth r1, r1
        bl next

        put A_PUT, str
        put A_CHPUT, strb
        put A_TOKPUT, strh

	action A_DUP
        str r1, [r0]
        subs r0, #4
        bl next
        
        action A_QDUP
        cmp r1, #0
        beq 1f
        str r1, [r0]
        subs r0, #4
1:
        bl next

	action A_OVER
        str r1, [r0]
        subs r0, #4
        ldr r1, [r0, #8]
        bl next

        action A_PICK
        adds r1, #1
        lsls r1, #2
        ldr r1, [r0, r1]
        bl next

        action A_ROT
        ldr r2, [r0, #4]
        str r1, [r0, #4]
        ldr r1, [r0, #8]
        str r2, [r0, #8]
        bl next

	action A_POP
	spop
        bl next

	action A_SWAP
	ldr r2, [r0, #4]
        str r1, [r0, #4]
        movs r1, r2
        bl next

        action A_TUCK
        ldr r2, [r0, #4]
        str r1, [r0, #4]
        str r2, [r0]
        subs r0, #4
        bl next
        
	action A_NIP
        adds r0, #4
        bl next

	action A_RPOP
	spush
        ldr r1, [r5]
        adds r5, #4
        bl next

	action A_RPUSH
        subs r5, #4
        str r1, [r5]
	spop
        bl next

        action A_RAT
        spush
        ldr r1, [r5]
        bl next

	action A_BRANCH0
	getarg r2
        cmp r1, #0
        bne 1f
	sxth r2, r2
	lsls r2, #1
        adds r4, r2
1:
	spop
        bl next
        
	action A_BRANCH
	getarg r2
	sxth r2, r2
        lsls r2, #1
        adds r4, r2
        bl next

	action A_LIT
        spush
	getarg r1
	sxth r1, r1
        bl next

        action A_LIT2
        spush
        ldrh r1, [r4]
        ldrh r2, [r4, #2]
        adds r4, #4
        lsls r2, #16
        adds r1, r2
        bl next

	action A_LOCALS
        @@ The stack contains (x1 x2 ... xn) with n at ip:
        @@ transfer (x1 x2 ... xn) to Rstack in reverse order.
        getarg r2
        cmp r2, #0
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
	getarg r2
	spush
        lsls r2, #2
        ldr r1, [r5, r2]
        bl next

        action A_SETLOC
	getarg r2
        lsls r2, #2
        str r1, [r5, r2]
	spop
        bl next
        
        action A_POPLOCS
	getarg r2
        lsls r2, #2
        adds r5, r2
        bl next

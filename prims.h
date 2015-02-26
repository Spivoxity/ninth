#define binary(s, p, w) prim(s, p, sp[1] = sp[1] w sp[0]; sp++)
#define get(s, p, t) prim(s, p, sp[0] = * (t *) sp[0])
#define put(s, p, t) prim(s, p, * (t *) sp[0] = sp[1]; sp += 2)

#define PRIMS \
     prim0(P_NOP, ) \
     prim("quit", P_QUIT, goto quit) \
     prim0(P_UNKNOWN, \
           printf("%s is unknown\n", def_name(w)); goto quit) \
     prim0(P_ENTER, \
           *--rp = (unsigned) ip; \
           ip = (ushort *) w->d_data) \
     prim("exit", P_EXIT, \
          if (rp >= (unsigned *) &rstack[RSTACK]) return; \
          ip = (ushort *) *rp++) \
     binary("+", P_ADD, +) \
     binary("-", P_SUB, -) \
     binary("*", P_MUL, *) \
     binary("/", P_DIV, /) \
     binary("mod", P_MOD, %) \
     binary("=", P_EQ, ==) \
     binary("<", P_LESS, <) \
     prim("u<", P_ULESS, sp[1] = (unsigned) sp[1] < (unsigned) sp[0]; sp++) \
     binary("and", P_AND, &) \
     binary("lsl", P_LSL, <<) \
     prim("lsr", P_LSR, sp[1] = (unsigned) sp[1] >> sp[0]; sp++) \
     binary("asr", P_ASR, >>) \
     binary("or", P_OR, |) \
     binary("xor", P_XOR, ^) \
     get("@", P_GET, int) \
     put("!", P_PUT, int) \
     get("ch@", P_CHGET, char) \
     put("ch!", P_CHPUT, char) \
     get("tok@", P_TOKGET, short) \
     put("tok!", P_TOKPUT, short) \
     prim("dup", P_DUP, sp--; sp[0] = sp[1]) \
     prim("pick", P_PICK, sp[0] = sp[sp[0]+1]) \
     prim("depth", P_DEPTH, t = (int *) &mem[MEMSIZE] - sp; *--sp = t) \
     prim("pop", P_POP, sp++) \
     prim("swap", P_SWAP, t = sp[0]; sp[0] = sp[1]; sp[1] = t) \
     prim("rot", P_ROT, t = sp[2]; sp[2] = sp[1]; sp[1] = sp[0]; sp[0] = t) \
     prim("r>", P_RPOP, *--sp = *rp++) \
     prim(">r", P_RPUSH, *--rp = *sp++) \
     prim("r@", P_RAT, *--sp = *rp) \
     prim("branch0", P_BRANCH0, t = (short) *ip++; if (*sp++ == 0) ip += t) \
     prim("branch", P_BRANCH, t = (short) *ip++; ip += t) \
     prim("lit", P_LIT, *--sp = (signed short) *ip++) \
     prim("lit2", P_LIT2, *--sp = (ip[1] << 16) + ip[0]; ip += 2) \
     prim("execute", P_EXECUTE, w = (def *) &mem[*sp++]; goto reswitch) \
     prim0(P_CALL, (* (void (*)(void)) w->d_data)()) \
     prim0(P_VAR, *--sp = (int) w->d_data) \
     prim0(P_CONST, *--sp = * (int *) w->d_data) \
     prim("gentok", P_GENTOK, * (short *) tp = *sp++; tp += sizeof(short)) \
     prim("immed?", P_IMMED, \
          sp[0] = ((((def *) &mem[sp[0]])->d_flags & IMMED) != 0)) \
     prim("align", P_ALIGN, dp = ALIGN(dp, 4))

#define prim(s, p, a) prim0(p, a)

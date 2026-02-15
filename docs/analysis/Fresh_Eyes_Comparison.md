# Fresh Eyes Comparison: smite.s vs smite_works.s

## Critical Differences Found

### 1. Tail Calls (jmp indirect)

**smite.s (BROKEN):**
```
18299:    aa36:	4ed1           	jmp (a1)
18321:    aa52:	4ed1           	jmp (a1)
```
- Has 2 direct `jmp (a1)` tail calls
- These are indirect tail calls that leak stack

**smite_works.s (WORKING):**
```
NO `jmp (a1)` found!
Only has `jmp (pc,offset)` which are computed jumps, not tail calls
```

### 2. int3Handler Stack Frame

**smite.s (BROKEN):**
```asm
int3Handler:
    b42:	598f           	subq.l #4,sp          ← ALLOCATES 4 BYTES ON STACK
    b44:	f227 e003      	fmovemx fp0-fp1,-(sp)
    b48:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)
    ...
    c16:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1
    c1a:	f21f d0c0      	fmovemx (sp)+,fp0-fp1
    c1e:	588f           	addq.l #4,sp          ← DEALLOCATES 4 BYTES
    c20:	4e73           	rte
```

**smite_works.s (WORKING):**
```asm
int3Handler:
    fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)  ← NO STACK ALLOCATION
    fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)
    ...
    fd2:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1
    fd6:	f21f d0c0      	fmovemx (sp)+,fp0-fp1
    fda:	4e73           	rte                    ← NO STACK DEALLOCATION
```

**KEY DIFFERENCE:** smite.s allocates 4 bytes on stack that smite_works.s doesn't!

### 3. int4Handler Stack Frame

**smite.s (BROKEN):**
```asm
int4Handler:
    c22:	598f           	subq.l #4,sp          ← ALLOCATES 4 BYTES
    c24:	f227 e003      	fmovemx fp0-fp1,-(sp)
    c28:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)
```

**smite_works.s (WORKING):**
```asm
int4Handler:
    eae:	f227 e003      	fmovemx fp0-fp1,-(sp)  ← NO STACK ALLOCATION
    eb2:	48e7 c0c0      	movem.l d0-d1/a0-a1,-(sp)  ← SAVES d0-d1, NOT d0-d2!
```

**KEY DIFFERENCES:**
1. smite.s allocates 4 bytes, smite_works.s doesn't
2. smite.s saves d0-d2, smite_works.s saves d0-d1 (no d2!)

### 4. logPushInt/logPopInt Calls

**smite.s (BROKEN):**
- NO logPushInt/logPopInt calls visible in int3Handler/int4Handler

**smite_works.s (WORKING):**
- HAS logPushInt/logPopInt calls:
```asm
int3Handler:
    faa:	3239 0001 97ac 	move.w 197ac <g_sLogManager.lto_priv.0+0x6>,d1  ← logPushInt
    ...
    fc2:	4a41           	tst.w d1                ← logPopInt check
```

## Root Cause Hypothesis

The **4-byte stack allocation** in smite.s interrupt handlers (`subq.l #4,sp`) combined with **tail calls** that leak stack space could be causing:

1. **Stack misalignment** - The 4-byte allocation changes stack alignment
2. **Stack frame corruption** - Tail calls leak stack, but the handler expects a specific frame size
3. **Exception frame corruption** - The 68020 RTE instruction expects a specific stack frame format

The working version:
- No stack allocation in interrupt handlers
- No tail calls
- Proper register saving (though int4Handler saves fewer registers!)

## Questions

1. Why does smite.s allocate 4 bytes on stack in interrupt handlers?
2. Why does smite_works.s int4Handler save d0-d1 instead of d0-d2?
3. What are those `jmp (a1)` tail calls in smite.s pointing to?


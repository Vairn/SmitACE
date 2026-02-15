# Stack Allocation Fix - The Real Issue

## Problem Found

**smite.s (BROKEN):**
```asm
int3Handler:
    b42:	598f           	subq.l #4,sp          ← ALLOCATES 4 BYTES
    b44:	f227 e003      	fmovemx fp0-fp1,-(sp)
    b48:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)
    ...
    b52:	3f68 001e 002e 	move.w 30(a0),46(sp)  ← STORES uwIntReq ON STACK
```

**smite_works.s (WORKING):**
```asm
int3Handler:
    fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)  ← NO STACK ALLOCATION
    fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)
    ...
    fb0:	3039 00df f01e 	move.w dff01e,d0      ← LOADS DIRECTLY INTO d0
```

## Root Cause

The compiler is storing `uwIntReq` on the stack instead of keeping it in a register. This causes:
1. **Stack allocation** (`subq.l #4,sp`) 
2. **Stack frame misalignment** - changes the stack frame size
3. **Vulnerability to stack corruption** - if tail calls leak stack, this location gets corrupted

## Fix Applied

Force `uwIntReq` to use register `d0`:

```c
register UWORD uwIntReq __asm("d0") = g_pCustom->intreqr;
```

This ensures:
- ✅ No stack allocation needed
- ✅ `uwIntReq` stays in register (protected from stack corruption)
- ✅ Matches working version behavior

## Why This Matters

The 4-byte stack allocation changes the interrupt handler's stack frame size. Combined with tail calls that leak stack space, this can cause:
- Exception stack frame corruption (68020 RTE validation fails)
- Stack misalignment
- Wrong stack offsets for saved registers

By eliminating the stack allocation, we match the working version's behavior exactly.


# Critical Structural Difference Found

## smite_works.s int3Handler (WORKING)

**Structure:**
```asm
int3Handler:
    Save registers (d0-d2/a0-a1, FP)
    logPushInt()
    Load uwIntReq into d0
    Check INTF_VERTB:
        If set: branch to fdc (timerOnInterrupt code - INLINED)
        timerOnInterrupt: increment counter, set uwReqClr=INTF_VERTB, branch back
    logPopInt() check
    Set intreq = uwReqClr
    Restore registers
    RTE
```

**Key points:**
- ✅ timerOnInterrupt is **INLINED** (no jsr call)
- ✅ Only handles INTF_VERTB (no COPER/BLIT in assembly!)
- ✅ Simple, linear code flow
- ✅ No function call overhead

## smite.s int3Handler (BROKEN)

**Structure:**
```asm
int3Handler:
    Save registers (d0-d3/a0-a1, FP)  ← Saves d3!
    Load uwIntReq into d0
    Check INTF_VERTB:
        If set: jsr timerOnInterrupt()  ← FUNCTION CALL!
        Check handler, call if exists
        Set uwReqClr = INTF_VERTB
    Check INTF_COPER:
        If set: Check handler, call if exists
        Set uwReqClr |= INTF_COPER
    Check INTF_BLIT:
        If set: Check handler, call if exists
        Set uwReqClr |= INTF_BLIT
    Set intreq = uwReqClr
    Restore registers
    RTE
```

**Key points:**
- ❌ timerOnInterrupt is **CALLED** with jsr (function call frame)
- ❌ Handles INTF_VERTB, INTF_COPER, INTF_BLIT
- ❌ Saves d3 unnecessarily
- ❌ More complex code with multiple function calls

## The Real Issue

**The working version doesn't have COPER/BLIT handling in the assembly!**

This could mean:
1. The working version's source code is different (simpler)
2. The compiler optimized away COPER/BLIT handling (unlikely)
3. COPER/BLIT handlers are never registered in the working version

**But more importantly:** The working version **inlines** timerOnInterrupt, while our version **calls** it. The jsr call creates a function call frame that could be causing stack issues.

## Hypothesis

The `jsr timerOnInterrupt()` call in smite.s creates a function call frame. If timerOnInterrupt or any function it calls has a tail call, or if the stack frame is misaligned, this could cause the crash.

The working version avoids this by inlining timerOnInterrupt, eliminating the function call entirely.


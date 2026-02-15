# Complete Structural Analysis: smite.s vs smite_works.s

## 🔴 CRITICAL DISCOVERY: Working Version Doesn't Call Handlers!

### smite_works.s int3Handler (WORKING)

**Complete Code Flow:**
```asm
int3Handler:
    fa2:  fmovemx fp0-fp1,-(sp)      # Save FP registers
    fa6:  movem.l d0-d2/a0-a1,-(sp)  # Save d0-d2, a0-a1
    faa:  move.w 197ac,d1            # ++g_sLogManager.wInterruptDepth (INLINED!)
    fb0:  move.w dff01e,d0           # uwIntReq = g_pCustom->intreqr
    fb6:  move.w d0,d2                # Copy to d2
    fb8:  andi.w #32,d2               # uwReqClr = INTF_VERTB (if set)
    fbc:  btst #5,d0                  # Check INTF_VERTB bit
    fc0:  bne.s fdc                   # If set, branch to timerOnInterrupt code
    
    # If INTF_VERTB NOT set, fall through to:
    fc2:  tst.w d1                    # Check interrupt depth
    fc4:  blt.s fee                    # If < 0, error
    fc6:  move.w d2,dff09c            # g_pCustom->intreq = uwReqClr
    fcc:  move.w d2,dff09c            # (duplicate write)
    fd2:  movem.l (sp)+,d0-d2/a0-a1   # Restore registers
    fd6:  fmovemx (sp)+,fp0-fp1       # Restore FP
    fda:  rte                         # Return

    # timerOnInterrupt code (INLINED at fdc):
    fdc:  move.w 19054,d0             # Load frame counter
    fe2:  addq.w #1,d0                # Increment
    fe4:  move.w d0,19054             # Store back
    fea:  moveq #32,d2                # uwReqClr = INTF_VERTB
    fec:  bra.s fc2                   # Branch back to depth check
```

**Key Observations:**
1. ✅ **NO function pointer calls** - Doesn't call `s_pAceInterrupts[].pHandler` at all!
2. ✅ **NO COPER/BLIT handling** - Only handles VERTB
3. ✅ **logPushInt/logPopInt INLINED** - No function calls
4. ✅ **timerOnInterrupt INLINED** - No function call
5. ✅ **Simple, linear code** - No complex branching

### smite.s int3Handler (BROKEN)

**Complete Code Flow:**
```asm
int3Handler:
    Save registers (d0-d3/a0-a1, FP)  # Saves d3!
    Load uwIntReq
    Check INTF_VERTB:
        If set:
            jsr timerOnInterrupt()     # FUNCTION CALL!
            Check s_pAceInterrupts[INTB_VERTB].pHandler
            If exists: jsr (a0)        # FUNCTION POINTER CALL!
            Set uwReqClr = INTF_VERTB
    Check INTF_COPER:
        If set:
            Check s_pAceInterrupts[INTB_COPER].pHandler
            If exists: jsr (a0)        # FUNCTION POINTER CALL!
            Set uwReqClr |= INTF_COPER
    Check INTF_BLIT:
        If set:
            Check s_pAceInterrupts[INTB_BLIT].pHandler
            If exists: jsr (a0)        # FUNCTION POINTER CALL!
            Set uwReqClr |= INTF_BLIT
    logPopInt()                        # FUNCTION CALL!
    Set intreq = uwReqClr
    Restore registers
    RTE
```

**Key Observations:**
1. ❌ **Multiple function pointer calls** - Calls handlers through `s_pAceInterrupts[].pHandler`
2. ❌ **COPER/BLIT handling** - Handles all three interrupt types
3. ❌ **Function calls** - timerOnInterrupt, logPopInt, handler functions
4. ❌ **Complex code** - Multiple branches and function calls

## 🔴 THE REAL DIFFERENCE

**The working version's int3Handler is a STUB that only:**
- Increments the frame counter
- Clears the interrupt flag
- **Does NOT call any registered handlers!**

**Our version's int3Handler:**
- Calls timerOnInterrupt()
- Calls registered handlers through function pointers
- Handles COPER and BLIT interrupts
- **Has multiple function pointer calls that could have tail calls!**

## Hypothesis

The working version might:
1. Use a different interrupt system (OS interrupt servers instead of direct handlers)
2. Have handlers registered elsewhere
3. Have been compiled with different source code that doesn't call handlers from int3Handler

**The function pointer calls in our version are the problem!** Each `jsr (a0)` call to a handler function could have tail calls that leak stack space.


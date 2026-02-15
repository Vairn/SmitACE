# THE REAL DIFFERENCE - Complete Analysis

## smite.s int3Handler (BROKEN) - Address 0xa8c

**Full Assembly:**
```asm
00000a8c <int3Handler>:
void HWINTERRUPT int3Handler(void) {
    a8c:  f227 e003       fmovemx fp0-fp1,-(sp)      # Save FP
    a90:  48e7 f0c0       movem.l d0-d3/a0-a1,-(sp)  # Save d0-d3, a0-a1
    a94:  [logPushInt call - need to check]
    a98:  [Load uwIntReq]
    a9c:  [Check INTF_VERTB]
    aa0:  [If set: call timerOnInterrupt]
    aa4:  [Check s_pAceInterrupts[INTB_VERTB].pHandler]
    aa8:  [If exists: jsr (a0)]                      # FUNCTION POINTER CALL!
    aac:  [Set uwReqClr = INTF_VERTB]
    ab0:  [Check INTF_COPER]
    ab4:  [If set: check handler, jsr (a0)]          # FUNCTION POINTER CALL!
    ab8:  [Check INTF_BLIT]
    abc:  [If set: check handler, jsr (a0)]          # FUNCTION POINTER CALL!
    ac0:  [logPopInt call]
    ac4:  [Set intreq = uwReqClr]
    ac8:  4cdf 030f       movem.l (sp)+,d0-d3/a0-a1  # Restore
    acc:  f21f d0c0       fmovemx (sp)+,fp0-fp1      # Restore FP
    ad0:  4e73            rte
```

**Key Points:**
- ❌ Saves d0-d3 (4 data registers)
- ❌ Calls timerOnInterrupt() with jsr
- ❌ Calls s_pAceInterrupts[].pHandler through function pointers (3 potential calls!)
- ❌ Handles COPER and BLIT interrupts
- ❌ Multiple function calls

## smite_works.s int3Handler (WORKING) - Address 0xfa2

**Full Assembly:**
```asm
00000fa2 <int3Handler>:
void HWINTERRUPT int3Handler(void) {
    fa2:  f227 e003       fmovemx fp0-fp1,-(sp)      # Save FP
    fa6:  48e7 e0c0       movem.l d0-d2/a0-a1,-(sp)  # Save d0-d2, a0-a1
    faa:  3239 0001 97ac  move.w 197ac,d1            # ++wInterruptDepth (INLINED!)
    fb0:  3039 00df f01e  move.w dff01e,d0           # uwIntReq = intreqr
    fb6:  3400            move.w d0,d2                # Copy to d2
    fb8:  0242 0020       andi.w #32,d2               # uwReqClr = INTF_VERTB (if set)
    fbc:  0800 0005       btst #5,d0                  # Check INTF_VERTB
    fc0:  661a            bne.s fdc                   # If set, branch to timer code
    
    # If INTF_VERTB NOT set:
    fc2:  4a41            tst.w d1                    # Check depth
    fc4:  6d28            blt.s fee                   # If < 0, error
    fc6:  33c2 00df f09c  move.w d2,dff09c            # intreq = uwReqClr
    fcc:  33c2 00df f09c  move.w d2,dff09c            # (duplicate)
    fd2:  4cdf 0307       movem.l (sp)+,d0-d2/a0-a1  # Restore
    fd6:  f21f d0c0       fmovemx (sp)+,fp0-fp1      # Restore FP
    fda:  4e73            rte                         # Return
    
    # timerOnInterrupt code (INLINED at fdc):
    fdc:  3039 0001 9054  move.w 19054,d0             # Load frame counter
    fe2:  5240            addq.w #1,d0                # Increment
    fe4:  33c0 0001 9054  move.w d0,19054             # Store
    fea:  7420            moveq #32,d2                # uwReqClr = INTF_VERTB
    fec:  60d4            bra.s fc2                   # Branch back
```

**Key Points:**
- ✅ Saves d0-d2 (3 data registers - NO d3!)
- ✅ timerOnInterrupt INLINED (no jsr call)
- ✅ **NO function pointer calls to s_pAceInterrupts[].pHandler!**
- ✅ **NO COPER/BLIT handling!**
- ✅ logPushInt/logPopInt INLINED
- ✅ Simple, linear code

## 🔴 THE CRITICAL DIFFERENCES

1. **Function Pointer Calls**: smite.s calls handlers through function pointers, smite_works.s doesn't!
2. **Register Count**: smite.s saves d3, smite_works.s doesn't
3. **COPER/BLIT**: smite.s handles them, smite_works.s doesn't
4. **Function Calls**: smite.s has multiple jsr calls, smite_works.s has none (all inlined)

## Hypothesis

**The function pointer calls in smite.s are the problem!** Each `jsr (a0)` call to a registered handler could:
- Have tail calls that leak stack
- Corrupt registers
- Cause stack misalignment

**The working version avoids this entirely by not calling handlers from int3Handler!**


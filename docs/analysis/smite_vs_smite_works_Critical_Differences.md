# Critical Differences: smite.s vs smite_works.s

## 🔴 CRITICAL ISSUE #1: Interrupt Handlers Missing Register Saves

### int3Handler (VBL Interrupt)

**smite.s (BROKEN):**
```asm
00000b02 <int3Handler>:
     b02:	2f01           	move.l d1,-(sp)    # Only saves d1
     b04:	2f00           	move.l d0,-(sp)    # Only saves d0
     b06:	3239 00df f01e 	move.w dff01e,d1   # Uses d1
     b0c:	3001           	move.w d1,d0       # Uses d0
     ...
     b24:	201f           	move.l (sp)+,d0
     b26:	221f           	move.l (sp)+,d1
     b28:	4e73           	rte
```

**smite_works.s (WORKING):**
```asm
00000fa2 <int3Handler>:
     fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)  # Saves FP registers!
     fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)  # Saves d0-d2, a0-a1!
     faa:	3239 0001 97ac 	move.w 197ac,d1
     ...
     fd2:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1  # Restores all
     fd6:	f21f d0c0      	fmovemx (sp)+,fp0-fp1      # Restores FP
     fda:	4e73           	rte
```

**Problem:** The broken version only saves `d0` and `d1`, but the interrupt handler code might use other registers (d2, a0, a1, or floating point registers). If the interrupted code was using those registers, they get corrupted!

### int4Handler (Audio Interrupt)

**smite.s (BROKEN):**
```asm
00000b3c <int4Handler>:
     b3c:	2f00           	move.l d0,-(sp)    # Only saves d0!
     b3e:	3039 00df f01e 	move.w dff01e,d0
     ...
     b50:	201f           	move.l (sp)+,d0
     b52:	4e73           	rte
```

**smite_works.s (WORKING):**
```asm
00000eae <int4Handler>:
     eae:	f227 e003      	fmovemx fp0-fp1,-(sp)  # Saves FP registers!
     eb2:	48e7 c0c0      	movem.l d0-d1/a0-a1,-(sp)  # Saves d0-d1, a0-a1!
     eb6:	3039 00df f01e 	move.w dff01e,d0
     ...
     ed0:	4cdf 0303      	movem.l (sp)+,d0-d1/a0-a1  # Restores all
     ed4:	f21f d0c0      	fmovemx (sp)+,fp0-fp1      # Restores FP
     ed8:	4e73           	rte
```

**Problem:** Same issue - only saves `d0`, but should save all registers used.

---

## 🔴 CRITICAL ISSUE #2: Missing logPushInt/logPopInt

**smite_works.s** has calls to `logPushInt()` and `logPopInt()` in interrupt handlers:
```asm
     faa:	3239 0001 97ac 	move.w 197ac,d1    # logPushInt() - increments depth
     ...
     fc2:	4a41           	tst.w d1            # logPopInt() - checks depth
```

**smite.s** appears to be missing these calls or they're optimized away. This could cause:
- Stack corruption detection not working
- Interrupt nesting issues not being caught

---

## 🔴 CRITICAL ISSUE #3: Tail Calls Still Present

**smite.s** still has 4 tail calls:
```asm
    2056:	4ed1           	jmp (a1)    # fadeCompleteGameOver
    2152:	4ed1           	jmp (a1)    # fadeCompleteWin
    2196:	4ed1           	jmp (a1)    # fadeCompleteTitle
    328a:	4ed1           	jmp (a1)    # stateChange
```

**smite_works.s** has ZERO tail calls - all use `jsr` with proper stack cleanup.

---

## ⚠️ Issue #4: Misassembled Instructions

Both files have `.short 0xe9c0` and `.short 0x4a88/0x4a89` entries, which are likely:
- 68020-specific instructions the disassembler doesn't recognize
- Or compiler-generated code patterns

These appear in both working and broken versions, so they're probably not the cause.

---

## Root Cause Analysis

The **most critical issue** is #1: **Interrupt handlers not saving all registers**.

### Why This Causes Crashes:

1. **Register Corruption**: When an interrupt fires, the CPU saves the program counter and status register, but NOT the general-purpose registers. If the interrupt handler uses registers without saving them first, it corrupts the interrupted code's state.

2. **68020 Specific Issue**: The 68020 has more registers and different calling conventions. If the compiler generates code that uses `a0`, `a1`, `d2`, or floating point registers in interrupt handlers, but the handler doesn't save them, the interrupted code's registers get corrupted.

3. **Cumulative Effect**: Each interrupt that corrupts registers makes the system more unstable. Eventually, a corrupted register value causes a crash.

### Why smite_works.s Works:

The working version saves **ALL** registers it might use:
- Floating point registers (`fmovemx fp0-fp1`)
- General purpose registers (`movem.l d0-d2/a0-a1`)

This ensures the interrupted code's state is fully preserved.

---

## Solution

The interrupt handlers need to save ALL registers they use. The working version shows the correct pattern:

```asm
fmovemx fp0-fp1,-(sp)      # Save floating point registers
movem.l d0-d2/a0-a1,-(sp)  # Save general purpose registers
# ... handler code ...
movem.l (sp)+,d0-d2/a0-a1 # Restore general purpose registers
fmovemx (sp)+,fp0-fp1      # Restore floating point registers
rte                        # Return from exception
```

This is likely a compiler optimization issue - the compiler is not generating proper register saves for interrupt handlers. The `HWINTERRUPT` attribute might not be working correctly, or the compiler is being too aggressive with register usage.

---

## Recommendations

1. **Check compiler flags** - Ensure interrupt handlers are compiled with proper register saving
2. **Verify HWINTERRUPT attribute** - Make sure it's correctly telling the compiler to save all registers
3. **Disable optimizations for interrupt handlers** - Use `__attribute__((optimize("O0")))` or similar
4. **Add explicit register saves** - If compiler doesn't do it, add inline assembly to save/restore registers


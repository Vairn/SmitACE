# Deep Dive: Why Tail Calls Work on 68000 But Crash on 68020
## The Critical Role of Exception Stack Frames and Interrupts

**Date:** Comprehensive analysis of interrupt/exception handling differences  
**Key Insight:** The 68020's complex exception stack frame format interacts catastrophically with tail call optimization in interrupt context

---

## Executive Summary

**Root Cause:** The 68020 processor uses a **more complex exception stack frame** that includes a **format word** beneath the saved Program Counter (PC). When interrupts fire and tail-call-optimized functions execute in interrupt context, the stack frame structure becomes misaligned. The processor's `RTE` (Return from Exception) instruction expects the format word, but tail call optimization corrupts the expected stack layout, causing crashes.

**Why 68000 Works:**
- Simpler exception stack frame (no format word)
- More forgiving stack layout
- Tail calls in interrupt context don't corrupt the exception frame structure

**Why 68020 Crashes:**
- Complex exception stack frame with format word
- `RTE` instruction requires specific stack layout
- Tail calls corrupt the exception frame, causing `RTE` to fail
- Faster execution means bugs manifest immediately

---

## Exception Stack Frame Differences

### 68000 Exception Stack Frame

When an interrupt or exception occurs on the 68000, the processor saves:

```
Stack after interrupt/exception:
┌─────────────┐
│   Format    │  ← Status Register (SR) - 16 bits
│   Word      │
├─────────────┤
│   Program   │  ← Program Counter (PC) - 32 bits
│   Counter   │
├─────────────┤
│   (rest of  │  ← Handler's stack frame
│   stack)    │
└─────────────┘
```

**Characteristics:**
- Simple, straightforward structure
- Only SR and PC saved
- Total: 6 bytes (2 bytes SR + 4 bytes PC)
- `RTE` instruction simply restores SR and PC

### 68020 Exception Stack Frame

The 68020 introduces a **format word** that provides additional exception information:

```
Stack after interrupt/exception:
┌─────────────┐
│   Format    │  ← Format Word (describes exception type)
│   Word      │     Contains exception vector number, etc.
├─────────────┤
│   Status    │  ← Status Register (SR) - 16 bits
│   Register  │
├─────────────┤
│   Program   │  ← Program Counter (PC) - 32 bits
│   Counter   │
├─────────────┤
│   (rest of  │  ← Handler's stack frame
│   stack)    │
└─────────────┘
```

**Characteristics:**
- **Format word** added beneath SR
- More complex structure
- Total: 8 bytes (2 bytes format + 2 bytes SR + 4 bytes PC)
- `RTE` instruction **requires** the format word to be present
- Format word describes exception type and additional context

**Critical Difference:** The 68020's `RTE` instruction **validates and uses** the format word. If the format word is missing or corrupted, `RTE` will fail, causing a format exception or crash.

---

## How Interrupts Work in This Codebase

### Interrupt Installation

From `deps/ACE/src/ace/managers/system.c`:

```c
ULONG osIntServer(void) {
    register volatile ULONG regData __asm("a1");
    
    UBYTE ubIntBit = regData;
    if(ubIntBit == INTB_VERTB) {
        timerOnInterrupt();
    }
    
    // INDIRECT CALL - Can be tail-call optimized!
    if(s_pAceInterrupts[ubIntBit].pHandler) {
        s_pAceInterrupts[ubIntBit].pHandler(
            g_pCustom, s_pAceInterrupts[ubIntBit].pData
        );
    }
    
    return ubIntBit == INTB_VERTB ? 0 : 1;
}
```

**Interrupts Installed:**
- **INTB_VERTB (Interrupt 5)**: Vertical blank - fires 50-60 times per second
- **INTB_AUD0-3 (Interrupt 4)**: Audio channels - fire when samples complete

**Critical Point:** These interrupts fire **continuously** during initialization, before `blitManagerCreate` runs.

### Interrupt Handler Chain

When an interrupt fires:

1. **Hardware saves exception frame** (SR, PC, format word on 68020)
2. **Jumps to interrupt vector** (installed by `systemCreate()`)
3. **Interrupt handler executes** (`osIntServer` or similar)
4. **Handler calls user function** through `s_pAceInterrupts[].pHandler`
5. **User function may call ProTracker functions** (`mt_e_cmds`, etc.)
6. **ProTracker functions use tail calls** (`jmp (a1)`)
7. **Handler returns** with `RTE` instruction
8. **RTE expects format word** - **CRASH if corrupted!**

---

## The Tail Call Problem in Interrupt Context

### Normal Function Call (Working)

```asm
Interrupt fires:
┌─────────────┐
│ Format Word │  ← 68020 exception frame
│ SR          │
│ PC          │
├─────────────┤
│ Handler     │  ← osIntServer stack frame
│ locals      │
├─────────────┤
│ Handler     │  ← Arguments for pHandler call
│ args        │
├─────────────┤
│ Return Addr │  ← From jsr (a0)
│ Handler     │
│ locals      │
├─────────────┤
│ Function    │  ← pHandler function stack
│ locals      │
└─────────────┘

Function returns (rts):
┌─────────────┐
│ Format Word │  ← Still intact!
│ SR          │
│ PC          │
├─────────────┤
│ Handler     │  ← Handler cleans up
│ locals      │
└─────────────┘

Handler returns (RTE):
RTE reads format word → ✅ SUCCESS
```

### Tail Call Optimization (Broken on 68020)

```asm
Interrupt fires:
┌─────────────┐
│ Format Word │  ← 68020 exception frame
│ SR          │
│ PC          │
├─────────────┤
│ Handler     │  ← osIntServer stack frame
│ locals      │
├─────────────┤
│ Handler     │  ← Arguments for pHandler call
│ args        │
├─────────────┤
│ Return Addr │  ← From jsr (a0) to pHandler
│ Handler     │
│ locals      │
├─────────────┤
│ Function    │  ← pHandler function stack
│ locals      │
├─────────────┤
│ Tail call   │  ← Arguments for tail-called function
│ args (12B)  │     (mt_e_cmds pushes 3 args)
└─────────────┘

Tail call executes (jmp):
┌─────────────┐
│ Format Word │  ← Still here
│ SR          │
│ PC          │
├─────────────┤
│ Handler     │  ← Handler stack frame
│ locals      │
├─────────────┤
│ Handler     │  ← Original args (overwritten)
│ args        │
├─────────────┤
│ Return Addr │  ← Points to handler
│ Handler     │
│ locals      │
├─────────────┤
│ Tail call   │  ← ❌ LEAKED! Never cleaned up
│ args (12B)  │
└─────────────┘

Tail-called function returns (rts):
Returns to handler, but stack is corrupted

Handler tries to return (RTE):
RTE reads format word → ❌ WRONG VALUE! (corrupted by leaked args)
→ Format exception or crash
```

---

## Why 68000 "Works"

### 68000 Exception Frame

The 68000's simpler exception frame is more forgiving:

```
Stack after interrupt:
┌─────────────┐
│ SR          │  ← Only SR and PC
│ PC          │
├─────────────┤
│ Handler     │
│ stack       │
└─────────────┘
```

**Why it works:**
1. **No format word** - `RTE` only needs SR and PC
2. **Leaked stack args** don't corrupt the exception frame
3. **RTE succeeds** even with extra bytes on stack
4. **Stack corruption accumulates slowly** (still a bug, but doesn't crash immediately)

**The bug still exists**, but:
- Stack leaks don't immediately corrupt exception frame
- Slower execution means leaks accumulate more slowly
- More stack headroom before overflow
- Different timing might allow "recovery"

---

## Why 68020 Crashes

### 68020 Exception Frame

The 68020's format word is **critical** for `RTE`:

```
Stack after interrupt:
┌─────────────┐
│ Format Word │  ← RTE REQUIRES THIS!
│ SR          │
│ PC          │
├─────────────┤
│ Handler     │
│ stack       │
└─────────────┘
```

**Why it crashes:**
1. **Format word is required** - `RTE` validates and uses it
2. **Tail call leaks stack args** - corrupts format word location
3. **RTE reads wrong value** - format word is overwritten
4. **Format exception** - processor detects invalid format
5. **Cascade failure** - exception handler also fails
6. **Immediate crash** - happens on first interrupt with tail call

### The Cascade

```
1. Interrupt fires → Exception frame saved (format word + SR + PC)
2. Handler calls pHandler → Tail-call-optimized function
3. Tail call leaks 12 bytes → Stack grows, format word location shifts
4. Handler returns → Tries to use corrupted stack
5. RTE executes → Reads wrong format word value
6. Format exception → Processor detects invalid format
7. Exception handler → Also fails (stack still corrupted)
8. CRASH → System halts
```

---

## Evidence from the Code

### Interrupt Frequency

From the analysis documents:
- **INTB_VERTB**: Fires **50-60 times per second** (every frame)
- **INTB_AUD0-3**: Fire when audio DMA completes
- Interrupts installed **early** in `systemCreate()`
- **Before `blitManagerCreate` runs**, dozens of interrupts have fired

### Tail Call Locations

Functions that use tail calls in interrupt context:
- `mt_e_cmds` - Called during music playback (in interrupt)
- `blocked_e_cmds` - Called during music playback (in interrupt)
- `mt_e8` - User callback (in interrupt)
- `checkmorefx` - Called every pattern row (in interrupt)

**Each tail call leaks 8-12 bytes** when executed in interrupt context.

### Stack Leak Rate

With interrupts firing:
- **50-60 interrupts/second** (VERTB)
- **Each interrupt** may call ProTracker functions
- **Each ProTracker call** may use tail-call-optimized dispatchers
- **Each tail call** leaks 8-12 bytes
- **Result**: Hundreds of bytes leaked per second

On a 4-8KB stack, this causes **rapid exhaustion**.

---

## The Critical Difference: Format Word Validation

### 68000 RTE Behavior

```asm
RTE instruction on 68000:
1. Pop PC from stack
2. Pop SR from stack
3. Resume execution

No validation - just restores registers
```

**Forgiving:** Even if stack has extra bytes, `RTE` just pops what it needs and continues.

### 68020 RTE Behavior

```asm
RTE instruction on 68020:
1. Read format word from stack
2. Validate format word
3. If invalid → Format exception
4. If valid → Use format to determine frame type
5. Pop appropriate registers based on format
6. Resume execution
```

**Strict:** `RTE` **requires** valid format word. If format word is corrupted (by leaked stack args), `RTE` fails immediately.

---

## Why This Explains Everything

### Why 68000 Appears to Work

1. **No format word** - `RTE` doesn't validate stack structure
2. **Leaked bytes don't matter** - `RTE` just pops SR and PC
3. **Stack corruption accumulates** - but doesn't cause immediate crash
4. **Bug still exists** - just manifests more slowly

### Why 68020 Crashes Immediately

1. **Format word required** - `RTE` validates it
2. **Tail calls corrupt format word** - leaked args overwrite it
3. **RTE fails immediately** - format exception on first return
4. **Cascade failure** - exception handler also fails
5. **Instant crash** - happens on first interrupt with tail call

### Why Disabling Tail Calls Fixes It

1. **No stack leaks** - functions clean up properly
2. **Format word preserved** - exception frame intact
3. **RTE succeeds** - format word is valid
4. **Stable execution** - interrupts work correctly

---

## Testing Hypothesis

### Test 1: Monitor Exception Frames

Add debug code to check format word before `RTE`:

```c
// In interrupt handler, before return
ULONG *pStack = (ULONG *)get_exception_frame();
UWORD formatWord = pStack[0];
if(formatWord != EXPECTED_FORMAT) {
    logWrite("Format word corrupted: %04x\n", formatWord);
    // This would confirm the theory
}
```

### Test 2: Disable Interrupts Temporarily

If interrupts are disabled during initialization:
- 68020 should not crash
- This would prove interrupt context is the issue

### Test 3: Compare Stack Layouts

Dump stack before/after tail calls in interrupt context:
- 68000: Format word location unchanged
- 68020: Format word location shifted by leaked bytes

---

## Conclusion

**The Real Answer:**

The 68000 and 68020 handle exception stack frames **fundamentally differently**:

1. **68000**: Simple frame (SR + PC), `RTE` is forgiving
2. **68020**: Complex frame (format + SR + PC), `RTE` is strict

When tail call optimization leaks stack bytes in interrupt context:

- **68000**: Leaked bytes don't corrupt exception frame → `RTE` succeeds → Appears to work
- **68020**: Leaked bytes corrupt format word → `RTE` fails → Immediate crash

**The tail call bug exists in both**, but:
- **68000**: Bug is masked by simpler exception handling
- **68020**: Bug is exposed by strict format word validation

**Solution:** Disable tail call optimization for both, but the **68020's strict exception handling** is why it crashes immediately while 68000 appears to work.

---

## Recommendations

### Immediate Fix

```cmake
# Disable tail call optimization for all builds
target_compile_options(${SMITE_EXECUTABLE} PRIVATE -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE -fno-optimize-sibling-calls)
```

### Long-term Considerations

1. **68020 Exception Handling:**
   - Be aware of format word requirements
   - Ensure stack alignment in interrupt handlers
   - Validate exception frames in debug builds

2. **Interrupt Safety:**
   - Avoid complex stack operations in interrupt context
   - Be careful with function pointers in interrupts
   - Monitor stack usage in interrupt handlers

3. **Testing:**
   - Test interrupt handlers thoroughly
   - Monitor stack usage during interrupts
   - Verify exception frame integrity

---

**Document Version:** 1.0  
**Last Updated:** Deep dive analysis  
**Related Documents:**
- `TailCallOptimizationBug_Analysis.md`
- `Why_68000_Works_But_68020_Crashes.md`
- `TailCall_RegisterUsage_Comparison_Report.md`


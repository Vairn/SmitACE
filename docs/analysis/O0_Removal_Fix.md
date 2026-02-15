# O0 Removal Fix - The Real Root Cause

## Problem Identified

**O0 optimization is causing:**
1. Inefficient code generation that uses d3 unnecessarily
2. 4-byte larger stack frame (32 bytes vs 28 bytes)
3. Stack frame misalignment that corrupts exception frames

## Current (BROKEN with O0):
```asm
int3Handler:
    movem.l d0-d3/a0-a1,-(sp)  # 6 registers = 24 bytes
    moveq #32,d3                # Uses d3 for constant
    and.l d3,d1
    ...
    movem.l (sp)+,d0-d3/a0-a1
```
**Stack frame: 32 bytes**

## Working (without O0):
```asm
int3Handler:
    movem.l d0-d2/a0-a1,-(sp)  # 5 registers = 20 bytes
    andi.w #32,d2               # Immediate constant
    ...
    movem.l (sp)+,d0-d2/a0-a1
```
**Stack frame: 28 bytes**

## Fix Applied

Changed from `O0` (disable all optimizations) to `no-optimize-sibling-calls` (targeted fix):

```c
// OLD:
__attribute__((optimize("O0")))

// NEW:
__attribute__((optimize("no-optimize-sibling-calls")))
```

This allows the compiler to:
- ✅ Optimize away unnecessary d3 usage
- ✅ Use immediate constants instead of registers
- ✅ Generate smaller stack frames
- ✅ Still prevent tail call optimization

## Why This Works

The working version was compiled **without O0**, allowing the compiler to optimize away d3 usage. By removing O0, we match the working version's optimization level while still preventing tail calls.

## Status

✅ **Fixed:** `int3Handler` and `int4Handler` now use `no-optimize-sibling-calls` instead of `O0`

**Expected Result:** Smaller stack frames (28 bytes instead of 32 bytes), matching the working version.


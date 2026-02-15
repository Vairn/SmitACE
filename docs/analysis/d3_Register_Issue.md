# d3 Register Issue - The Real Problem

## Current State (smite.s - BROKEN)

```asm
int3Handler:
    b42:	f227 e003      	fmovemx fp0-fp1,-(sp)
    b46:	48e7 f0c0      	movem.l d0-d3/a0-a1,-(sp)  ← SAVES d3
    ...
    b60:	7620           	moveq #32,d3               ← USES d3 for constant
    b62:	c283           	and.l d3,d1
    ...
    b9a:	7610           	moveq #16,d3               ← USES d3 again
    b9c:	c283           	and.l d3,d1
    ...
    c1c:	4cdf 030f      	movem.l (sp)+,d0-d3/a0-a1  ← RESTORES d3
```

**Stack frame:** 8 (FP) + 24 (regs) = **32 bytes**

## Working Version (smite_works.s)

```asm
int3Handler:
    fa2:	f227 e003      	fmovemx fp0-fp1,-(sp)
    fa6:	48e7 e0c0      	movem.l d0-d2/a0-a1,-(sp)  ← NO d3!
    ...
    fb8:	0242 0020      	andi.w #32,d2             ← IMMEDIATE constant
    ...
    fd2:	4cdf 0307      	movem.l (sp)+,d0-d2/a0-a1  ← NO d3!
```

**Stack frame:** 8 (FP) + 20 (regs) = **28 bytes**

## The Problem

1. **smite.s saves d3 unnecessarily** - uses d3 to hold constants (32, 16) when immediate constants would work
2. **4-byte stack frame difference** - 32 bytes vs 28 bytes
3. **This misalignment** could cause exception frame corruption on 68020

## Why This Happens

With `__attribute__((optimize("O0")))`, the compiler generates unoptimized code that:
- Uses registers to hold constants instead of immediate values
- Doesn't optimize away unnecessary register usage
- Generates larger stack frames

## The Fix

The working version doesn't use d3 at all. We need to ensure our code also doesn't use d3, OR we need to match the working version's exact register usage pattern.

**Option 1:** Remove O0 and use a more targeted optimization disable
**Option 2:** Force the compiler to use immediate constants instead of d3
**Option 3:** Accept d3 usage but ensure stack frame matches (unlikely to work)

The real issue is that **O0 is too aggressive** - it disables optimizations that would eliminate d3 usage, but it also generates inefficient code that uses d3 unnecessarily.


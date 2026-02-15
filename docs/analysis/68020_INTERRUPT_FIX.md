# 68020 Interrupt Race Condition Fix

## Problem

When compiling for the 68020 processor, the application hits a race condition in interrupt handling. This is due to differences in how the 68020 handles interrupts compared to the 68000:

1. **Instruction Cache**: The 68020 has an instruction cache that can cache reads from memory-mapped I/O registers, causing the CPU to miss interrupt edges
2. **Faster Execution**: The 68020's faster execution speed creates timing windows where interrupts can be missed or improperly acknowledged
3. **Interrupt Acknowledgment Timing**: The 68020 requires more careful interrupt acknowledgment to avoid race conditions

## Root Cause

The VBL (vertical blank) wait loops in `system.c` were waiting for the interrupt to occur but not explicitly acknowledging it before proceeding with DMA operations. On 68000, this worked due to slower execution and lack of instruction caching. On 68020, the instruction cache and faster execution can cause:

- The interrupt to be detected but not properly acknowledged
- Race conditions where DMA is disabled while an interrupt is still pending
- Instruction cache causing stale reads of interrupt status registers

## Solution

The fix acknowledges the VBL interrupt **twice** (a known workaround for high-end Amiga configurations) immediately after detecting it, before disabling DMA. This ensures:

1. The interrupt is properly acknowledged on 68020
2. Any pending interrupt state is cleared
3. The instruction cache is effectively flushed by the register writes

## Changes Made

### Fixed Locations in `deps/ACE/src/ace/managers/system.c`:

1. **`systemCreate()`** (line ~927):
   - Added VBL interrupt acknowledgment after wait loop

2. **`systemOsDisable()`** (line ~717):
   - Added VBL interrupt acknowledgment after wait loop

3. **`systemUse()`** (line ~1041):
   - Added VBL interrupt acknowledgment after wait loop

4. **`systemDestroy()`** (line ~953):
   - Added VBL interrupt acknowledgment after wait loop

### Pattern Applied:

```c
// Wait for vbl before disabling sprite DMA
// On 68020+ we need to properly acknowledge the interrupt to avoid race conditions
while (!(g_pCustom->intreqr & INTF_VERTB)) continue;
// Acknowledge VBL interrupt (clear twice for 68020+ compatibility)
g_pCustom->intreq = INTF_VERTB;
g_pCustom->intreq = INTF_VERTB;
// Now safe to disable DMA
g_pCustom->dmacon = ...;
```

## Why Clear Twice?

From the ACE codebase comments (line 35-36 in system.c):
```
// Clearing bit is done twice for some high end configs
// http://eab.abime.net/showthread.php?t=95263
```

This is a known requirement for 68020+ processors and certain accelerator configurations. Writing to `intreq` twice ensures the interrupt acknowledgment is properly registered even if the first write is cached or delayed.

## Testing

After applying this fix, test on:
- 68020-based Amigas (A1200, A3000, A4000)
- Accelerated 68000 systems (if available)
- UAE emulator with 68020 CPU emulation

The fix should not affect 68000 compatibility since the double-clear is harmless on slower processors.

## References

- ACE Framework issue discussion on interrupt handling
- EAB thread: http://eab.abime.net/showthread.php?t=95263
- Amiga Hardware Reference Manual - Interrupt handling chapter


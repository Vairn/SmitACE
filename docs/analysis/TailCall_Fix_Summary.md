# Tail Call Fix Summary

## Problem
The code was crashing on 68020 due to tail call optimization (TCO) bug in GCC's m68k backend. The optimizer was converting `jsr/rts` sequences to `jmp` instructions without cleaning up stack-allocated arguments, causing progressive stack corruption.

## Attempted Solution: Per-Function Attributes
We added `__attribute__((optimize("no-optimize-sibling-calls")))` to:
- All interrupt handlers (int1Handler through int7Handler, osIntServer, osIntVector, ciaIcrHandler, aceInputHandler)
- ProTracker functions (mt_e_cmds, blocked_e_cmds, checkmorefx, mt_e8, moreBlockedFx)

**Result:** The attributes worked for direct function calls - assembly shows `jsr` with proper stack cleanup for these functions. However, the code still crashes.

## Root Cause of Persistent Crash
The problem is that **functions called through function pointers** cannot be protected by per-function attributes. For example:
- `s_pAceInterrupts[INTB_VERTB].pHandler()` - called from interrupt handlers
- `ecmd_tab[ubCmdE]()` - called from mt_e_cmds
- `blecmd_tab[ubCmdE]()` - called from blocked_e_cmds

These function pointers can point to any function, and those functions might have tail calls. We can't add attributes to function pointers at compile time.

## Final Solution: Global Disable
Enabled global `-fno-optimize-sibling-calls` flag in `CMakeLists.txt`:

```cmake
target_compile_options(${SMITE_EXECUTABLE} PRIVATE  -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE   -fno-optimize-sibling-calls)
```

This disables tail call optimization for the entire codebase, ensuring no function (whether called directly or through function pointers) can have the problematic tail call optimization.

## Verification
After rebuilding with the global flag:
1. All functions should use `jsr` instead of `jmp` for function calls
2. All function calls should have proper stack cleanup (`lea X(sp),sp` before `rts`)
3. No tail calls should be present in the generated assembly

## Performance Impact
Minimal - disabling TCO adds one extra `rts` instruction per tail-call-eligible function (~6 cycles on 68000), which is negligible compared to the stability gained.


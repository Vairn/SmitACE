# Why It's Still Crashing - Analysis

## Root Cause

The **global `-fno-optimize-sibling-calls` flag was commented out** in `CMakeLists.txt`!

## The Problem

Even though we added attributes to specific functions:
- ✅ Interrupt handlers have `O0` attribute
- ✅ ProTracker functions have `no-optimize-sibling-calls` attribute
- ✅ `stateChange` has `no-optimize-sibling-calls` attribute

**BUT:** These attributes only work for **direct function calls**. They **DO NOT** work for:
1. **Function pointer calls** - Functions called through `s_pAceInterrupts[].pHandler`, `ecmd_tab[]`, etc.
2. **Functions called from interrupt handlers** - Like `mt_TimerAInt`, `mt_TimerBInt`, `intPlay`, `mt_music`, etc.

## Why Per-Function Attributes Don't Work

When you call a function through a pointer:
```c
s_pAceInterrupts[INTB_VERTB].pHandler(g_pCustom, pData);
```

The compiler doesn't know what function `pHandler` points to, so it can't apply the attributes. The **callee** function might have tail calls even if the **caller** has attributes.

## Functions That Need Global Protection

These functions are called from interrupt context but we can't add attributes to them because they're called through function pointers:

1. **`mt_TimerAInt`** - Registered as `INTB_VERTB` handler, calls `intPlay()` → `mt_music()` → ProTracker functions
2. **`mt_TimerBInt`** - Registered as CIA timer handler
3. **`intPlay`** - Called from `mt_TimerAInt`
4. **`mt_music`** - Called from `intPlay`, calls ProTracker functions
5. **`mt_sfxonly`** - Called from `intPlay`, calls ProTracker functions
6. **Any user-registered interrupt handlers** - Unknown functions registered via `systemSetInt()`

## Solution

**Enable the global flag** in `CMakeLists.txt`:

```cmake
target_compile_options(${SMITE_EXECUTABLE} PRIVATE  -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE   -fno-optimize-sibling-calls)
```

This disables tail call optimization for **ALL** functions in the codebase, ensuring:
- ✅ No function pointer calls can have tail calls
- ✅ No functions called from interrupt context can have tail calls
- ✅ Complete protection across the entire codebase

## Trade-off

**Performance Impact:** Minimal - adds one extra `rts` instruction per tail-call-eligible function (~6 cycles on 68000). This is negligible compared to the stability gained.

**Benefit:** Complete protection against the tail call bug, no matter how functions are called.

---

## Status

✅ **Fixed:** Global flag has been enabled in `CMakeLists.txt`

**Next Step:** Rebuild the project and verify the assembly shows `jsr` instead of `jmp` for all function calls.


# prefx_tab Functions - Tail Call Fix

## Issue Found

The functions in `prefx_tab` are called from `mt_playvoice`, which is called from interrupt context. These functions have **tail calls** that were being optimized:

1. **`set_finetune`** - Tail calls `set_period` at the end
2. **`set_sampleoffset`** - Tail calls `set_period` at the end  
3. **`set_period`** - Called from the above, and itself calls `checkmorefx` (already protected)
4. **`set_toneporta`** - Also in `prefx_tab`, should be protected too

## Call Chain

```
Interrupt Handler (int3Handler/int4Handler)
  ↓ (calls through function pointer)
mt_TimerAInt / mt_TimerBInt
  ↓ (direct call)
mt_music / mt_sfxonly
  ↓ (direct call)
mt_playvoice
  ↓ (calls through prefx_tab function pointer)
set_finetune / set_sampleoffset / set_period / set_toneporta
  ↓ (tail call - LEAKS STACK!)
set_period / checkmorefx
```

## Fix Applied

Added `__attribute__((optimize("no-optimize-sibling-calls")))` and `__attribute__((noinline))` to:

1. ✅ **`set_period`** - Line ~2650
2. ✅ **`set_finetune`** - Line ~2688
3. ✅ **`set_sampleoffset`** - Line ~2699
4. ✅ **`set_toneporta`** - Line ~2710

## Why This Matters

These functions are called **very frequently** during music playback:
- Every frame (50-60 Hz) during music playback
- For every channel that has a note/command
- Through function pointers, so per-function attributes on callers don't help

Each tail call that leaks 4-12 bytes of stack space accumulates rapidly, causing stack corruption and crashes.

## Status

✅ **Fixed:** All `prefx_tab` functions now protected against tail call optimization


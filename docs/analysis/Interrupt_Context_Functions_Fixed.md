# Interrupt Context Functions - Fixed Without Global Flag

## Problem

Functions called from interrupt context (but not hardware interrupt handlers) were not protected against tail call optimization. These functions are registered as interrupt handlers through function pointers, so per-function attributes on the callers don't help.

## Functions Fixed

All functions called from interrupt context now have `__attribute__((optimize("no-optimize-sibling-calls")))`:

### ProTracker Interrupt Handlers

1. ✅ **`mt_TimerAInt`** - Registered as `INTB_VERTB` or CIA timer handler
   - Calls: `intPlay()` → `mt_music()` or `mt_sfxonly()`

2. ✅ **`mt_TimerBInt`** - Registered as CIA timer B handler
   - Calls: `intSetRep()` or `intDmaOn()`

3. ✅ **`intPlay`** - Called from `mt_TimerAInt`
   - Calls: `mt_music()` or `mt_sfxonly()`

4. ✅ **`intSetRep`** - Called from `mt_TimerBInt`
   - Sets channel repeat registers

5. ✅ **`intDmaOn`** - Called from `mt_TimerBInt`
   - Enables DMA for audio channels

6. ✅ **`mt_music`** - Called from `intPlay`
   - Main music playback routine, calls ProTracker functions

7. ✅ **`mt_sfxonly`** - Called from `intPlay`
   - Sound effects only playback

8. ✅ **`onAudio`** - Registered as audio interrupt handler (if `PTPLAYER_USE_AUDIO_INT_HANDLERS` is defined)
   - Handles audio channel completion interrupts

## Why These Need Protection

These functions are called from interrupt context but are **not** hardware interrupt handlers:
- They don't have `HWINTERRUPT` attribute
- They're registered via `systemSetInt()` or `systemSetCiaInt()`
- They're called through function pointers from interrupt handlers
- They can have tail calls that leak stack space in interrupt context

## Complete Protection Chain

```
Hardware Interrupt (int3Handler/int4Handler)
  ↓ (calls through function pointer)
mt_TimerAInt / mt_TimerBInt / onAudio
  ↓ (direct call)
intPlay / intSetRep / intDmaOn
  ↓ (direct call)
mt_music / mt_sfxonly
  ↓ (calls)
ProTracker functions (mt_e_cmds, etc.)
```

All levels are now protected with `no-optimize-sibling-calls` attribute.

## Status

✅ **All interrupt context functions protected** without needing global `-fno-optimize-sibling-calls` flag.

**Next Step:** Rebuild and verify the assembly shows `jsr` instead of `jmp` for all these functions.


# timerOnInterrupt Inline Fix

## Critical Difference Found

**smite_works.s (WORKING):**
- `timerOnInterrupt` is **INLINED** into `int3Handler`
- No function call (`jsr`) - code is directly in the handler
- No function call frame overhead
- Simpler code flow

**smite.s (BROKEN):**
- `timerOnInterrupt` is **CALLED** with `jsr`
- Creates function call frame
- More complex stack management
- Potential for stack misalignment

## Fix Applied

Changed `timerOnInterrupt` from a regular function to `static inline`:

**Before:**
```c
// timer.c
void timerOnInterrupt(void) {
	++g_sTimerManager.uwFrameCounter;
}

// timer.h
void timerOnInterrupt(void);
```

**After:**
```c
// timer.h
static inline void timerOnInterrupt(void) {
	++g_sTimerManager.uwFrameCounter;
}
```

## Why This Matters

1. **No function call overhead** - Eliminates `jsr`/`rts` pair
2. **No stack frame** - No return address pushed/popped
3. **Matches working version** - Same code generation as smite_works.s
4. **Simpler interrupt handler** - Less code, less chance for bugs

## Expected Result

After this change, `int3Handler` should generate code similar to smite_works.s:
- `timerOnInterrupt` code inlined directly
- No `jsr timerOnInterrupt` call
- Simpler, more efficient code flow


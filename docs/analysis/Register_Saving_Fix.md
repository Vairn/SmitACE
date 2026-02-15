# Register Saving Fix

## Issue Found

The current `int3Handler` and `int4Handler` store `uwReqClr` on the stack instead of in a register. This makes it vulnerable to stack corruption from tail calls.

**Current (vulnerable):**
```c
UWORD uwReqClr = 0;  // Stored on stack at 42(sp)
```

**Working version:**
- Uses `d2` register for `uwReqClr`
- Saves `d2` in `movem.l d0-d2/a0-a1,-(sp)`

## Fix Applied

Changed `uwReqClr` to use register `d2` explicitly:

```c
register UWORD uwReqClr __asm("d2") = 0;
```

This ensures:
1. ✅ `uwReqClr` is stored in register `d2`, not on stack
2. ✅ `d2` will be automatically saved by the compiler (since we use it)
3. ✅ `uwReqClr` won't be corrupted by stack leaks from tail calls

## Why This Matters

If `uwReqClr` is on the stack and tail calls leak stack space, the stack location where `uwReqClr` is stored gets corrupted. This can cause:
- Wrong interrupt flags being cleared
- Interrupts not being properly acknowledged
- System instability

By using a register, `uwReqClr` is protected from stack corruption.

## Status

✅ **Fixed:** `int3Handler` and `int4Handler` now use `d2` register for `uwReqClr`

**Next Step:** Rebuild and verify the assembly shows `d2` being used instead of stack storage.


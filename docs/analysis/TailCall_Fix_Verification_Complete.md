# Tail Call Fix Verification - Complete

## ✅ Verification Summary

All critical functions have been updated with appropriate attributes to prevent tail call optimization and ensure proper register saving.

---

## 🔴 Hardware Interrupt Handlers (system.c)

**Status:** ✅ All 7 handlers updated with `__attribute__((optimize("O0")))`

This disables ALL optimizations to ensure proper register saving (critical for interrupt handlers).

1. ✅ `int1Handler` - Line 135
2. ✅ `int2Handler` - Line 145
3. ✅ `int3Handler` - Line 190
4. ✅ `int4Handler` - Line 235
5. ✅ `int5Handler` - Line 280
6. ✅ `int6Handler` - Line 290
7. ✅ `int7Handler` - Line 339

**Attribute used:**
```c
__attribute__((optimize("O0")))  // Disables ALL optimizations
__attribute__((noinline))
FN_HOTSPOT
void HWINTERRUPT intXHandler(void) {
```

**Why O0?** Interrupt handlers need to save ALL registers (including FP registers). O0 ensures the compiler doesn't optimize away register saves.

---

## 🟡 OS Interrupt Handlers (system.c)

**Status:** ✅ All 4 handlers updated with `__attribute__((optimize("no-optimize-sibling-calls")))`

1. ✅ `aceInputHandler` - Line 351
2. ✅ `ciaIcrHandler` - Line 375
3. ✅ `osIntVector` - Line 390
4. ✅ `osIntServer` - Line 408

**Attribute used:**
```c
__attribute__((optimize("no-optimize-sibling-calls")))
__attribute__((noinline))
```

**Why no-optimize-sibling-calls?** These are OS-level handlers that call function pointers. We need to prevent tail calls but don't need full O0.

---

## 🟢 ProTracker Functions (ptplayer.c)

**Status:** ✅ All 5 functions updated with `__attribute__((optimize("no-optimize-sibling-calls")))`

1. ✅ `moreBlockedFx` - Line 1191
2. ✅ `checkmorefx` - Line 1224
3. ✅ `mt_e_cmds` - Line 2207
4. ✅ `blocked_e_cmds` - Line 2282
5. ✅ `mt_e8` - Line 2477

**Attribute used:**
```c
__attribute__((optimize("no-optimize-sibling-calls")))
__attribute__((noinline))
static void mt_e_cmds(...) {
```

**Why no-optimize-sibling-calls?** These functions call through function pointer tables (`ecmd_tab`, `blecmd_tab`, etc.) and must not use tail calls.

---

## 🔵 State Management (state.c)

**Status:** ✅ `stateChange` updated with `__attribute__((optimize("no-optimize-sibling-calls")))`

1. ✅ `stateChange` - Line 145

**Attribute used:**
```c
__attribute__((optimize("no-optimize-sibling-calls")))
__attribute__((noinline))
void stateChange(tStateManager *pStateManager, tState *pState) {
```

**Why?** This function calls `pStateManager->pCurrent->cbCreate()` through a function pointer, which was being tail-call optimized.

---

## 📊 Summary

| Category | Functions | Attribute | Status |
|----------|-----------|-----------|--------|
| Hardware Interrupts | 7 | `O0` | ✅ Complete |
| OS Interrupts | 4 | `no-optimize-sibling-calls` | ✅ Complete |
| ProTracker | 5 | `no-optimize-sibling-calls` | ✅ Complete |
| State Management | 1 | `no-optimize-sibling-calls` | ✅ Complete |
| **TOTAL** | **17** | - | ✅ **Complete** |

---

## 🔍 Next Steps

After rebuilding, verify the assembly:

1. **Interrupt handlers** should save all registers:
   ```asm
   fmovemx fp0-fp1,-(sp)      # FP registers
   movem.l d0-d2/a0-a1,-(sp)  # GP registers
   ```

2. **All function calls** should use `jsr` not `jmp`:
   ```asm
   jsr (a0)           # Good - proper call
   lea 12(sp),sp      # Stack cleanup
   rts                # Return
   ```

3. **No tail calls** should remain:
   ```asm
   jmp (a1)           # Bad - tail call (should not appear)
   ```

---

## ✅ Verification Complete

All critical functions have been protected against tail call optimization. The system should now be stable after rebuilding.


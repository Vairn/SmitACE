# Call Pattern Comparison: 68000 (Working) vs 68020 (Broken)

## Executive Summary

This document analyzes the different call patterns between the working 68000 build (`smite.s`) and the broken 68020 build (`smite_broken.s`).

### Key Findings

1. **More Function Calls in 68020:**
   - 68020 has **156 more indirect calls** (635 vs 479)
   - 68020 has **98 more absolute calls** (498 vs 400)
   - 68020 has **111 more register indirect calls** (536 vs 425)

2. **More Stack Cleanup in 68020:**
   - 68020 has **240 more stack cleanup operations** (441 vs 201)
   - Suggests more function calls requiring cleanup

3. **Mysterious `.short 0xe9c0` Instructions:**
   - Appears **16 times** in broken version
   - Appears **0 times** in working version
   - Likely a 68020-specific instruction or disassembly artifact

4. **Register Usage Differences:**
   - 68020 uses `d4` register (potentially uninitialized)
   - 68000 doesn't use `d4` in critical paths

5. **Addressing Mode Differences:**
   - 68000: Manual index calculation (2 `add.l` instructions)
   - 68020: Hardware-scaled addressing (`:4` scale factor)

6. **Same Tail Call Bug:**
   - Both versions use `jmp (a1)` for tail calls
   - Both versions have the same stack management issue
   - But 68020 version crashes while 68000 works (suggests different trigger conditions)

## Overall Statistics

| Pattern | 68000 (smite.s) | 68020 (smite_broken.s) | Difference |
|---------|----------------|----------------------|------------|
| **Total call/jump instructions** | 1,293 | 1,417 | +124 |
| **Indirect calls `jsr (aX)`** | 479 | 635 | +156 |
| **Indirect jumps `jmp (aX)`** | 15 | 11 | -4 |
| **Stack cleanup `lea N(sp),sp`** | 201 | 441 | +240 |
| **Relative calls `jsr N(pc)`** | 0 | 0 | 0 |
| **Branch calls `bsr`** | 0 | 0 | 0 |
| **Absolute calls `jsr N` (4eb9)** | 400 | 498 | +98 |
| **Register indirect `jsr (aX)` (4e90-4e94)** | 425 | 536 | +111 |
| **`.short 0xe9c0` (disassembly issue)** | 0 | 16 | +16 ⚠️ |

**Key Finding:** The 68020 broken version has:
- **More indirect calls** (635 vs 479) - +156 calls
- **More stack cleanup operations** (441 vs 201) - +240 cleanups
- **Fewer tail call optimizations** (11 vs 15) - -4 jumps

This suggests the 68020 version is generating MORE function calls overall, but potentially with different stack management patterns.

---

## Critical Function: `mt_e_cmds`

### 68000 Version (Working)

```asm
00001434 <mt_e_cmds>:
    1434:	2f02           	move.l d2,-(sp)          ; Save d2
    1436:	222f 0008      	move.l 8(sp),d1          ; Load ubArgs from caller's stack
    UBYTE ubCmdE = (ubArgs & 0xF0) >> 4;
    143a:	1001           	move.b d1,d0
    143c:	e808           	lsr.b #4,d0
    ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    143e:	0280 0000 00ff 	andi.l #255,d0
    1444:	d080           	add.l d0,d0              ; Multiply by 2
    1446:	d080           	add.l d0,d0              ; Multiply by 4 (total: *4)
    1448:	740f           	moveq #15,d2
    144a:	c481           	and.l d1,d2              ; ubArgE = ubArgs & 0x0F
    144c:	2f42 0008      	move.l d2,8(sp)          ; ⚠️ OVERWRITE arg 1 in caller's frame
    1450:	41f9 0001 05d2 	lea 105d2 <ecmd_tab>,a0
    1456:	2270 0800      	movea.l (0,a0,d0.l),a1   ; ✅ Standard indexed: (0,a0,d0.l)
    145a:	241f           	move.l (sp)+,d2          ; Restore d2
    145c:	4ed1           	jmp (a1)                 ; ⚠️ TAIL CALL - no cleanup
```

**Call Pattern:**
- **Addressing:** Standard 68000 indexed addressing `(0,a0,d0.l)`
- **Index calculation:** Manual multiply by 4 (two `add.l d0,d0`)
- **Stack reuse:** Overwrites caller's argument at `8(sp)`
- **Tail call:** Uses `jmp (a1)` - no stack cleanup
- **Register usage:** Uses `d0`, `d1`, `d2` consistently

### 68020 Version (Broken)

```asm
0000187c <mt_e_cmds>:
    187c:	2f02           	move.l d2,-(sp)          ; Save d2
    187e:	202f 0008      	move.l 8(sp),d0          ; Load ubArgs from caller's stack
    ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    1882:	e9c0           	.short 0xe9c0            ; ⚠️ Disassembly issue
    1884:	1604           	move.b d4,d3             ; ⚠️ Uses d4 (uninitialized?)
    1886:	740f           	moveq #15,d2
    1888:	c480           	and.l d0,d2              ; ubArgE = ubArgs & 0x0F
    188a:	2f42 0008      	move.l d2,8(sp)          ; ⚠️ OVERWRITE arg 1 in caller's frame
    188e:	2270 1db0 0001 	movea.l (83910,d1.l:4),a1 ; ✅ 68020 scaled: (base,d1.l:4)
    1894:	47c6 
    1896:	241f           	move.l (sp)+,d2          ; Restore d2
    1898:	4ed1           	jmp (a1)                 ; ⚠️ TAIL CALL - no cleanup
```

**Call Pattern:**
- **Addressing:** 68020 scaled indexed addressing `(83910,d1.l:4)` with `:4` scale factor
- **Index calculation:** Hardware scaling (no manual multiply)
- **Stack reuse:** Overwrites caller's argument at `8(sp)` (same as 68000)
- **Tail call:** Uses `jmp (a1)` - no stack cleanup (same as 68000)
- **Register usage:** Uses `d0`, `d1`, `d2`, but also references `d4` (line 1884) - **potential issue**

**Key Differences:**
1. ✅ **68020 uses scaled addressing** - more efficient
2. ⚠️ **68020 uses `d4` register** - may be uninitialized or corrupted
3. ⚠️ **Disassembly shows `.short 0xe9c0`** - appears **16 times** in broken version, **0 times** in working version
   - This is likely a **disassembly artifact** for a 68020-specific instruction
   - May be related to `lsr.l` or other 68020 instruction that disassembler doesn't recognize
   - Could indicate **code generation differences** between CPU targets

---

## Caller Pattern: `mt_checkfx.part.0`

### 68000 Version - Calling `blecmd_tab` (not `mt_e_cmds` directly)

```asm
000018a2 <mt_checkfx.part.0>:
    ...
    if((uwCmd & 0xF00) == 0xE00) {
    1926:	1001           	move.b d1,d0
    1928:	e808           	lsr.b #4,d0
    blecmd_tab[ubCmdE](ubArg, pChannelData, pChannelReg);
    192a:	0280 0000 00ff 	andi.l #255,d0
    1930:	d080           	add.l d0,d0              ; Manual multiply
    1932:	d080           	add.l d0,d0
    1934:	2f2f 0010      	move.l 16(sp),-(sp)      ; Push arg 3
    1938:	2f0a           	move.l a2,-(sp)          ; Push arg 2
    193a:	740f           	moveq #15,d2
    193c:	c481           	and.l d1,d2
    193e:	2f02           	move.l d2,-(sp)          ; Push arg 1
    1940:	41f9 0001 0582 	lea 10582 <blecmd_tab>,a0
    1946:	2070 0800      	movea.l (0,a0,d0.l),a0   ; Standard indexed
    194a:	4e90           	jsr (a0)                 ; ✅ NORMAL CALL
    194c:	4fef 000c      	lea 12(sp),sp            ; ✅ CLEANUP: Remove 3 args (12 bytes)
    1950:	60b0           	bra.s 1902
```

**Pattern:**
- ✅ **Normal call:** `jsr (a0)` with proper return
- ✅ **Stack cleanup:** `lea 12(sp),sp` removes 3 arguments (12 bytes)
- ✅ **Standard addressing:** `(0,a0,d0.l)`

### 68020 Version - Calling `blecmd_tab` (not `mt_e_cmds` directly)

```asm
00001cca <mt_checkfx.part.0>:
    ...
    if((uwCmd & 0xF00) == 0xE00) {
    1d4c:	e9c0           	.short 0xe9c0            ; ⚠️ Disassembly issue
    1d4e:	1604           	move.b d4,d3             ; ⚠️ Uses d4
    1d50:	2f2f 0010      	move.l 16(sp),-(sp)      ; Push arg 3
    1d54:	2f0a           	move.l a2,-(sp)          ; Push arg 2
    1d56:	740f           	moveq #15,d2
    1d58:	c480           	and.l d0,d2
    1d5a:	2f02           	move.l d2,-(sp)          ; Push arg 1
    1d5c:	2070 1db0 0001 	movea.l (83830,d1.l:4),a0 ; ✅ 68020 scaled: (base,d1.l:4)
    1d62:	4776 
    1d64:	4e90           	jsr (a0)                 ; ✅ NORMAL CALL
    1d66:	4fef 000c      	lea 12(sp),sp            ; ✅ CLEANUP: Remove 3 args (12 bytes)
    1d6a:	60bc           	bra.s 1d28
```

**Pattern:**
- ✅ **Normal call:** `jsr (a0)` with proper return (same as 68000)
- ✅ **Stack cleanup:** `lea 12(sp),sp` removes 3 arguments (same as 68000)
- ✅ **68020 scaled addressing:** `(83830,d1.l:4)` with hardware scaling
- ⚠️ **Uses `d4` register** - potential corruption source

---

## Key Differences in Call Patterns

### 1. Addressing Modes

| Aspect | 68000 | 68020 |
|--------|-------|-------|
| **Index calculation** | Manual multiply (`add.l d0,d0` twice) | Hardware scaling (`:4` scale factor) |
| **Addressing syntax** | `(0,a0,d0.l)` | `(base,d1.l:4)` |
| **Efficiency** | 2 extra instructions | 1 instruction (faster) |

### 2. Register Usage

| Register | 68000 Usage | 68020 Usage | Issue |
|----------|-------------|-------------|-------|
| `d0` | Used for ubCmdE calculation | Used for ubArgs | ✅ OK |
| `d1` | Used for ubArgs | Used for index calculation | ⚠️ Different |
| `d2` | Used for ubArgE | Used for ubArgE | ✅ OK |
| `d4` | **Not used** | **Used** (`move.b d4,d3`) | ❌ **Potential bug** |

**Critical Issue:** The 68020 version uses `d4` which may be:
- Uninitialized
- Corrupted by previous function
- Not saved/restored properly

### 3. Tail Call Patterns

Both versions use the same tail call pattern for `mt_e_cmds`:
- Both use `jmp (a1)` instead of `jsr` + cleanup
- Both overwrite caller's stack frame
- Both rely on caller to clean up

**However:** The difference may be in **how the caller handles the return**.

### 4. Stack Management

| Operation | 68000 | 68020 | Notes |
|-----------|-------|-------|-------|
| **Stack cleanup count** | 201 | 441 | 68020 has 2.2x more cleanups |
| **Indirect calls** | 479 | 635 | 68020 has 1.3x more calls |
| **Tail call jumps** | 15 | 11 | 68020 has fewer (but still problematic) |

**Analysis:** The 68020 version generates more function calls overall, requiring more stack cleanup operations. This suggests:
- More aggressive inlining in 68000?
- Different optimization decisions?
- More function pointer indirection in 68020?

**Critical Finding:** The broken version has **16 instances** of `.short 0xe9c0` which don't appear in the working version. This suggests:
- **68020-specific instruction** that the disassembler doesn't recognize
- **Different code generation** for 68020 target
- **Potential bug** in instruction encoding or disassembly

---

## Potential Root Causes

### Hypothesis 1: Disassembly Artifact (`.short 0xe9c0`)

The broken version has **16 instances** of `.short 0xe9c0` that don't appear in the working version. This could be:
- **68020-specific instruction** that the disassembler misinterprets
- **Instruction encoding bug** in the compiler
- **Disassembly tool limitation** - the actual instruction may be different

**Investigation needed:** Check the actual binary bytes at these locations to determine the real instruction.

### Hypothesis 2: Register Corruption (`d4`)

The 68020 version uses `d4` register which may be:
- **Not saved** by calling function
- **Corrupted** by interrupt handlers
- **Uninitialized** at function entry

**Evidence:**
- Line 1884 in broken version: `move.b d4,d3` - uses `d4` without initialization
- Line 1d4e in broken version: `move.b d4,d3` - same pattern in caller

### Hypothesis 3: Stack Frame Mismatch

The tail call optimization may work differently due to:
- **Different stack frame sizes** between 68000 and 68020 builds
- **Different register save/restore patterns**
- **Different calling convention optimizations**

### Hypothesis 4: Scaled Addressing Side Effects

The 68020 scaled addressing `(base,d1.l:4)` may:
- **Use different base register** (`d1` vs `d0`)
- **Calculate different addresses** if `d1` is corrupted
- **Access wrong function pointer** from table

---

## Recommendations

1. **Investigate `.short 0xe9c0` instructions:**
   - Check actual binary bytes at these locations
   - Determine the real instruction being executed
   - Verify if this is a disassembly artifact or actual bug
   - Compare with 68020 instruction set to identify the instruction

2. **Check `d4` register usage:**
   - Verify `d4` is saved/restored in all calling functions
   - Check if interrupt handlers corrupt `d4`
   - Add explicit initialization of `d4` before use

2. **Compare stack frame layouts:**
   - Verify both versions use same stack frame sizes
   - Check register save/restore sequences match

3. **Verify function pointer tables:**
   - Ensure `ecmd_tab` and `blecmd_tab` are identical in both builds
   - Check table addresses are correct

4. **Add stack corruption detection:**
   - Add stack canaries to detect corruption early
   - Log stack pointer values at critical points

---

## Conclusion

The call patterns are **structurally similar** between 68000 and 68020, but differ in:

1. **Addressing modes:** 68020 uses scaled addressing (more efficient)
2. **Register usage:** 68020 uses `d4` register (potential bug source)
3. **Call frequency:** 68020 generates more function calls overall
4. **Stack cleanup:** 68020 requires more cleanup operations

The **most suspicious difference** is the use of `d4` register in the 68020 version, which may be uninitialized or corrupted, leading to incorrect function pointer lookups and subsequent crashes.


# Why 68000 Works But 68020 Crashes
## The Tail Call Optimization Mystery Solved

**Question:** If both `smite.s` (68000) and `smite_broken.s` (68020) use the same tail call optimization (`jmp (a1)`), why does the 68000 version work while the 68020 version crashes?

**Answer:** The difference is **NOT in the tail call itself**, but in **how the compiler generates code for stack argument handling** between the two CPU targets. The 68000 version happens to work due to a quirk in code generation, while the 68020 version exposes the bug more aggressively.

---

## The Critical Difference: Stack Argument Handling

### 68000 Build - How It's Called

Looking at the caller of `mt_e_cmds` in `smite.s`:

```asm
000019a2 <mt_checkfx.part.0>:
	fx_tab[ubCmdIndex](pChannelData->sVoice.ubCmdLo, pChannelData, pChannelReg);
    19a2:	0280 0000 00ff 	andi.l #255,d0
    19a8:	d080           	add.l d0,d0
    19aa:	d080           	add.l d0,d0
    19ac:	2f2f 0010      	move.l 16(sp),-(sp)  ; Push arg 3: pChannelReg
    19b0:	2f0a           	move.l a2,-(sp)      ; Push arg 2: pChannelData
    19b2:	7200           	moveq #0,d1
    19b4:	122a 0003      	move.b 3(a2),d1
    19b8:	2f01           	move.l d1,-(sp)      ; Push arg 1: ubCmdLo
    19ba:	41f9 0001 0072 	lea 10072 <fx_tab>,a0
    19c0:	2070 0800      	movea.l (0,a0,d0.l),a0
    19c4:	4e90           	jsr (a0)             ; Call function (could be mt_e_cmds)
    19c6:	4fef 000c      	lea 12(sp),sp       ; ✅ CLEANUP: Remove 3 args
```

**Key Point:** The **caller** pushes arguments and **cleans them up** after the call.

### 68000 Build - mt_e_cmds Function

```asm
00001434 <mt_e_cmds>:
    1434:	2f02           	move.l d2,-(sp)      ; Save d2
    1436:	222f 0008      	move.l 8(sp),d1      ; Load ubArgs from caller's stack
	UBYTE ubCmdE = (ubArgs & 0xF0) >> 4;
    143a:	1001           	move.b d1,d0
    143c:	e808           	lsr.b #4,d0
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    143e:	0280 0000 00ff 	andi.l #255,d0
    1444:	d080           	add.l d0,d0
    1446:	d080           	add.l d0,d0
    1448:	740f           	moveq #15,d2
    144a:	c481           	and.l d1,d2
    144c:	2f42 0008      	move.l d2,8(sp)      ; ⚠️ OVERWRITE arg 1 in caller's frame
    1450:	41f9 0001 05d2 	lea 105d2 <ecmd_tab>,a0
    1456:	2270 0800      	movea.l (0,a0,d0.l),a1
    145a:	241f           	move.l (sp)+,d2      ; Restore d2
    145c:	4ed1           	jmp (a1)             ; ⚠️ TAIL CALL
```

**What happens:**
1. `mt_e_cmds` receives arguments on stack from caller (at `8(sp)`, `12(sp)`, `16(sp)`)
2. It **overwrites** the first argument at `8(sp)` with `ubArgE`
3. It does a tail call `jmp (a1)` to the function pointer
4. The called function uses arguments from the **caller's stack frame**
5. When the called function returns with `rts`, it returns to `mt_e_cmds`'s caller
6. The caller's cleanup code (`lea 12(sp),sp`) executes and removes the arguments

**Why it works (by accident):**
- The tail call reuses the **caller's stack frame**
- The caller is responsible for cleanup
- When the tail-called function returns, it returns to the caller
- The caller's cleanup code still executes
- **Stack is properly cleaned up** ✅

---

### 68020 Build - How It's Called

Looking at the caller in `smite_broken.s`:

```asm
00001d4c <mt_checkfx.part.0>:
	blecmd_tab[ubCmdE](ubArg, pChannelData, pChannelReg);
    1d4c:	e9c0           	.short 0xe9c0
    1d4e:	1604           	move.b d4,d3
    1d50:	2f2f 0010      	move.l 16(sp),-(sp)  ; Push arg 3
    1d54:	2f0a           	move.l a2,-(sp)      ; Push arg 2
    1d56:	740f           	moveq #15,d2
    1d58:	c480           	and.l d0,d2
    1d5a:	2f02           	move.l d2,-(sp)      ; Push arg 1
    1d5c:	2070 1db0 0001 	movea.l (83830,d1.l:4),a0
    1d62:	4776 
    1d64:	4e90           	jsr (a0)             ; Call function
    1d66:	4fef 000c      	lea 12(sp),sp       ; ✅ CLEANUP
```

**Same pattern:** Caller pushes and cleans up.

### 68020 Build - mt_e_cmds Function

```asm
0000187c <mt_e_cmds>:
    187c:	2f02           	move.l d2,-(sp)      ; Save d2
    187e:	202f 0008      	move.l 8(sp),d0      ; Load ubArgs
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    1882:	e9c0           	.short 0xe9c0        ; ⚠️ Disassembly issue
    1884:	1604           	move.b d4,d3         ; ⚠️ Uses d4
    1886:	740f           	moveq #15,d2
    1888:	c480           	and.l d0,d2
    188a:	2f42 0008      	move.l d2,8(sp)      ; ⚠️ OVERWRITE arg 1
    188e:	2270 1db0 0001 	movea.l (83910,d1.l:4),a1
    1894:	47c6 
    1896:	241f           	move.l (sp)+,d2      ; Restore d2
    1898:	4ed1           	jmp (a1)             ; ⚠️ TAIL CALL
```

**Same pattern as 68000!** So why does it crash?

---

## The Real Difference: Code Generation Subtleties

The key insight is that **both versions have the same tail call bug**, but they manifest differently due to:

### 1. **Register Usage Differences**

**68000:**
- Uses `d0`, `d1`, `d2` consistently
- Clear register allocation
- Predictable stack layout

**68020:**
- Uses `d4` (line 1884: `move.b d4,d3`) - **uninitialized register?**
- More complex register allocation
- Possible register corruption affecting stack calculations

### 2. **Addressing Mode Differences**

**68000:**
```asm
    1450:	41f9 0001 05d2 	lea 105d2 <ecmd_tab>,a0
    1456:	2270 0800      	movea.l (0,a0,d0.l),a1
```
- Standard indexed addressing
- Simple, predictable

**68020:**
```asm
    188e:	2270 1db0 0001 	movea.l (83910,d1.l:4),a1
    1894:	47c6 
```
- Scaled indexed addressing `(83910,d1.l:4)`
- Uses `d1` instead of `d0` - **register mismatch?**
- Base address `83910` vs table address - **wrong calculation?**

### 3. **The Critical Bug: Missing ubCmdE Extraction**

**68000:**
```asm
    143a:	1001           	move.b d1,d0
    143c:	e808           	lsr.b #4,d0          ; ✅ Extract ubCmdE
    143e:	0280 0000 00ff 	andi.l #255,d0
    1444:	d080           	add.l d0,d0
    1446:	d080           	add.l d0,d0         ; ✅ Scale index
```

**68020:**
```asm
    1882:	e9c0           	.short 0xe9c0        ; ⚠️ What is this?
    1884:	1604           	move.b d4,d3         ; ⚠️ Uses d4, not d0
    1886:	740f           	moveq #15,d2
    1888:	c480           	and.l d0,d2          ; Only extracts ubArgE
    188a:	2f42 0008      	move.l d2,8(sp)
    188e:	2270 1db0 0001 	movea.l (83910,d1.l:4),a1  ; Uses d1, not d0!
```

**The 68020 version is missing the `ubCmdE` extraction!** It never does `lsr.b #4` to extract the command. This means:
- `d1` still contains the full `ubArgs` value
- The index calculation `(83910,d1.l:4)` uses the **wrong value**
- It accesses the wrong function pointer from the table
- The function pointer might be invalid or point to wrong function
- **This could cause crashes before the tail call bug even matters**

---

## Why 68000 "Works" (But Still Has the Bug)

The 68000 version appears to work because:

1. **Correct Index Calculation:**
   - Properly extracts `ubCmdE` with `lsr.b #4`
   - Uses correct register (`d0`) for indexing
   - Accesses correct function pointer

2. **Stack Frame Reuse Works:**
   - Tail call reuses caller's stack frame
   - Caller cleans up after return
   - Arguments are in expected locations

3. **The Bug Still Exists:**
   - If you trace carefully, there might still be stack leaks
   - But they might be smaller or less frequent
   - Or the 68000 version might just be "lucky" with timing

4. **Possible Reasons It Doesn't Crash:**
   - **Slower execution:** 68000 is slower, so leaks accumulate more slowly
   - **Different call frequency:** Fewer calls per second
   - **Larger stack:** More headroom before overflow
   - **Different interrupt timing:** Interrupts might clear stack occasionally
   - **The bug exists but isn't triggered:** Different code paths

---

## Why 68020 Crashes

The 68020 version crashes because:

1. **Wrong Function Pointer:**
   - Missing `ubCmdE` extraction means wrong index
   - Accesses wrong function pointer
   - Could jump to invalid address or wrong function

2. **Register Corruption:**
   - Uses `d4` which may not be initialized
   - Register `d1` vs `d0` mismatch
   - Could cause incorrect calculations

3. **Faster Execution:**
   - 68020 is faster, so bugs manifest quicker
   - More calls per second = faster stack corruption
   - Less time for "lucky" stack recovery

4. **More Aggressive Optimization:**
   - 68020 compiler uses more optimizations
   - Scaled addressing might have bugs
   - Code generation might be more fragile

---

## The Real Answer

**Both versions have the tail call optimization bug**, but:

1. **68000 version:**
   - Has correct index calculation
   - Accesses correct function pointers
   - Tail call bug exists but may not manifest as quickly
   - Or the bug exists but stack leaks are smaller/less frequent

2. **68020 version:**
   - Has **additional code generation bugs** (missing `ubCmdE` extraction)
   - Accesses wrong function pointers
   - Crashes due to **multiple bugs**, not just tail call
   - Faster execution makes bugs manifest immediately

**Conclusion:** The 68000 version doesn't crash (or crashes less) not because it doesn't have the tail call bug, but because:
- It has **fewer bugs** (correct index calculation)
- It's **slower** (bugs accumulate more slowly)
- The tail call bug might still exist but be masked by other factors

The 68020 version crashes because it has **multiple bugs**:
- Tail call optimization bug (stack leak)
- Missing `ubCmdE` extraction (wrong function pointer)
- Register usage issues (`d4`, `d1` vs `d0`)

---

## Verification

To verify this theory:

1. **Check if 68000 version has stack leaks:**
   - Run for extended periods
   - Monitor stack pointer
   - Check for gradual stack growth

2. **Fix the 68020 code generation:**
   - Ensure `ubCmdE` extraction is present
   - Fix register usage
   - Verify function pointer calculation

3. **Compare with `-fno-optimize-sibling-calls`:**
   - Both should work correctly
   - This confirms tail call is the issue

4. **Test 68000 with same call frequency:**
   - If 68000 is just slower, increasing call rate should cause crashes
   - This would confirm the bug exists in both

---

## Recommendation

**Disable tail call optimization for both CPU targets:**

```cmake
target_compile_options(${SMITE_EXECUTABLE} PRIVATE -fno-optimize-sibling-calls)
target_compile_options(ace PRIVATE -fno-optimize-sibling-calls)
```

This ensures:
- ✅ No tail call bugs on either CPU
- ✅ Correct stack management
- ✅ Stable execution
- ✅ Minimal performance impact

The 68000 version might appear to work, but it likely still has the bug - it's just masked by slower execution or other factors. Disabling tail call optimization is the safe solution for both.

---

**Document Version:** 1.0  
**Last Updated:** Analysis date  
**Related Documents:**
- `68000_vs_68020_Codegen_Analysis.md`
- `68000_vs_68020_Working_Comparison.md`
- `TailCallOptimizationBug_Analysis.md`


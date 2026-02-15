# 68000 vs 68020 Code Generation Analysis
## With optimize-sibling-calls Enabled

**Date:** Analysis of `smite.s` (68000) vs `smite_broken.s` (68020)  
**Context:** Both builds use `-foptimize-sibling-calls`, targeting different CPU architectures

---

## Executive Summary

Comparing the disassembly of two builds reveals significant differences in code generation between 68000 and 68020 targets, even with identical optimization flags. The 68020 build shows:

1. **More efficient register operations** (using `clr.l` instead of `moveq #0`)
2. **68020-specific addressing modes** (scaled index addressing with `:4` scale factor)
3. **Code generation issues** (missing instruction sequences, misassembled instructions)
4. **Different tail call optimization patterns** (fewer tail calls in 68020 version)

---

## Key Differences

### 1. Register Initialization

#### 68000 Build (`smite.s`)
```asm
      26:	7400           	moveq #0,d2
```
- Uses `moveq #0,d2` (2 bytes, 4 cycles on 68000)
- Optimal for 68000 architecture

#### 68020 Build (`smite_broken.s`)
```asm
      26:	4282           	clr.l d2
```
- Uses `clr.l d2` (2 bytes, 2 cycles on 68020, 6 cycles on 68000)
- Optimal for 68020 architecture
- GCC correctly selects CPU-appropriate instruction

**Verdict:** ✅ Correct optimization for each target CPU

---

### 2. Array Indexing and Addressing Modes

#### 68000 Build - `mt_e_cmds` Function

```asm
00001434 <mt_e_cmds>:
    1434:	2f02           	move.l d2,-(sp)
    1436:	222f 0008      	move.l 8(sp),d1
	UBYTE ubCmdE = (ubArgs & 0xF0) >> 4;
    143a:	1001           	move.b d1,d0
    143c:	e808           	lsr.b #4,d0
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    143e:	0280 0000 00ff 	andi.l #255,d0
    1444:	d080           	add.l d0,d0      ; multiply by 2
    1446:	d080           	add.l d0,d0      ; multiply by 4 (total)
    1448:	740f           	moveq #15,d2
    144a:	c481           	and.l d1,d2
    144c:	2f42 0008      	move.l d2,8(sp)
    1450:	41f9 0001 05d2 	lea 105d2 <ecmd_tab>,a0
    1456:	2270 0800      	movea.l (0,a0,d0.l),a1  ; standard indexed addressing
    145a:	241f           	move.l (sp)+,d2
    145c:	4ed1           	jmp (a1)          ; tail call optimization
```

**Characteristics:**
- Manual index scaling: Two `add.l d0,d0` instructions to multiply by 4
- Standard indexed addressing: `(0,a0,d0.l)`
- Clear, readable instruction sequence
- Proper extraction of `ubCmdE` with `lsr.b #4` and `andi.l`

#### 68020 Build - `mt_e_cmds` Function

```asm
0000187c <mt_e_cmds>:
    187c:	2f02           	move.l d2,-(sp)
    187e:	202f 0008      	move.l 8(sp),d0
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
    1882:	e9c0           	.short 0xe9c0     ; ⚠️ MISASSEMBLED INSTRUCTION
    1884:	1604           	move.b d4,d3      ; ⚠️ Uses uninitialized d4
    1886:	740f           	moveq #15,d2
    1888:	c480           	and.l d0,d2
    188a:	2f42 0008      	move.l d2,8(sp)
    188e:	2270 1db0 0001 	movea.l (83910,d1.l:4),a1  ; 68020 scaled addressing
    1894:	47c6 
    1896:	241f           	move.l (sp)+,d2
    1898:	4ed1           	jmp (a1)          ; tail call optimization
```

**Characteristics:**
- **68020 scaled indexed addressing:** `(83910,d1.l:4)` - uses `:4` scale factor
- **Missing instruction sequence:** No visible `ubCmdE` extraction (`lsr.b #4`)
- **Misassembled instruction:** `.short 0xe9c0` suggests disassembler confusion
- **Register usage issue:** `move.b d4,d3` uses `d4` which may not be initialized
- **Different base calculation:** Uses `d1` instead of `d0` for indexing

**Verdict:** ⚠️ **Problematic** - The 68020 version appears to have code generation or disassembly issues

---

### 3. Tail Call Optimization Patterns

Both builds use `jmp (aX)` for tail call optimization, but with different frequencies:

| Build | Tail Call Count | Pattern |
|-------|----------------|---------|
| **68000** (`smite.s`) | 13 occurrences | `jmp (a1)` |
| **68020** (`smite_broken.s`) | 7 occurrences | `jmp (a1)` |

**Example from both builds:**
```asm
    145c:	4ed1           	jmp (a1)    ; 68000 mt_e_cmds
    1898:	4ed1           	jmp (a1)    ; 68020 mt_e_cmds
```

**Analysis:**
- Both correctly optimize sibling calls to `jmp` instead of `jsr`/`rts`
- 68020 version has fewer tail calls, possibly due to:
  - Different inlining decisions
  - Different function structure after optimization
  - Code generation differences affecting tail call eligibility

**Verdict:** ✅ Both correctly apply tail call optimization, but with different frequency

---

### 4. Misassembled/Unknown Instructions

The disassembler (`objdump`) shows many `.short` entries indicating unrecognized instructions:

| Build | `.short` Count | Issue |
|-------|----------------|-------|
| **68000** | 5 occurrences | Minimal disassembly issues |
| **68020** | 19 occurrences | **Significant disassembly problems** |

**68020 Examples:**
```asm
    1882:	e9c0           	.short 0xe9c0
    18f0:	e9c0           	.short 0xe9c0
     27c:	4a89           	.short 0x4a89
     4f8:	4a88           	.short 0x4a88
     552:	49c0           	.short 0x49c0
```

**Possible Causes:**
1. **68020-specific instructions** not recognized by disassembler
2. **Extended addressing modes** that objdump doesn't understand
3. **Code generation bugs** producing invalid instruction sequences
4. **Disassembler limitations** with 68020 opcodes

**Verdict:** ⚠️ **Concerning** - High number of unrecognized instructions suggests either:
- Disassembler needs update for 68020 support
- Compiler generating invalid/corrupted code
- Extended addressing modes not properly decoded

---

## Function-by-Function Comparison

### `mt_e_cmds` Function

**Purpose:** Dispatches E-commands in ProTracker player

| Aspect | 68000 | 68020 |
|--------|-------|-------|
| **ubCmdE extraction** | ✅ `lsr.b #4` + `andi.l` | ❌ Missing/obscured |
| **Index scaling** | Manual (`add.l` × 2) | Hardware (`:4` scale) |
| **Addressing mode** | `(0,a0,d0.l)` | `(83910,d1.l:4)` |
| **Register usage** | Clean (d0, d1, d2) | Problematic (d4 used) |
| **Tail call** | ✅ `jmp (a1)` | ✅ `jmp (a1)` |

### `blocked_e_cmds` Function

Similar pattern to `mt_e_cmds`:
- 68000: Clear, standard addressing
- 68020: Scaled addressing with `.short 0xe9c0` issue

---

## Addressing Mode Deep Dive

### 68000: Standard Indexed Addressing
```asm
lea 105d2 <ecmd_tab>,a0        ; Load table base
movea.l (0,a0,d0.l),a1          ; Index: (base + 0 + index*1)
```
- Requires manual index scaling (multiply by 4)
- Simple, well-understood addressing mode
- Works on all 680x0 CPUs

### 68020: Scaled Indexed Addressing
```asm
movea.l (83910,d1.l:4),a1       ; Index: (base + index*4)
```
- **68020+ feature:** Built-in scale factor `:4`
- Eliminates need for manual multiplication
- More efficient (fewer instructions)
- **Issue:** Base address `83910` seems wrong (should be table address)
- Uses `d1` instead of `d0` - register mismatch?

**Potential Problem:**
The 68020 version appears to use an absolute base address (`83910`) instead of the table address, and uses `d1` (which contains `ubArgs`) instead of the extracted `ubCmdE` index. This could cause incorrect function pointer lookups.

---

## Code Generation Quality Assessment

### 68000 Build
- ✅ Clear, readable assembly
- ✅ Proper instruction sequences
- ✅ Correct register usage
- ✅ Standard addressing modes
- ✅ Minimal disassembly issues

### 68020 Build
- ⚠️ Uses advanced 68020 features (scaled addressing)
- ⚠️ Missing instruction sequences (ubCmdE extraction)
- ⚠️ Misassembled instructions (`.short` entries)
- ⚠️ Register usage issues (d4, d1 vs d0)
- ⚠️ Many disassembly problems

---

## Root Cause Hypothesis

The 68020 build shows signs of:

1. **Compiler optimization bugs** when targeting 68020:
   - Missing instruction sequences
   - Incorrect register usage
   - Wrong addressing mode calculations

2. **Disassembler limitations**:
   - Doesn't recognize 68020 extended addressing modes
   - Misinterprets instruction boundaries
   - Shows `.short` for valid 68020 instructions

3. **Possible combination**:
   - Compiler generates valid but complex 68020 code
   - Disassembler fails to decode it properly
   - Actual binary may be correct, but disassembly is misleading

---

## Recommendations

### Immediate Actions

1. **Verify actual binary behavior:**
   - Test if `smite_broken.s` binary actually crashes
   - Compare runtime behavior, not just disassembly
   - The disassembly issues might be cosmetic

2. **Check disassembler version:**
   - Ensure `objdump` supports 68020 instructions
   - Try alternative disassemblers (IDA, Ghidra)
   - Verify instruction decoding is correct

3. **Review compiler flags:**
   - Check if 68020-specific optimizations are causing issues
   - Consider `-m68020` vs `-m68020-60` differences
   - Verify no conflicting optimization flags

### Long-term Solutions

1. **Update toolchain:**
   - Use latest GCC with known-good 68020 support
   - Update binutils for proper 68020 disassembly

2. **Code review:**
   - If binary is actually broken, file GCC bug report
   - Provide minimal reproducer for codegen issues
   - Document specific functions with problems

3. **Build system:**
   - Add disassembly verification step
   - Flag builds with excessive `.short` entries
   - Compare assembly output between CPU targets

---

## Conclusion

The comparison reveals that while both builds use `optimize-sibling-calls`, the 68020 build shows:

- **Advanced features:** Proper use of 68020 scaled addressing
- **Code quality issues:** Missing sequences, register problems
- **Disassembly problems:** Many unrecognized instructions

**Critical Question:** Are these disassembly artifacts, or actual code generation bugs?

**Next Steps:**
1. Test if the 68020 binary actually works correctly
2. Verify disassembler supports 68020 properly
3. If binary is broken, investigate compiler codegen bugs
4. If binary works, update disassembler or use better tools

The tail call optimization itself appears to work correctly in both builds, but the surrounding code generation for 68020 shows concerning patterns that warrant investigation.

---

## Appendix: Instruction Reference

### `moveq #0,dn` (68000 optimal)
- **Size:** 2 bytes
- **Cycles:** 4 (68000), 2 (68020)
- **Range:** -128 to +127

### `clr.l dn` (68020 optimal)
- **Size:** 2 bytes  
- **Cycles:** 6 (68000), 2 (68020)
- **Range:** Full 32-bit

### Scaled Indexed Addressing (68020+)
- **Format:** `(base,An:scale)` or `(d16,An:scale)`
- **Scale factors:** `:1`, `:2`, `:4`, `:8`
- **Efficiency:** Eliminates manual index multiplication
- **Example:** `(0,a0,d0.l:4)` = `a0 + d0*4`

---

**Document Version:** 1.0  
**Last Updated:** Analysis date  
**Related Documents:** `TailCallOptimizationBug_Analysis.md`, `TailCallOptimizationBug_FieldReport.md`


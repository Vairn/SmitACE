# GCC m68k Tail Call Bug – Field Report

## TL;DR
- Release builds crashed inside `blitManagerCreate`, but the stack was already trashed long before that call.
- GCC’s m68k backend sometimes rewrites indirect tail calls (`jsr` + cleanup) into raw jumps (`jmp`) even when the callee arguments live on the stack.
- Every one of those bogus jumps leaks 8–12 bytes per call. With interrupts and the ProTracker player running, the stack evaporates within milliseconds.
- Adding `-fno-optimize-sibling-calls` to our build stops the transformation and the game runs normally.

---

## How We Stumbled Into It
We were enabling more aggressive optimizations for the 68020 build when the game started crashing during startup. The fault always showed up while `blitManagerCreate` ran, yet the code inside that function looked innocent. Classic red flag: the stack pointer pointed into garbage before `blitManagerCreate` even executed its prologue.

The next clue came from comparing two disassemblies:
- `build/unknown/smite.s` – Release build with default optimizations (crashed).
- `build/unknown/smite_works.s` – Same code but forced to keep sibling calls disabled (stable).

The “bad” file was ~760 lines longer and, more importantly, it contained seven extra `jmp (aX)` instructions at the end of small dispatcher functions. Each of those replaced a normal `jsr` + stack cleanup sequence.

---

## Tail Calls in Human Terms
A tail call is simply “call something else right before you return.” Optimizers like to turn:

```asm
    jsr (a0)
    lea 12(sp),sp
    rts
```

into:

```asm
    jmp (a0)
```

That works only if the current function does not own any extra stack space that needs cleaning up. GCC’s m68k backend misses that detail for some indirect calls, so we jump away while three freshly pushed arguments are still on the stack. The callee eventually executes `rts`, but it returns straight to our caller and never pops those bytes. Instant leak.

---

## Smoking Gun #1: `mt_e_cmds`

`deps/ACE/src/ace/managers/ptplayer.c` contains this tiny dispatcher:

```c
static void mt_e_cmds(
	UBYTE ubArgs, tChannelStatus *pChannelData,
	volatile tChannelRegs *pChannelReg
) {
	UBYTE ubArgE = ubArgs & 0x0F;
	UBYTE ubCmdE = (ubArgs & 0xF0) >> 4;
	ecmd_tab[ubCmdE](ubArgE, pChannelData, pChannelReg);
}
```

When the optimizer behaves, the assembler looks like this (from `smite_works.s`):

```asm
00001748 <mt_e_cmds>:
    1752: 2f2f 0010       move.l 16(sp),-(sp)   ; push pChannelReg
    1756: 2f2f 0010       move.l 16(sp),-(sp)   ; push pChannelData
    175e: 2f02            move.l d2,-(sp)       ; push ubArgE
    1768: 4e90            jsr (a0)
    176a: 4fef 000c       lea 12(sp),sp         ; pop three args
    1770: 4e75            rts
```

The broken build (`smite.s`) skips the pushes entirely, rewrites the parameter slot in-place, and jumps straight to the callee:

```asm
0000174a <mt_e_cmds>:
    1758: 2f42 0008       move.l d2,8(sp)       ; scribble over caller frame
    1764: 241f            move.l (sp)+,d2
    1766: 4ed1            jmp (a1)              ; never reclaims 12 bytes
```

Every E-command dispatched through `ecmd_tab` now leaks 12 bytes. ProTracker hits this path constantly.

---

## Smoking Gun #2: `mt_e8` Callback

The E8 command lets a caller register a C callback. Same pattern:

```c
static void mt_e8(UBYTE ubArg, ...) {
	mt_E8Trigger = ubArg;
	if(s_cbOnE8) {
		s_cbOnE8(ubArg);
	}
}
```

Optimized assembly (bad):

```asm
00001980 <mt_e8>:
    199a: 2f41 0004       move.l d1,4(sp)   ; overwrite original arg slot
    199e: 4ed1            jmp (a1)          ; leaks 4 bytes
```

Safe assembly (good):

```asm
00001996 <mt_e8>:
    19b0: 2f01            move.l d1,-(sp)
    19b2: 4e90            jsr (a0)
    19b4: 588f            addq.l #4,sp
    19b6: 4e75
```

Any game logic using `ptplayerSetE8Callback` would have corrupted the stack the first time it triggered.

---

## Why Interrupts Make It Explode
`systemCreate()` installs the vertical blank (INTB_VERTB) and audio (INTB_AUD0-3) handlers before `blitManagerCreate` runs. Those interrupts fire 50–60 times per second (vertb) plus whenever audio DMA finishes (audX). Many of those handlers bounce through function-pointer tables that live inside ptplayer. Every leaked 8–12 bytes accumulate on the same small system stack. By the time control returns to `blitManagerCreate`, the stack pointer already points into thin air, so any simple `move.l (sp)+,d3` faults immediately.

---

## Other Hot Spots We Found
From diffing the two disassemblies, seven functions showed the same broken tail call:

| Function | Target | Bytes Leaked | Notes |
| --- | --- | --- | --- |
| `mt_e_cmds` | `ecmd_tab[ubCmdE]` | 12 | Runs for every E-command |
| `blocked_e_cmds` | `blecmd_tab[ubCmdE]` | 12 | Same idea, used when channels are blocked |
| `mt_e8` | `s_cbOnE8` | 4 | User callback |
| `checkmorefx` | `morefx_tab[uwCmd]` | 12 | Called every pattern row |
| `morefx_tab` callers (x3) | various | 12 | Several dispatcher helpers |

Any one of these firing in a loop is enough to exhaust a 4–8 KB stack.

---

## Workaround We Ship Today
We simply forbid GCC from doing sibling-call optimizations for this target:

```cmake
# CMakeLists.txt
target_compile_options(${SMITE_EXECUTABLE}
  PRIVATE -fno-optimize-sibling-calls)
```

Side effects:
- Adds an extra `rts` at the end of a few functions.
- No measurable hit to frame time on the 68020 build.
- Allows us to keep the rest of `-O2` optimizations.

---

## Reproducing and Verifying

1. **Build the “broken” binary** (no special flags) and disassemble:
   ```
   cmake -DCMAKE_BUILD_TYPE=Release ..
   ninja
   ```
   Inspect `build/unknown/smite.s` and count trailing `jmp (aX)` patterns.

2. **Build the “fixed” binary**:
   ```
   cmake -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_C_FLAGS="-fno-optimize-sibling-calls" ..
   ninja
   ```
   Compare `smite_works.s`; every dispatcher now ends with `jsr` + cleanup.

3. **Runtime check**: With the flag present the intro runs indefinitely, without it the game dies within seconds once interrupts and ptplayer kick in.

---

## Sharing This Upstream
If (when) we file a GCC bug, the report should mention:
- Toolchain: `m68k-amigaos-gcc`, Release build, 68020 target.
- Source references: `deps/ACE/src/ace/managers/ptplayer.c` (`mt_e_cmds`, `blocked_e_cmds`, `mt_e8`).
- Assembly evidence: the `jsr` → `jmp` rewrite with stack arguments still in place.
- Minimal repro idea: tiny C file that does nothing but push three args, call through a function pointer, and return.

The expected fix inside GCC would be to bail out of tail-call optimization whenever stack arguments differ between caller and callee for indirect calls.

---

## Takeaways for Us
- Keep `-fno-optimize-sibling-calls` in the project settings until GCC proves this is fixed.
- Document the flag in the build instructions so nobody “cleans it up” later.
- When writing new dispatcher wrappers, drop a dummy statement after the call (or capture the return value) to make tail-call removal unattractive, just in case.
- Continue diffing the generated assembly when mysterious stack corruption appears; the disassembler comparison made this bug obvious.

---

## Appendix – Quick Reference Snippets

**Safe call pattern (what we want):**

```asm
    move.l 16(sp),-(sp)  ; arg 3
    move.l 16(sp),-(sp)  ; arg 2
    move.l d2,-(sp)      ; arg 1
    jsr (a0)
    lea 12(sp),sp
    rts
```

**Broken tail call pattern (avoid):**

```asm
    move.l d2,8(sp)      ; overwrite caller frame
    move.l (sp)+,d2
    jmp (a1)             ; leaked stack space forever
```

Spotting that pattern anywhere in `build/unknown/smite.s` is a hint that the compiler reverted to its unsafe optimization and we need to re-check the build flags.


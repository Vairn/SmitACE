#pragma once

#include <ace/types.h>

#ifndef GAME_TIME_TICKS_PER_SEC
/** One wall-clock second in ACE game ticks (VBL frames). PAL 50 Hz; use 60 for NTSC builds. */
#define GAME_TIME_TICKS_PER_SEC 50u
#endif

/** Start or restart the ~1 s period from the current game time (e.g. on level start / fade-in). */
void gameTimeReset1s(void);

/**
 * Non-zero once per ~1 s of game time (uses ACE @c ulGameTicks — pauses when the timer manager is paused).
 */
UBYTE gameTimeCheck1s(void);

/** Same as @ref gameTimeCheck1s but for an arbitrary interval in game ticks (reusable timer slot). */
UBYTE gameTimeCheckPeriod(ULONG *pLast, ULONG ulTicks);

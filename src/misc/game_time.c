#include "game_time.h"

#include <ace/managers/timer.h>

static ULONG s_ulOneSecond;

void gameTimeReset1s(void)
{
	s_ulOneSecond = g_sTimerManager.ulGameTicks;
}

UBYTE gameTimeCheck1s(void)
{
	return timerCheck(&s_ulOneSecond, GAME_TIME_TICKS_PER_SEC);
}

UBYTE gameTimeCheckPeriod(ULONG *pLast, ULONG ulTicks)
{
	return timerCheck(pLast, ulTicks);
}

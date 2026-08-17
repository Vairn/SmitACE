#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>

#include <ace/managers/system.h>
#include <ace/managers/state.h>
#include <ace/managers/ptplayer.h>
#ifndef ACE_HOST
#include <bartman/gcc8_c_support.h>
#endif
#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/dos.h> // Bartman's compiler needs this
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>
#ifdef ACE_HOST
#include <host_os.h>
#endif
#ifdef AMIGA
#include <clib/exec_protos.h> // AvailMem, AllocMem, FreeMem, etc.
#endif

#include "smite.h"
#include "screen.h"
#include "item.h"
#include "monster.h"

tStateManager *g_pStateMachineGame;

#ifdef ACE_HOST
/*
 * Host builds have no real 50 Hz vblank interrupt to bound the game loop the
 * way Amiga hardware does: the loop was advancing logic as fast as the CPU
 * would go. Explicitly pace the frame loop to the emulated display rate so the
 * game's frame-based clock (timerGet / game_time) runs at 1 logic-pass/frame.
 */
static void hostFrameLimit(void) {
	static uint64_t s_nextDeadline = 0;
	uint64_t hz = systemGetVerticalBlankFrequency();
	uint64_t freq = hostOsClockFreq();
	if(!freq) {
		freq = 1000u;
	}
	if(!s_nextDeadline) {
		s_nextDeadline = hostOsClockTicks();
	}
	s_nextDeadline += freq / hz;
	hostOsSleepUntilTicks(s_nextDeadline);
	/* Resync after a long hitch instead of dumping out a burst of catch-up frames. */
	uint64_t now = hostOsClockTicks();
	if(now >= s_nextDeadline + freq / hz) {
		s_nextDeadline = now;
	}
}
#endif

ULONG memGetFastSize(void) {
    return AvailMem(MEMF_FAST);
}

void genericCreate(void) {

    //warpmode(0);
    g_pStateMachineGame = stateManagerCreate();
    keyCreate();
    mouseCreate(MOUSE_PORT_1);
    ptplayerCreate(1);
    // Optional: Set initial state. If not set, stateProcess() will safely do nothing.
     stateChange(g_pStateMachineGame, &g_sStateLogo);
    systemUnuse();
    //ULONG memFree = memGetFastSize();
    //AllocateCommandList();
}

void genericProcess(void) {

    ptplayerProcess();
    keyProcess();
    mouseProcess();
    stateProcess(g_pStateMachineGame);
#ifdef ACE_HOST
    hostFrameLimit();
#endif
}

void genericDestroy(void) {
    stateManagerDestroy(g_pStateMachineGame);
    ScreenClose();
    itemSystemDestroy();
    monsterTableClear();
    ptplayerDestroy();
    keyDestroy();
    mouseDestroy();
}
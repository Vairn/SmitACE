#include "smite.h"

#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>

#include <ace/managers/system.h>
#include <ace/managers/state.h>
#include <ace/managers/ptplayer.h>
#include <bartman/gcc8_c_support.h>
#include <proto/exec.h> // Bartman's compiler needs this
#include <proto/dos.h> // Bartman's compiler needs this
#include <ace/managers/memory.h>
#include <ace/managers/system.h>
#include <ace/managers/log.h>

#ifdef AMIGA
#include <clib/exec_protos.h> // AvailMem, AllocMem, FreeMem, etc.
#endif
tStateManager *g_pStateMachineGame;

ULONG memGetFastSize(void) {
	return AvailMem(MEMF_FAST);
}

void genericCreate(void) {

	warpmode(0);
	g_pStateMachineGame = stateManagerCreate();
	keyCreate();
    mouseCreate(MOUSE_PORT_1);
	ptplayerCreate(1);
	stateChange(g_pStateMachineGame, &g_sStateTitle);
	systemUnuse();
	//ULONG memFree = memGetFastSize();
	//AllocateCommandList();
}

void genericProcess(void) {

	ptplayerProcess();
	keyProcess();
    mouseProcess();
	stateProcess(g_pStateMachineGame);
}

void genericDestroy(void) {
	ptplayerDestroy();
	keyDestroy();
    mouseDestroy();
	stateManagerDestroy(g_pStateMachineGame);
}
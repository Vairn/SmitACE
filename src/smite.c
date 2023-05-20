#include "smite.h"

#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>

#include <ace/managers/system.h>
#include <ace/managers/state.h>
#include <ace/managers/ptplayer.h>
tStateManager *g_pStateMachineGame;


void genericCreate(void) {

	g_pStateMachineGame = stateManagerCreate();
	keyCreate();
    mouseCreate(MOUSE_PORT_1);
	ptplayerCreate(1);
	stateChange(g_pStateMachineGame, &g_sStateGame);
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
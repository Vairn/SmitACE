#include "smite.h"

#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>

#include <ace/managers/system.h>
#include <ace/managers/state.h>
#include <ace/managers/ptplayer.h>
#include <gcc8_c_support.h>
tStateManager *g_pStateMachineGame;



void genericCreate(void) {

	warpmode(0);
	g_pStateMachineGame = stateManagerCreate();
	keyCreate();
    mouseCreate(MOUSE_PORT_1);
	ptplayerCreate(1);
	stateChange(g_pStateMachineGame, &g_sStateTitle);
	systemUnuse();
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
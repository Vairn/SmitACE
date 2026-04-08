#include "smite.h"
#include "screen.h"

#include <ace/managers/key.h>
#include <ace/managers/joy.h>
#include <ace/managers/mouse.h>

static void pausedGsCreate(void) {}

static void pausedGsDestroy(void) {}

static void pausedGsLoop(void)
{
	if (keyUse(KEY_F9) || keyUse(KEY_ESCAPE))
		statePop(g_pStateMachineGame);
	if (joyUse(JOY1 + JOY_FIRE) || joyUse(JOY2 + JOY_FIRE))
		statePop(g_pStateMachineGame);
	if (mouseUse(MOUSE_PORT_1, MOUSE_LMB))
		statePop(g_pStateMachineGame);
	ScreenUpdate();
}

tState g_sStatePaused = {
	.cbCreate = pausedGsCreate,
	.cbLoop = pausedGsLoop,
	.cbDestroy = pausedGsDestroy,
};

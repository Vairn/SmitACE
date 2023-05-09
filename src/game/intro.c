#include "intro.h"
#include "smite.h"

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/palette.h>
#include <ace/managers/ptplayer.h>

#include "screen.h"
static void introGsCreate(void) 
{

	logBlockBegin("logoGsCreate()");
	tScreen* pScreen = ScreenGetActive();
	systemUnuse();
	
}

static void introGsLoop(void)
{
    if (keyUse(KEY_RETURN) || keyUse(KEY_ESCAPE) || keyUse(KEY_SPACE) ||
		keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		joyUse(JOY1 + JOY_FIRE) || joyUse(JOY2 + JOY_FIRE))
    {
       stateChange(g_pStateMachineGame, &g_sStateTitle);
      //  statePush(g_pStateMachineGame, &g_sStateTitle);
    }

    ScreenUpdate();
}

static void introGsDestroy(void)
{
    // unload stuff here
}


tState g_sStateIntro = {
	.cbCreate = introGsCreate, .cbLoop = introGsLoop, .cbDestroy = introGsDestroy
};
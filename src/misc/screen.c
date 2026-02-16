#include "screen.h"

#include <mini_std/stdlib.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/macros.h>
#include <ace/managers/sprite.h>

#include "mouse_pointer.h"

tScreen *g_pCurrentScreen = NULL;

tScreen *CreateNewScreen(BYTE palCount)
{

    tScreen *pNewScreen = memAllocFastClear(sizeof(tScreen));
    if (pNewScreen)
    {

        pNewScreen->_pView = viewCreate(0,
                                        TAG_VIEW_GLOBAL_PALETTE, 1,
                                        TAG_VIEW_USES_AGA, 1,
                                        TAG_END);

        pNewScreen->_pVp = vPortCreate(0,
                                       TAG_VPORT_BPP, 8,
                                       TAG_VPORT_USES_AGA, 1,
                                       TAG_VPORT_FMODE,3,
                                       TAG_VPORT_VIEW, pNewScreen->_pView,
                                       TAG_END);

        pNewScreen->_pBfr = simpleBufferCreate(0,
                                               TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR |BMF_INTERLEAVED,
                                               TAG_SIMPLEBUFFER_VPORT, pNewScreen->_pVp,
                                               TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                               TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
                                               TAG_END);

        pNewScreen->_pFade = fadeCreate(pNewScreen->_pView, pNewScreen->_pView->pFirstVPort->pPalette, 255);

    spriteManagerCreate(pNewScreen->_pView, 0, NULL);
    systemSetDmaBit(DMAB_SPRITE, 1);
    
	mouse_pointer_create("data/pointers.bm");

        return pNewScreen;
    }

    timerCreate();
    return 0;
}
void ScreenMakeActive(tScreen *pScreen)
{
    g_pCurrentScreen = pScreen;

    viewLoad(pScreen->_pView);
}

tScreen *ScreenGetActive(void)
{
    ////llplli764cv    4] <-- this is a secret message from the future, Alistair's first Code. aww.

    if (g_pCurrentScreen == 0)
    {

        UBYTE ubSystemUsed = systemIsUsed();
        g_pCurrentScreen = CreateNewScreen(0);
        if (ubSystemUsed)
        {
            systemUnuse();
        }

        viewLoad(g_pCurrentScreen->_pView);

        if (ubSystemUsed)
        {
            systemUse();
        }
    }

    return g_pCurrentScreen;
}

void ScreenUpdate(void)
{
    logWrite("ScreenUpdate");
    systemSetDmaBit(DMAB_SPRITE, 1);

    mouse_pointer_update();
    // int k = 0;
    // for(int i = 0; i < 10000; i++)
    // {
    //     k++;
    //     // do nothing
    // }
    if (g_pCurrentScreen->_pFade->eState != FADE_STATE_IDLE)
    {
        fadeProcess(g_pCurrentScreen->_pFade);
    }
    viewProcessManagers(g_pCurrentScreen->_pView);
    copProcessBlocks();
    // systemIdleBegin();
    vPortWaitUntilEnd(g_pCurrentScreen->_pVp);
    timerProcess();
    //	systemIdleEnd();
}

void ScreenFadeFromBlack(UWORD *pal, LONG delay, void *upFunc)
{
    logWrite("ScreenFadeFromBlack");
    if (g_pCurrentScreen->_pFade->eState != FADE_STATE_IN)
    {
        fadeSet(g_pCurrentScreen->_pFade, FADE_STATE_IN, delay, upFunc);
    }
}

void ScreenFadeToBlack(UWORD *pal, LONG delay, void *upFunc)
{
    logWrite("ScreenFadeToBlack");
    if (g_pCurrentScreen->_pFade->eState != FADE_STATE_OUT)
    {
        fadeSet(g_pCurrentScreen->_pFade, FADE_STATE_OUT, delay, upFunc);
    }
}

void ScreenClose()
{
    tScreen *pScreen = g_pCurrentScreen;
    g_pCurrentScreen = 0;
    if (pScreen)
    {
        systemSetDmaBit(DMAB_SPRITE, 0);

        mouse_pointer_destroy();
        simpleBufferDestroy(pScreen->_pBfr);
        viewDestroy(pScreen->_pView);
        fadeDestroy(pScreen->_pFade);

        memFree(pScreen, sizeof(tScreen));
    }
}
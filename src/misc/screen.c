#include "screen.h"

#include <stdlib.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/macros.h>
#include <ace/managers/sprite.h>
#include <ace/utils/palette.h>

#include "mouse_pointer.h"

tScreen *g_pCurrentScreen = NULL;

tScreen *CreateNewScreen(BYTE palCount)
{

    tScreen  *pNewScreen = memAllocFastClear(sizeof(tScreen));
    if (pNewScreen)
    {

        pNewScreen->_pView = viewCreate(0,
                                        TAG_VIEW_GLOBAL_PALETTE, 1,
                                        TAG_VIEW_USES_AGA, 1,
                                        TAG_END);

        pNewScreen->_pVp = vPortCreate(0,
                                       TAG_VPORT_BPP, 8,
                                       TAG_VPORT_USES_AGA, 1,
#ifdef ACE_HOST
                                       /* Host Denise FMODE 3 + interleaved fetch still shreds
                                        * 64px columns. OCS-width fetch and planar (non-IL)
                                        * buffers match the .bm assets and the host rasterizer. */
                                       TAG_VPORT_FMODE, 0,
#else
                                       TAG_VPORT_FMODE, 3,
#endif
                                       TAG_VPORT_VIEW, pNewScreen->_pView,
                                       TAG_END);

        pNewScreen->_pBfr = simpleBufferCreate(0,
#ifdef ACE_HOST
                                               TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
#else
                                               TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
#endif
                                               TAG_SIMPLEBUFFER_VPORT, pNewScreen->_pVp,
                                               TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
                                               TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
                                               TAG_END);

        pNewScreen->_pFade = fadeCreate(pNewScreen->_pView, pNewScreen->_pView->pFirstVPort->pPalette, 256);

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

void ScreenSyncFadePaletteToViewport(tScreen *pScreen)
{
    if (!pScreen || !pScreen->_pFade || !pScreen->_pView || !pScreen->_pView->pFirstVPort)
        return;
#ifdef ACE_USE_AGA_FEATURES
    if (pScreen->_pView->pFirstVPort->eFlags & VP_FLAG_AGA) {
        paletteDimAga(
            (ULONG *)pScreen->_pFade->pPaletteRef,
            (ULONG *)pScreen->_pView->pFirstVPort->pPalette,
            pScreen->_pFade->uwColorCount, 255);
    } else
#endif
    {
        UWORD cnt = pScreen->_pFade->uwColorCount;
        if (cnt > 32)
            cnt = 32;
        paletteDimOcs(
            (UWORD *)pScreen->_pFade->pPaletteRef,
            (volatile UWORD *)pScreen->_pView->pFirstVPort->pPalette,
            cnt, 15);
    }
}

void ScreenUpdate(void)
{
    //logWrite("ScreenUpdate");
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
    /* Wait for the next occurrence of the viewport end line (not "already
     * past it"). On the host blit is instant, so isExact=0 returned immediately
     * for most of the frame and the game loop spun unbound. */
    vPortWaitForEnd(g_pCurrentScreen->_pVp);
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
        fadeDestroy(pScreen->_pFade);
        /*
         * VPort managers (including simpleBuffer) are destroyed by viewDestroy()
         * via vPortDestroy()->vPortRmManager(). Destroying them here causes
         * double-free and corrupts the viewport manager list.
         */
        viewDestroy(pScreen->_pView);

        memFree(pScreen, sizeof(tScreen));
    }
}
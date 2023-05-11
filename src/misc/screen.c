#include "screen.h"

#include <mini_std/stdlib.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/system.h>
#include <ace/managers/memory.h>
#include <ace/macros.h>

tScreen* g_pCurrentScreen=NULL;

tScreen* CreateNewScreen(BYTE palCount)
{
 
    tScreen* pNewScreen = memAllocFastClear(sizeof(tScreen));
    if (pNewScreen)
    {
        
        pNewScreen->_pView = viewCreate(0,
            TAG_VIEW_GLOBAL_PALETTE, 1,
            TAG_VIEW_USES_AGA, 1,
        TAG_END);

        pNewScreen->_pVp = vPortCreate(0,
            TAG_VPORT_BPP, 8,
            TAG_VPORT_VIEW, pNewScreen->_pView,
        TAG_END);

		
		pNewScreen->_pBfr = simpleBufferCreate(0,
            TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
            TAG_SIMPLEBUFFER_VPORT, pNewScreen->_pVp,
            TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
            TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
        TAG_END);
 
        pNewScreen->_pFade = fadeCreate(pNewScreen->_pView, pNewScreen->_pView->pFirstVPort->pPalette,255);

        return pNewScreen;
    }

    return 0;
}
void ScreenMakeActive(tScreen* pScreen)
{
    g_pCurrentScreen = pScreen;
    
    viewLoad(pScreen->_pView);
    

}

tScreen* ScreenGetActive(void)
{
    ////llplli764cv    4] <-- this is a secret message from the future, Alistair's first Code. aww.
    
    if (g_pCurrentScreen == 0){
        
        UBYTE ubSystemUsed = systemIsUsed();
        g_pCurrentScreen = CreateNewScreen(0);
        if (ubSystemUsed)
        {
            systemUnuse();
        }
    
		ScreenMakeActive(g_pCurrentScreen);

        if (ubSystemUsed)
        {
            systemUse();
        }
    }

    return g_pCurrentScreen;
}

void ScreenUpdate(void)
{
    //fadeProcess(g_pCurrentScreen->_pFade);
    vPortWaitForEnd(g_pCurrentScreen->_pVp);


}

void ScreenFadeFromBlack(UWORD* pal,LONG delay, void *upFunc) {
	fadeSet(g_pCurrentScreen->_pFade, FADE_STATE_IN, delay,NULL);
    while ( fadeProcess(g_pCurrentScreen->_pFade) != FADE_STATE_IDLE)
    {
         //vPortWaitForEnd(g_pCurrentScreen->_pVp);
    }
}

void ScreenFadeToBlack(UWORD* pal,LONG delay, void *upFunc) {
	
//	ScreenFadePalette( delay, upFunc);
    fadeSet(g_pCurrentScreen->_pFade, FADE_STATE_OUT, delay,NULL);
	while ( fadeProcess(g_pCurrentScreen->_pFade) != FADE_STATE_IDLE)
    {
         //vPortWaitForEnd(g_pCurrentScreen->_pVp);
    }
}

void getFadeParams(UWORD* pal, WORD delay, WORD* pDelayInc, WORD* pDiff )
{
    UBYTE maxDiff = 0;
    UBYTE* scnPal = (UBYTE*)&g_pCurrentScreen->_screenPalette;
	for (WORD i = 0; i < 768; ++i) {
		*pDiff = abs(pal[i] - scnPal[i]);
		maxDiff = MAX(maxDiff, *pDiff);
	}

	*pDelayInc = (delay << 8) & 0x7FFF;
	if (maxDiff != 0)
		*pDelayInc /= maxDiff;

	delay = *pDelayInc;
	for (*pDiff = 1; *pDiff <= maxDiff; ++pDiff) {
		if (*pDelayInc >= 512)
			break;
		*pDelayInc += delay;
	}

}

void ScreenFadePalette(LONG delay, void* pFunc)
{
    if (g_pCurrentScreen)
    {
        LONG diff = 0, delayInc = 0;
	    getFadeParams(&g_pCurrentScreen->_palettes[0], delay, &delayInc, &diff);

    }
}

void ScreenClose()
{
    if (g_pCurrentScreen)
    {
        viewDestroy(g_pCurrentScreen->_pView);
    }
}
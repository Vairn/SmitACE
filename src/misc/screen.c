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

		#ifdef AMIGA
		pNewScreen->_pBfr = simpleBufferCreate(0,
            TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR,
            TAG_SIMPLEBUFFER_VPORT, pNewScreen->_pVp,
            TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
            TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
        TAG_END);
#endif

        
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
    vPortWaitForEnd(g_pCurrentScreen->_pVp);
	viewUpdateCLUT(g_pCurrentScreen->_pView);
}

void ScreenFadeFromBlack(UWORD* pal,LONG delay, void *upFunc) {
	ScreenFadePalette( delay, upFunc);
}

void ScreenFadeToBlack(UWORD* pal,LONG delay, void *upFunc) {
	
	//Palette pal(getPalette(0).getNumColors());
	ScreenFadePalette( delay, upFunc);
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
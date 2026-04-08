/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fade.h"
#include <ace/utils/palette.h>
#include <ace/managers/ptplayer.h>

tFade *fadeCreate(tView *pView, UWORD *pPalette, UBYTE ubColorCount)
{
	tFade *pFade = memAllocFastClear(sizeof(*pFade));
	pFade->eState = FADE_STATE_IDLE;
	pFade->pView = pView;
	pFade->ubColorCount = ubColorCount;
	
	// Check viewport flags for AGA, not view flags (VP_FLAG_AGA != VIEW_FLAG_GLOBAL_AGA)
	if (pView->pFirstVPort->eFlags & VP_FLAG_AGA)
	{
		UWORD uwColorCount = (1 << pView->pFirstVPort->ubBpp);
		pFade->pPaletteRef = memAllocFastClear(sizeof(ULONG) * uwColorCount);
		
		// For AGA, copy as ULONGs (24-bit colors), not UWORDs
		ULONG *pDest = (ULONG *)pFade->pPaletteRef;
		ULONG *pSrc = (ULONG *)pPalette;
		for (UWORD i = 0; i < ubColorCount; ++i)
		{
			pDest[i] = pSrc[i];
		}
	}
	else
	{
		pFade->pPaletteRef = memAllocFastClear(sizeof(UWORD) * 32);
		
		// For OCS/ECS, copy as UWORDs (12-bit colors)
		for (UBYTE i = 0; i < ubColorCount; ++i)
		{
			pFade->pPaletteRef[i] = pPalette[i];
		}
	}
	return pFade;
}

void fadeDestroy(tFade *pFade)
{
	if (!pFade) return;
	// Check viewport flags for AGA, not view flags
	if (pFade->pView->pFirstVPort->eFlags & VP_FLAG_AGA)
	{
		// AGA uses 24 bit palette entries.
		memFree(pFade->pPaletteRef, sizeof(ULONG) * (1 << pFade->pView->pFirstVPort->ubBpp));
	}
	else
	{
		// 12 bit palette entries for Non-AGA
		memFree(pFade->pPaletteRef, sizeof(UWORD) * 32);
	}

	memFree(pFade, sizeof(*pFade));
}

void fadeSet(
	tFade *pFade, tFadeState eState, UBYTE ubFramesToFullFade,
	tCbFadeOnDone cbOnDone)
{
	pFade->eState = eState;
	pFade->ubCnt = 0;
	pFade->ubCntEnd = ubFramesToFullFade;
	pFade->cbOnDone = cbOnDone;
}

tFadeState fadeProcess(tFade *pFade)
{
	if (!pFade) {
		return FADE_STATE_IDLE;
	}
	/* Avoid division by zero and guard against torn-down view (e.g. during shutdown). */
	if (pFade->ubCntEnd == 0 || !pFade->pView || !pFade->pView->pFirstVPort) {
		return pFade->eState;
	}
	tFadeState eState = pFade->eState;
	if (pFade->eState != FADE_STATE_IDLE && pFade->eState != FADE_STATE_EVENT_FIRED)
	{
		++pFade->ubCnt;

		UBYTE ubCnt = pFade->ubCnt;
		if (pFade->eState == FADE_STATE_OUT)
		{
			ubCnt = pFade->ubCntEnd - pFade->ubCnt;
		}

		// Check viewport flags for AGA, not view flags
		if (pFade->pView->pFirstVPort->eFlags & VP_FLAG_AGA)
		{
			UBYTE ubRatio = (255 * ubCnt) / pFade->ubCntEnd;
			paletteDimAGA(
				(ULONG *)pFade->pPaletteRef, (ULONG *)pFade->pView->pFirstVPort->pPalette,
				pFade->ubColorCount, ubRatio);
		}
		else
		{
			UBYTE ubRatio = (15 * ubCnt) / pFade->ubCntEnd;
			paletteDim(
				pFade->pPaletteRef, pFade->pView->pFirstVPort->pPalette,
				pFade->ubColorCount, ubRatio);
		}
		viewUpdateGlobalPalette(pFade->pView);
		UBYTE ubVolume = (64 * ubCnt) / pFade->ubCntEnd;
		ptplayerSetMasterVolume(ubVolume);

		if (pFade->ubCnt >= pFade->ubCntEnd)
		{
			pFade->eState = FADE_STATE_EVENT_FIRED;
			// Save state for return incase fade object gets destroyed in fade cb
			eState = pFade->eState;
			if (pFade->cbOnDone)
			{
				pFade->cbOnDone();
			}
		}
	}
	else
	{
		pFade->eState = FADE_STATE_IDLE;
		eState = pFade->eState;
	}
	return eState;
}

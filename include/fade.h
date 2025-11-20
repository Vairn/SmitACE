/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_FADE_H_
#define _GERMZ_FADE_H_

#include <ace/utils/extview.h>

typedef enum _tFadeState {
	FADE_STATE_IN,
	FADE_STATE_OUT,
	FADE_STATE_PALETTE,
	FADE_STATE_IDLE,
	FADE_STATE_EVENT_FIRED
} tFadeState;

typedef void (*tCbFadeOnDone)(void);

typedef struct _tFade {
	tFadeState eState;  // 4 bytes (enum, int-sized on 68000/68020)
	UBYTE ubColorCount;
	UBYTE ubCnt;
	UBYTE ubCntEnd;
	UBYTE _pad;   // Padding to ensure pointers are 4-byte aligned for 68020+
	// eState (4) + 3 UBYTEs (3) = 7 bytes, need 1 byte to align to 8
	UWORD* pPaletteRef;
	tCbFadeOnDone cbOnDone;
	tView *pView;
} tFade;

tFade *fadeCreate(tView *pView, UWORD *pPalette, UBYTE ubColorCount);

void fadeDestroy(tFade *pFade);

void fadeSet(
	tFade *pFade, tFadeState eState, UBYTE ubFramesToFullFade,
	tCbFadeOnDone cbOnDone
);

/**
 * @brief Processes fade-in or fade-out.
 * @param pFade Fade definition to be processed.
 *
 * @return Current fade state:
 * - FADE_STATE_EVENT_FIRED if fade is done, and OnDone event has just fired.
 * - FADE_STATE_IDLE if fade is done.
 */
tFadeState fadeProcess(tFade *pFade);

#endif // _GERMZ_FADE_H_

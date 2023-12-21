#include "mouse_pointer.h"

#include <ace/managers/sprite.h>
#include <ace/managers/blit.h>
#include <ace/managers/viewport/simplebuffer.h> 
#include <ace/managers/system.h>
#include <ace/managers/mouse.h>

#include "screen.h"

static tBitMap *pointers_low_[MOUSE_MAX_COUNT];
static tBitMap *pointers_high_[MOUSE_MAX_COUNT];
static tSprite *current_pointer0_;
static tSprite *current_pointer1_; // attached sprite.

#define POINTER_WIDTH 16

#define POINTER_HEIGHT 16
#define POINTER_BPP 4
#define SPRITE_BPP 2

void mouse_pointer_create(char const *filepath)
{
    systemUse();
    tBitMap *atlas = bitmapCreateFromFile(filepath, 0);

    for (BYTE idx = 0; idx < MOUSE_MAX_COUNT; idx++)
    {
        // Sprites need to have one extra line above and below the image.
        tBitMap *temp = bitmapCreate(
            POINTER_WIDTH, POINTER_HEIGHT + 2,
            POINTER_BPP, BMF_CLEAR | BMF_INTERLEAVED);

        pointers_high_[idx] = bitmapCreate(
            POINTER_WIDTH, POINTER_HEIGHT + 2,
            SPRITE_BPP, BMF_CLEAR | BMF_INTERLEAVED);

        pointers_low_[idx] = bitmapCreate(
            POINTER_WIDTH, POINTER_HEIGHT + 2,
            SPRITE_BPP, BMF_CLEAR | BMF_INTERLEAVED);

        blitCopyAligned(
            atlas, idx * POINTER_WIDTH, 0,
            temp, 0, 1,
            POINTER_WIDTH, POINTER_HEIGHT);
        
        // Convert the 4bpp bitmap to 2bpp.
        for (UWORD r = 0; r < temp->Rows; r++)
        {
            UWORD offetSrc = r * temp->BytesPerRow;
            UWORD offetDst = r * pointers_low_[idx]->BytesPerRow;
            memcpy(pointers_low_[idx]->Planes[0] + offetDst, temp->Planes[0] + offetSrc, bitmapGetByteWidth(temp));
            memcpy(pointers_low_[idx]->Planes[1] + offetDst, temp->Planes[1] + offetSrc, bitmapGetByteWidth(temp));
            memcpy(pointers_high_[idx]->Planes[0] + offetDst, temp->Planes[2] + offetSrc, bitmapGetByteWidth(temp));
            memcpy(pointers_high_[idx]->Planes[1] + offetDst, temp->Planes[3] + offetSrc, bitmapGetByteWidth(temp));
        }

        bitmapDestroy(temp);
    }

    bitmapDestroy(atlas);

   
    current_pointer0_ = spriteAdd(0, pointers_low_[MOUSE_POINTER]);
    spriteSetEnabled(current_pointer0_, 1);

    current_pointer1_ = spriteAdd(1, pointers_high_[MOUSE_POINTER]);
    spriteSetEnabled(current_pointer1_, 1);
    spriteSetAttached(current_pointer1_, 1);
    systemUnuse();
}

void mouse_pointer_switch(mouse_pointer_t new_pointer)
{
    spriteSetBitmap(current_pointer0_, pointers_low_[new_pointer]);
    spriteSetBitmap(current_pointer1_, pointers_high_[new_pointer]);
}

void mouse_pointer_update(void)
{
    current_pointer0_->wX = mouseGetX(MOUSE_PORT_1);
    current_pointer0_->wY = mouseGetY(MOUSE_PORT_1);
    current_pointer1_->wX = mouseGetX(MOUSE_PORT_1);
    current_pointer1_->wY = mouseGetY(MOUSE_PORT_1);
    spriteRequestMetadataUpdate(current_pointer0_);
    spriteRequestMetadataUpdate(current_pointer1_);

    spriteProcessChannel(0);
    spriteProcessChannel(1);
    spriteProcess(current_pointer0_);
    spriteProcess(current_pointer1_);
}

void mouse_pointer_destroy(void)
{
    for (BYTE idx = 0; idx < MOUSE_MAX_COUNT; idx++)
    {
        bitmapDestroy(pointers_low_[idx]);
        bitmapDestroy(pointers_high_[idx]);
    }

    spriteRemove(current_pointer0_);
    spriteRemove(current_pointer1_);

}

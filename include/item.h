#include <ace/types.h>
typedef struct _tItem
{
    UBYTE ubType;
    UBYTE ubSubType;
    UBYTE ubFlags;
    UBYTE ubModifier; // attack/defense etc.
    UBYTE ubValue;
    UBYTE ubWeight; // How heavy it is. not used in smite atm
    UBYTE ubIcon;
    
} tItem;

extern tItem* s_vecItems;

void loadItems(const char* filename);

tItem* getItem(UBYTE ubIndex);

#ifndef GUARD_MAIN_MENU2_H
#define GUARD_MAIN_MENU2_H

#include "structs/menu.h"
#include "structs/str_text.h"

// size: 0x1A8 (+ the Explorers of Sky transfer fields)
typedef struct MainMenu2Work
{
    u32 unk0;
    MenuStruct unk4[4];
    WindowTemplates unk144;
    /* 0x1A4 */ u32 currMenuChoice;
    // Copy of the current menu with "Transfer" added, see SetMainMenuItemsWithTransfer
    MenuItem transferMenuItems[8];
    WindowTemplate transferMenuWindow;
} MainMenu2Work;

void CleanMainMenu(void);
void DrawMainMenu(void);
u32 UpdateMainMenu(void);

bool8 sub_80363E0(void);

#endif // GUARD_MAIN_MENU2_H
#include "global.h"
#include "constants/colors.h"
#include "constants/main_menu.h"
#include "input.h"
#include "menu_input.h"
#include "music_util.h"
#include "pokemon.h"
#include "sky_transfer_menu.h"
#include "string_format.h"
#include "text_1.h"
#include "text_2.h"
#include "text_3.h"
#include "transfer_password.h"

// Shows the password that carries the hero, partner and money over to Explorers of Sky

#define SKY_TRANSFER_WINDOW_ID 0
#define PASSWORD_CHARS_PER_ROW (TRANSFER_PASSWORD_LENGTH / 2)
#define PASSWORD_CHAR_SPACING 10
#define PASSWORD_X ((26 * 8 - PASSWORD_CHARS_PER_ROW * PASSWORD_CHAR_SPACING) / 2)

static const WindowTemplate sSkyTransferWindowTemplate = {
    .flags = WINTEMPLATE_FLAG_NONE,
    .type = WINDOW_TYPE_NORMAL,
    .pos = { 2, 2 },
    .width = 26,
    .height = 16,
    .totalHeight = 16,
    .unk12 = 0,
    .header = NULL,
};

ALIGNED(4) static const u8 sTitleText[] = _("{COLOR YELLOW}Transfer to Explorers of Sky{RESET}");
ALIGNED(4) static const u8 sHeroText[] = _("Hero: {POKEMON_0}  Lv. {VALUE_0}");
ALIGNED(4) static const u8 sPartnerText[] = _("Partner: {POKEMON_1}  Lv. {VALUE_1}");
ALIGNED(4) static const u8 sMoneyText[] = _("Money: {VALUE_0} {POKE}  Bank: {VALUE_1} {POKE}");
ALIGNED(4) static const u8 sInstructions1Text[] = _("In Explorers of Sky, choose Transfer");
ALIGNED(4) static const u8 sInstructions2Text[] = _("on the Top Menu and enter:");
ALIGNED(4) static const u8 sReturnText[] = _("Press {A_BUTTON} or {B_BUTTON} to go back.");

static void DrawSkyTransferMenu(const TransferData *data, const u8 *password);

// Everything is drawn once here, so this screen needs no work memory
void CreateSkyTransferMenu(void)
{
    WindowTemplates windows;
    TransferData data;
    u8 password[TRANSFER_PASSWORD_LENGTH];

    // Only offered from the main menu when this succeeds, see CanTransferToSky
    BuildTransferData(&data);
    EncodeTransferPassword(&data, password);

    PrintColoredPokeNameToBuffer(gFormatBuffer_Monsters[0], GetLeaderMon2(), COLOR_WHITE);
    PrintColoredPokeNameToBuffer(gFormatBuffer_Monsters[1], GetPartnerMon2(), COLOR_WHITE);

    RestoreSavedWindows(&windows);
    windows.id[SKY_TRANSFER_WINDOW_ID] = sSkyTransferWindowTemplate;
    ResetUnusedInputStruct();
    ShowWindows(&windows, TRUE, TRUE);
    DrawSkyTransferMenu(&data, password);
}

u32 UpdateSkyTransferMenu(void)
{
    if (gRealInputs.pressed & (A_BUTTON | B_BUTTON)) {
        PlayMenuSoundEffect(MENU_SFX_BACK);
        return MENU_MAIN_SCREEN;
    }

    return MENU_NO_SCREEN_CHANGE;
}

void CleanSkyTransferMenu(void)
{
    ResetUnusedInputStruct();
    ShowWindows(NULL, TRUE, TRUE);
}

static void DrawSkyTransferMenu(const TransferData *data, const u8 *password)
{
    s32 i;
    s32 x;
    s32 y;
    u32 color;

    CallPrepareTextbox_8008C54(SKY_TRANSFER_WINDOW_ID);
    sub_80073B8(SKY_TRANSFER_WINDOW_ID);

    PrintStringOnWindow(8, 2, sTitleText, SKY_TRANSFER_WINDOW_ID, 0);

    gFormatArgs[0] = data->heroLevel;
    gFormatArgs[1] = data->partnerLevel;
    PrintFormattedStringOnWindow(8, 18, sHeroText, SKY_TRANSFER_WINDOW_ID, 0);
    PrintFormattedStringOnWindow(8, 30, sPartnerText, SKY_TRANSFER_WINDOW_ID, 0);

    gFormatArgs[0] = data->money;
    gFormatArgs[1] = data->savings;
    PrintFormattedStringOnWindow(8, 42, sMoneyText, SKY_TRANSFER_WINDOW_ID, 0);

    PrintStringOnWindow(8, 60, sInstructions1Text, SKY_TRANSFER_WINDOW_ID, 0);
    PrintStringOnWindow(8, 72, sInstructions2Text, SKY_TRANSFER_WINDOW_ID, 0);

    // Drawn glyph by glyph since '#' is a control character for the string printers.
    // Groups of 4 alternate colors to make the password easier to copy.
    y = 90;
    for (i = 0; i < TRANSFER_PASSWORD_LENGTH; i++) {
        if (i == PASSWORD_CHARS_PER_ROW)
            y += 14;

        color = ((i / 4) % 2) ? COLOR_YELLOW : COLOR_WHITE_2;
        x = PASSWORD_X + (i % PASSWORD_CHARS_PER_ROW) * PASSWORD_CHAR_SPACING;
        DrawCharOnWindowWidth12(x, y, password[i], color, SKY_TRANSFER_WINDOW_ID);
    }

    PrintStringOnWindow(8, 116, sReturnText, SKY_TRANSFER_WINDOW_ID, 0);
    sub_80073E0(SKY_TRANSFER_WINDOW_ID);
}

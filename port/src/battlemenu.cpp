#include "battlemenu.hpp"

#include "clsmes.hpp"
#include "confuse.hpp"
#include "memcard.hpp"
#include "menu_draw.hpp"
#include "menu_manual.hpp"
#include "menu_misc.hpp"
#include "menu_mouse.hpp"
#include "menuetc.hpp"
#include "mglib.hpp"
#include "rect.hpp"
#include "snd.hpp"
#include "texture.hpp"
#include "weaponlevelup.hpp"

// BattleMenuDraw is replaced only to drop retail's VU1 program call; the renderer has no programs.
// The static helpers it calls cannot be reached from here, so they are carried over as retail has
// them, with the locals MWCC left unset at zero as the port's build of ps2/src has them.
namespace {

int GetMenuModeMax() {
    int icon_count = 0;

    switch (BtlMenuMode) {
        case BATTLE_MENU_MODE_DUNGEON:
            icon_count = 7;
            break;
        case BATTLE_MENU_MODE_TOWN:
            icon_count = 8;
            break;
    }

    if (GetGameFlagForManualMenu() == 0) {
        icon_count--;
    }

    return icon_count;
}

void BtlMenuMekeIconInfo(int *icons, int menu_mode) {
    int icon_count = GetMenuModeMax();

    for (int i = 0; i < icon_count; i++) {
        icons[i] = BtlDrawTbl[BtlMenuMode][i];
    }

    if (BtlMenuMode != BATTLE_MENU_MODE_DUNGEON) {
        int special_icon = 0;

        switch (NowGetGameFlagForBtlMenu(BtlMenuMode)) {
            case MENU_MOVE_LOCKED:
            case MENU_MOVE_FIRST_DUNGEON:
                special_icon = 9;
                break;
            case MENU_MOVE_WORLD_MAP:
                special_icon = 8;
                break;
            case MENU_MOVE_INTERIOR_OUT:
                special_icon = 0xC;
                break;
        }

        icons[4] = special_icon;
    }
}

void DrawBtlMenuBar() {
    int alpha;
    int icon_alpha;
    int i;
    int x;
    int menu_flag;
    int brightness;
    int selected;
    int y;

    if (BtlEffectFlag == BTLEFFECT_PAGE_CLOSE || BattleMenuFlag == BTLMENU_STATE_APPEAR) {
        alpha = 10.0f * BtlEffectCt;

        if (alpha >= 128) {
            alpha = 128;
        }
    }

    if (BtlEffectFlag == BTLEFFECT_PAGE_OPEN || BattleMenuFlag == BTLMENU_STATE_EXIT) {
        alpha = 128.0f - 10.0f * BtlEffectCt;

        if (alpha < 0) {
            alpha = 0;
        }
    } else {
        alpha = 128;
    }

    NowGetGameFlagForBtlMenu(BtlMenuMode);
    int icon_count = GetMenuModeMax();
    int icons[8] = {};
    BtlMenuMekeIconInfo(icons, BtlMenuMode);

    for (i = 0; i < icon_count; i++) {
        icon_alpha = alpha;
        brightness = 128;

        if (MenuWarningMsgFlag != 0) {
            brightness = 64;
        }

        menu_flag = BattleMenuFlag;

        if (BTLMENU_STATE_MAIN < menu_flag && menu_flag < BTLMENU_STATE_ITEM_OPEN) {
            icon_alpha = 0;
        }

        selected = 0;
        float icon_x = NorMenuIcon[i].x;
        x = icon_x;
        float icon_y = NorMenuIcon[i].y;
        float lowered_y = 2.0f + icon_y;
        y = 1.0f + lowered_y;

        if (i == MenuSelect[1]) {
            if ((menu_flag == BTLMENU_STATE_APPEAR || menu_flag == BTLMENU_STATE_EXIT) &&
                menu_flag == BTLMENU_STATE_MOVE_OPEN) {
                icon_alpha = alpha;
            }

            selected = 1;
            x = icon_x - 10.0f;
            y = 1.0f + (icon_y - 2.0f);
            icon_alpha = 128;
        }

        if (icons[i] >= 0) {
            DrawMainMenuIcon(x, y, icons[i], selected, brightness, icon_alpha);
        }
    }
}

int BtlMenuDrawSpecialFlag(int flag) {
    if ((BattleMenuFlag == BTLMENU_STATE_ATLA_OPEN || BattleMenuFlag == BTLMENU_STATE_ATLA ||
         BattleMenuFlag == BTLMENU_STATE_ATLA_CLOSE) &&
        GetMenuAtraEventFlag() != 0) {
        flag = 0;
    }

    if (MenuChara.state == MENU_CHARA_CHANGE_LOAD || MenuChara.state == MENU_CHARA_CHANGE_EXIT) {
        flag = 0;
    }

    return flag;
}

char g_frame_image_name[] = "frame_image";

} // namespace

PORT_OVERRIDE void BattleMenuDraw() {
    MenuMouseNoteBattleMenu();
    int text_x = 0;
    int text_y = 0;

    setbilinear(0);
    MenuWorldTrans(&MenuCamera);
    MenuTextureReload(BtlMenuReadBlock);
    CTexture  frame = *TexManager.GetTexture(g_frame_image_name, -1);
    CTexture *texture = &frame;
    int       tcc = 0;

    if (texture != NULL) {
        ((sceGsTex0 *) &frame.tex0)->bits.tcc = tcc;
    } else {
        return;
    }

    int bright = 0x40;

    switch (BattleMenuFlag) {
        case BTLMENU_STATE_APPEAR:
            bright = (int) (128.0f - 4.0f * BtlEffectCt);

            if (bright < 0x40) {
                bright = 0x40;
            }

            break;
        case BTLMENU_STATE_EXIT:
            bright += 4.0f * BtlEffectCt;

            if (bright > 0x80) {
                bright = 0x80;
            }

            break;
    }

    DrawMenu2DSprite(&frame, MenuDispRc, MenuDispRc, bright, bright, bright, 0x80);

    switch (BattleMenuFlag) {
        case BTLMENU_STATE_MAIN:
            DrawBattleMain();
            break;
        case BTLMENU_STATE_CHARA:
        case BTLMENU_STATE_CHARA_OPEN:
        case BTLMENU_STATE_CHARA_CLOSE:
        case BTLMENU_STATE_UNK_1A:
            DrawCharaSelect();
            break;
        case BTLMENU_STATE_WEAPON:
        case BTLMENU_STATE_WEAPON_OPEN:
        case BTLMENU_STATE_WEAPON_CLOSE:
            WeaponMenuDraw();
            break;
        case BTLMENU_STATE_ITEM:
        case BTLMENU_STATE_ITEM_OPEN:
        case BTLMENU_STATE_ITEM_CLOSE:
            ItemMenuModeDraw();
            break;
        case BTLMENU_STATE_ATLA:
        case BTLMENU_STATE_ATLA_OPEN:
        case BTLMENU_STATE_ATLA_CLOSE:
            DrawMenuAtoraSelect();
            break;
        case BTLMENU_STATE_MOVE:
        case BTLMENU_STATE_MOVE_OPEN:
        case BTLMENU_STATE_MOVE_CLOSE:
            DrawMenuMove();
            break;
        case BTLMENU_STATE_OPTION:
        case BTLMENU_STATE_OPTION_OPEN:
        case BTLMENU_STATE_OPTION_CLOSE:
            DrawMenuOption();
            setbilinear(0);
            break;
        case BTLMENU_STATE_SAVE:
        case BTLMENU_STATE_SAVE_OPEN:
        case BTLMENU_STATE_SAVE_CLOSE:
            DrawMenuSave(g_frame_image_name);
            setbilinear(0);
            break;
        case BTLMENU_STATE_MANUAL:
        case BTLMENU_STATE_MANUAL_OPEN:
        case BTLMENU_STATE_MANUAL_CLOSE:
            MenuManualDraw();
            break;
    }

    CursorVibeCnt++;

    if (CursorVibeCnt >= 0x405F7E00) {
        CursorVibeCnt = 0;
    }

    int draw_help = BtlMenuDrawSpecialFlag(BtlWakuMake2);

    if (BtlMenuReadEndFlag != 0) {
        switch (BattleMenuFlag) {
            case BTLMENU_STATE_EXIT:
                BtlHelpWinAlpha -= 8;

                if (BtlHelpWinAlpha < 0) {
                    BtlHelpWinAlpha = 0;
                }

                break;
            default:
                BtlHelpWinAlpha += 9;

                if (BtlHelpWinAlpha > 0x80) {
                    BtlHelpWinAlpha = 0x80;
                }

                break;
        }

        if (draw_help != 0) {
            MenuHelpWinDraw((int) HelpWinHead[0], (int) HelpWinHead[1], BtlHelpWinW, BtlHelpWinH, BtlHelpWinAlpha);
        }
    }

    MenuTextureReload(CommonMenuMes2.tex_block);
    GetMainMenuRightHelpMsgLangOffset(text_x, text_y);
    CommonMenuMes2.text_x = (int) (HelpWinHead[0] + text_x);
    CommonMenuMes2.text_y = (int) (HelpWinHead[1] + text_y);

    if (BtlMenuReadEndFlag != 0 && draw_help != 0) {
        CommonMenuMes2.stay_frame = false;
        CommonMenuMes2.edge_alpha = BtlHelpWinAlpha;
        CommonMenuMes2.Step();
        CommonMenuMes2.DrawMesWin();
    }

    setbilinear(0);
    draw_help = BtlMenuDrawSpecialFlag(1);

    if (draw_help != 0) {
        DrawBtlMenuBar();
    }

    if (WepMenu.mode != WEP_MENU_BUILDUP_SELECT) {
        MenuWepLevelUp.Step();
        MenuWepLevelUp.Draw();
    }

    MenuMouseDrawOverlay();

    if (GetInteriorOutFlag() != 0) {
        BtlEffectCt += 1.0f;
        int fade = (int) (3.0f * BtlEffectCt);

        if (fade > 0x80) {
            fade = 0x80;
        }

        AllFadeForMenu(fade);
    }

    setbilinear(1);
}

// Retail's MenuClsMes::NowWeaponStatus, which lists the names of a weapon's options for its SPECIAL window, with
// Confuse (bit 14) listed after the thirteen retail options under its own name.
PORT_OVERRIDE void MenuClsMes::NowWeaponStatus(WEAPON_HAVE *selected_weapon) {
    weapon = selected_weapon;
    option_flags = 1;
    option_count = 0;

    if (selected_weapon != NULL) {
        option_flags = selected_weapon->flags;

        for (int slot = 0; slot < 6; slot++) {
            ATTACH_LIST *attachment = &weapon->attach[slot];
            s16          attachment_flags = attachment->sphere_flags;

            if (attachment_flags != 0 && attachment_flags != 1) {
                option_flags |= attachment_flags;
            }
        }

        option_flags = CheckWeaponOptionStatus(option_flags);
        option_count = 0;
        int clear_slot;
        int changed = 0;

        for (int bit = 1; bit <= kConfuseOptionBit; bit++) {
            if (option_flags & (1 << bit)) {
                int     message_no = ConfuseOptionMessage(bit);
                ClsMes *window = message;
                int     index = option_count;

                if (message_no != window->mes_no[index]) {
                    changed = 1;
                }

                window->mes_no[index] = message_no;
                option_count++;
            }
        }

        for (clear_slot = option_count; clear_slot < 10; clear_slot++) {
            ClsMes *window = message;
            window->mes_no[clear_slot] = 0;
        }

        if (changed != 0) {
            message->mes_made = -1;
            message->MakeMesWin(422);
        }
    }
}

namespace {

// Shifts a message line's x left by five pixels for each of its characters.
int OptionLineX(int x, int len) {
    return x - len * 5;
}

// Places one line of a message window, ignoring lines past the window's ten.
void OptionLinePos(ClsMes *window, int line, int x, int y) {
    if (line >= 0 && line < 10) {
        window->line_pos[line].x = x;
        window->line_pos[line].y = y;
    }
}

} // namespace

// Retail's MenuClsMes::Draw1, which draws the SPECIAL window's rows, with Confuse (bit 14) drawn as a benefit
// with its own icon.
PORT_OVERRIDE void MenuClsMes::Draw1(int x, int y, int draw_alpha) {
    if (message != NULL && weapon != NULL) {
        int row_y = y;
        int option;

        switch (mode) {
            case MENU_CLSMES_OFF:
                break;
            case MENU_CLSMES_OPTIONS:
            case MENU_CLSMES_UNK_2:
                if (option_flags == 0 || option_flags == 1) {
                    return;
                }

                row_y -= option_count * 10;
                DrawMenu2DSprite(PerBoardTex, CRect_i_(x + 0x1E, row_y - 0x17, 0x52, 0x17), CRect_i_(0x80, 0x68, 0x52, 0x18), alpha);
                DrawMenu2DSprite(PerBoardTex, CRect_i_(x - 0x24, y - 10, 0x20, 0x20), CRect_i_(0x80, 0x80, 0x20, 0x20), alpha);
                CRect_i_  left(0x20, 0x80, 0x20, 0x18);
                CRect_i_  middle(0x40, 0x80, 0x14, 0x18);
                CRect_i_  right(0x78, 0x80, 0x8, 0x18);
                CRect_i_  icon_src(0xEC, 0x50, 0x14, 0x14);
                CTexture *face = TexManager.GetTexture(const_cast<char *>("charaface"), -1);

                for (option = 1; option <= kConfuseOptionBit; option++) {
                    if (option == 8) {
                        icon_src.x -= icon_src.width;
                    }

                    if (option_flags & (1 << option)) {
                        if (IsWeaponOptionGoodOrBad(option)) {
                            left.y = middle.y = right.y = 0x80;
                        } else {
                            left.y = middle.y = right.y = 0x68;
                        }

                        int icon = option;

                        if (option > 7) {
                            icon = option - 7;
                        }

                        if (option == 8 && icon == 1) {
                            icon++;
                        }

                        if (option == 9 && icon == 2) {
                            icon--;
                        }

                        icon_src.y = icon_src.height * (icon - 1) + 0x50;
                        DrawMenu2DSprite(PerBoardTex, CRect_i_(x, row_y, left.width, left.height), left, alpha);
                        DrawMenu2DSprite(PerBoardTex, CRect_i_(x + left.width, row_y, 100, middle.height), middle, alpha);
                        DrawMenu2DSprite(PerBoardTex, CRect_i_(x + left.width + 100, row_y, right.width, right.height), right, alpha);
                        DrawMenu2DSprite(face, CRect_i_(x + 2, row_y + 2, 0x14, 0x15), icon_src, alpha);
                        row_y += left.height;
                    }
                }

                MenuTextureReload(message->tex_block);
                x = (int) (x + 0xB);
                row_y = y - option_count * 10;

                for (option = 0; option < option_count; option++, row_y += left.height) {
                    OptionLinePos(message, option, OptionLineX(x + 0x11, message->GetMesLen_system(message->mes_no[option])), row_y);
                }

                message->edge_alpha = alpha;
                message->DrawMesWin();
                break;
        }
    }
}

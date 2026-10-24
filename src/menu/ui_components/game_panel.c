/**
 * @file game_panel.c
 * @brief Game info panel shown beside narrow lists (file list, favorites, history).
 * @ingroup ui_components
 */

#include <string.h>

#include "../ui_components.h"
#include "../fonts.h"
#include "constants.h"

/** @brief Frames the selection must rest on an item before its info and box art load. */
#define LOAD_DELAY (10)

typedef enum {
    PANEL_STATE_EMPTY,
    PANEL_STATE_PENDING,
    PANEL_STATE_LOADED,
    PANEL_STATE_FAILED,
} panel_state_t;

static int selected_id = -1;
static int load_delay = 0;
static panel_state_t state = PANEL_STATE_EMPTY;
static rom_info_t info;
static component_boxart_t *boxart = NULL;


static void unload (void) {
    if (boxart) {
        ui_components_boxart_free(boxart);
        boxart = NULL;
    }
    if (state == PANEL_STATE_LOADED) {
        rom_info_free_meta(&info);
    }
    state = PANEL_STATE_EMPTY;
}

static const char *format_expansion_pak (rom_expansion_pak_t expansion_pak) {
    switch (expansion_pak) {
        case EXPANSION_PAK_REQUIRED: return "Required";
        case EXPANSION_PAK_RECOMMENDED: return "Recommended";
        case EXPANSION_PAK_SUGGESTED: return "Suggested";
        case EXPANSION_PAK_FAULTY: return "Do not use";
        default: return NULL;
    }
}

void ui_components_game_panel_select (int id) {
    if (id == selected_id) {
        return;
    }

    unload();
    selected_id = id;
    load_delay = LOAD_DELAY;
}

bool ui_components_game_panel_load_due (void) {
    if ((selected_id == -1) || (load_delay == 0)) {
        return false;
    }

    return (--load_delay == 0);
}

void ui_components_game_panel_load (menu_t *menu, path_t *rom_path) {
    unload();

    memset(&info, 0, sizeof(info));
    if (rom_config_load(rom_path, &info) != ROM_OK) {
        rom_info_free_meta(&info);
        state = PANEL_STATE_FAILED;
        return;
    }

    state = PANEL_STATE_LOADED;
    boxart = ui_components_boxart_init(menu->storage_prefix, info.game_code, info.title, IMAGE_BOXART_FRONT);
}

void ui_components_game_panel_draw (const char *name, const char *subtitle, const char *details) {
    char rom_details[512];
    rom_details[0] = '\0';

    if (state == PANEL_STATE_LOADED) {
        if (info.meta.name != NULL && info.meta.name[0] != '\0') {
            name = info.meta.name;
        }
        if (info.meta.release_date != NULL && info.meta.release_date[0] != '\0') {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Released: %s\n", info.meta.release_date);
        }
        if (info.meta.num_players > 0) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Players: %lu\n", info.meta.num_players);
        }
        const char *expansion_pak = format_expansion_pak(info.features.expansion_pak);
        if (expansion_pak != NULL) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Expansion Pak: %s\n", expansion_pak);
        }
        if (info.features.rumble_pak) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Rumble Pak\n");
        }
        details = rom_details;
    } else if (state == PANEL_STATE_FAILED) {
        details = "Unable to read this ROM.\nIt may have been moved or deleted.";
    }

    int text_x = GAME_PANEL_X + TEXT_MARGIN_HORIZONTAL;
    int text_y = VISIBLE_AREA_Y0 + TAB_HEIGHT + TEXT_MARGIN_VERTICAL + TEXT_OFFSET_VERTICAL;

    rdpq_text_printf(
        &(rdpq_textparms_t) {
            .width = VISIBLE_AREA_X1 - text_x - TEXT_MARGIN_HORIZONTAL,
            // stop above the tallest box art (Japanese covers are square)
            .height = BOXART_Y_JP - text_y - TEXT_MARGIN_VERTICAL,
            .wrap = WRAP_WORD,
        },
        FNT_DEFAULT,
        text_x,
        text_y,
        "^%02X%s^00\n"
        "^%02X%s^00\n"
        "\n"
        "%s",
        STL_YELLOW, name ? name : "",
        STL_GRAY, subtitle ? subtitle : "",
        details ? details : ""
    );

    if (boxart != NULL) {
        ui_components_boxart_draw(boxart);
    }
}

void ui_components_game_panel_free (void) {
    unload();
    selected_id = -1;
    load_delay = 0;
}

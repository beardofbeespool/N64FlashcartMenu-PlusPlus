/**
 * @file game_panel.c
 * @brief Game info panel shown beside narrow lists (file list, favorites, history).
 * @ingroup ui_components
 */

#include <string.h>

#include "../ui_components.h"
#include "../fonts.h"
#include "constants.h"

/** @brief Frames (at 30 fps) the selection must rest on an item before its info and box art load. */
#define LOAD_DELAY (15)

/**
 * Loading is split into one small step per frame, so no single frame stalls
 * long enough to make the next button press feel late.
 */
typedef enum {
    PANEL_STATE_EMPTY,
    PANEL_STATE_LOAD_META,      /**< Header loaded, metadata next */
    PANEL_STATE_LOAD_BOXART,    /**< Metadata loaded, box art next */
    PANEL_STATE_LOADED,
    PANEL_STATE_FAILED,
} panel_state_t;

static int selected_id = -1;
static int load_delay = 0;
static panel_state_t state = PANEL_STATE_EMPTY;
static bool info_valid = false;
static bool meta_loaded = false;
static rom_info_t info;
static component_boxart_t *boxart = NULL;


static void unload (void) {
    if (boxart) {
        ui_components_boxart_free(boxart);
        boxart = NULL;
    }
    if (info_valid) {
        rom_info_free_meta(&info);
        info_valid = false;
    }
    meta_loaded = false;
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

    // only the header here, not rom_config_load(): that also looks for several optional
    // files next to the ROM, and each missing one means scanning the whole ROM folder
    memset(&info, 0, sizeof(info));
    if (rom_info_load_header(rom_path, &info) != ROM_OK) {
        rom_info_free_meta(&info);
        state = PANEL_STATE_FAILED;
        return;
    }

    info_valid = true;
    state = PANEL_STATE_LOAD_META;
}

void ui_components_game_panel_poll (menu_t *menu) {
    switch (state) {
        case PANEL_STATE_LOAD_META:
            meta_loaded = rom_info_load_database_meta(menu->storage_prefix, &info);
            state = PANEL_STATE_LOAD_BOXART;
            break;
        case PANEL_STATE_LOAD_BOXART:
            // the image itself decodes in the background, see png_decoder_poll()
            boxart = ui_components_boxart_init(menu->storage_prefix, info.game_code, info.title, IMAGE_BOXART_FRONT);
            state = PANEL_STATE_LOADED;
            break;
        default:
            break;
    }
}

void ui_components_game_panel_draw (const char *name, const char *subtitle, const char *details) {
    char rom_details[512];
    rom_details[0] = '\0';

    if (info_valid) {
        if (meta_loaded && info.meta.name != NULL && info.meta.name[0] != '\0') {
            name = info.meta.name;
        }
        if (meta_loaded && info.meta.release_date != NULL && info.meta.release_date[0] != '\0' && strcmp(info.meta.release_date, "Not specified") != 0) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Released: %s\n", info.meta.release_date);
        }
        if (meta_loaded && info.meta.num_players > 0) {
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

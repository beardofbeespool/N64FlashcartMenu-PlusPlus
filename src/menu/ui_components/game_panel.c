/**
 * @file game_panel.c
 * @brief Game info panel shown beside narrow lists (file list, favorites, history).
 * @ingroup ui_components
 */

#include <string.h>

#include "../ui_components.h"
#include "../fonts.h"
#include "constants.h"

/** @brief Frames the selection must rest on an item before it is read from the SD card. */
#define LOAD_DELAY (3)

/** @brief Recently shown games kept in memory, so going back to one is instant. */
#define CACHE_SIZE_EXPANDED (16)
/** @brief Cache size without an Expansion Pak, each cover takes about 35 KB. */
#define CACHE_SIZE_BASIC (3)

/** @brief Neighbours of the selected game preloaded in the background (next, previous, next but one...). */
#define PREFETCH_EXPANDED (4)
#define PREFETCH_BASIC (1)

/**
 * Loading is split into steps run from the poll, so no single frame stalls
 * long enough to make the next button press feel late.
 */
typedef enum {
    PANEL_STATE_LOAD_META = 1,  /**< Header loaded, metadata next */
    PANEL_STATE_LOAD_BOXART,    /**< Metadata loaded, box art next */
    PANEL_STATE_LOADED,
} panel_state_t;

typedef struct {
    bool used;
    char path[256];             /**< Full ROM path, the cache key */
    uint32_t last_used;
    panel_state_t stage;        /**< Next loading step still to do for this game */
    rom_info_t info;
    bool meta_loaded;
    surface_t *image;           /**< Decoded box art, NULL if there is none */
} cache_entry_t;

static cache_entry_t cache[CACHE_SIZE_EXPANDED];
static uint32_t use_counter = 0;

static int selected_id = -1;
static bool lookup_due = false;
static int load_delay = 0;
static bool failed = false;
static int current = -1;
static int prefetch_slot = -1;
static int prefetch_count = 0;
static int loading_slot = -1;
static component_boxart_t *loading_boxart = NULL;


static int cache_capacity (void) {
    return is_memory_expanded() ? CACHE_SIZE_EXPANDED : CACHE_SIZE_BASIC;
}

static void cache_entry_clear (cache_entry_t *entry) {
    if (!entry->used) {
        return;
    }
    rom_info_free_meta(&entry->info);
    if (entry->image) {
        surface_free(entry->image);
        free(entry->image);
    }
    memset(entry, 0, sizeof(*entry));
}

static int cache_find (const char *path) {
    for (int i = 0; i < cache_capacity(); i++) {
        if (cache[i].used && (strcmp(cache[i].path, path) == 0)) {
            return i;
        }
    }
    return -1;
}

static int cache_allocate (void) {
    int slot = -1;

    for (int i = 0; i < cache_capacity(); i++) {
        if (!cache[i].used) {
            return i;
        }
        // least recently used, but never the game being shown
        if ((i != current) && ((slot == -1) || (cache[i].last_used < cache[slot].last_used))) {
            slot = i;
        }
    }

    if (slot == prefetch_slot) {
        prefetch_slot = -1;
    }
    cache_entry_clear(&cache[slot]);
    return slot;
}

static void stop_loading (void) {
    // the entry stays at PANEL_STATE_LOAD_BOXART, so its box art is loaded again later
    if (loading_boxart) {
        ui_components_boxart_free(loading_boxart);
        loading_boxart = NULL;
    }
    loading_slot = -1;
}

static int cache_add (path_t *rom_path) {
    // the decoder may still be working for the entry that gets replaced
    stop_loading();

    int slot = cache_allocate();
    cache_entry_t *entry = &cache[slot];

    // only the header here, not rom_config_load(): that also looks for several optional
    // files next to the ROM, and each missing one means scanning the whole ROM folder
    if (rom_info_load_header(rom_path, &entry->info) != ROM_OK) {
        rom_info_free_meta(&entry->info);
        memset(entry, 0, sizeof(*entry));
        return -1;
    }

    entry->used = true;
    snprintf(entry->path, sizeof(entry->path), "%s", path_get(rom_path));
    entry->last_used = ++use_counter;
    entry->stage = PANEL_STATE_LOAD_META;
    return slot;
}

static void cache_entry_load (menu_t *menu, int slot) {
    cache_entry_t *entry = &cache[slot];

    if (entry->stage == PANEL_STATE_LOAD_META) {
        entry->meta_loaded = rom_info_load_database_meta(menu->storage_prefix, &entry->info);
        entry->stage = PANEL_STATE_LOAD_BOXART;
    }

    if (entry->stage != PANEL_STATE_LOAD_BOXART) {
        return;
    }

    if (loading_boxart == NULL) {
        // a cover seen before loads straight from its cache file, a new one
        // decodes in the background, see png_decoder_poll()
        loading_boxart = ui_components_boxart_init(menu->storage_prefix, entry->info.game_code, entry->info.title, IMAGE_BOXART_FRONT);
        if (loading_boxart == NULL) {
            // no box art for this game
            entry->stage = PANEL_STATE_LOADED;
            return;
        }
        loading_slot = slot;
    }

    if (!loading_boxart->loading) {
        // keep the decoded image in the cache, the component no longer owns it
        entry->image = loading_boxart->image;
        loading_boxart->image = NULL;
        stop_loading();
        entry->stage = PANEL_STATE_LOADED;
    }
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

    // a cover still decoding carries on, it is only stopped once the decoder is needed elsewhere
    current = -1;
    failed = false;
    selected_id = id;
    lookup_due = true;
    load_delay = LOAD_DELAY;
    prefetch_slot = -1;
    prefetch_count = 0;
}

bool ui_components_game_panel_load_due (void) {
    if (selected_id == -1) {
        return false;
    }

    // straight away, a game already in the cache is shown without waiting
    if (lookup_due) {
        lookup_due = false;
        return true;
    }

    if ((load_delay == 0) || (--load_delay > 0)) {
        return false;
    }

    return (current == -1);
}

void ui_components_game_panel_load (menu_t *menu, path_t *rom_path) {
    int slot = cache_find(path_get(rom_path));
    if (slot != -1) {
        current = slot;
        cache[slot].last_used = ++use_counter;
        // carries on where it stopped if the selection moved away mid-load
        return;
    }

    if (load_delay > 0) {
        // not in the cache, read it once the selection has rested
        return;
    }

    current = cache_add(rom_path);
    failed = (current == -1);
}

void ui_components_game_panel_poll (menu_t *menu) {
    // a cover that finished decoding after the selection moved on is still kept
    if ((loading_boxart != NULL) && !loading_boxart->loading) {
        cache_entry_load(menu, loading_slot);
    }

    int slot = -1;

    if ((current != -1) && (cache[current].stage != PANEL_STATE_LOADED)) {
        slot = current;
    } else if ((prefetch_slot != -1) && (cache[prefetch_slot].stage != PANEL_STATE_LOADED)) {
        slot = prefetch_slot;
    }

    if (slot == -1) {
        return;
    }

    // the selected game goes first, before a neighbour still being preloaded
    if ((loading_slot != -1) && (loading_slot != slot)) {
        stop_loading();
    }

    cache_entry_load(menu, slot);
}

int ui_components_game_panel_prefetch_offset (void) {
    int prefetch_max = is_memory_expanded() ? PREFETCH_EXPANDED : PREFETCH_BASIC;

    if ((selected_id == -1) || (load_delay > 0) || (prefetch_count >= prefetch_max)) {
        return 0;
    }
    // only once the selected game is fully shown, and one neighbour at a time
    if ((current != -1) && (cache[current].stage != PANEL_STATE_LOADED)) {
        return 0;
    }
    if ((prefetch_slot != -1) && (cache[prefetch_slot].stage != PANEL_STATE_LOADED)) {
        return 0;
    }

    int i = prefetch_count++;
    int distance = (i / 2) + 1;
    return (i % 2 == 0) ? distance : -distance;
}

void ui_components_game_panel_prefetch (menu_t *menu, path_t *rom_path) {
    int slot = cache_find(path_get(rom_path));
    if (slot == -1) {
        slot = cache_add(rom_path);
        if (slot == -1) {
            return;
        }
    }

    // recently used, so it is still cached when the selection gets there
    cache[slot].last_used = ++use_counter;
    prefetch_slot = slot;
}

void ui_components_game_panel_draw (const char *name, const char *subtitle, const char *details) {
    char rom_details[512];
    rom_details[0] = '\0';

    cache_entry_t *entry = (current != -1) ? &cache[current] : NULL;

    if (entry) {
        rom_info_t *info = &entry->info;
        bool meta_loaded = entry->meta_loaded;

        if (meta_loaded && info->meta.name != NULL && info->meta.name[0] != '\0') {
            name = info->meta.name;
        }
        if (meta_loaded && info->meta.release_date != NULL && info->meta.release_date[0] != '\0' && strcmp(info->meta.release_date, "Not specified") != 0) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Released: %s\n", info->meta.release_date);
        }
        if (meta_loaded && info->meta.num_players > 0) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Players: %lu\n", info->meta.num_players);
        }
        const char *expansion_pak = format_expansion_pak(info->features.expansion_pak);
        if (expansion_pak != NULL) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Expansion Pak: %s\n", expansion_pak);
        }
        if (info->features.rumble_pak) {
            snprintf(rom_details + strlen(rom_details), sizeof(rom_details) - strlen(rom_details), "Rumble Pak\n");
        }
        details = rom_details;
    } else if (failed) {
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

    if (entry && entry->image) {
        component_boxart_t cached = { .loading = false, .image = entry->image };
        ui_components_boxart_draw(&cached);
    } else if ((loading_boxart != NULL) && (loading_slot == current)) {
        // placeholder box while the image decodes
        ui_components_boxart_draw(loading_boxart);
    }
}

void ui_components_game_panel_free (void) {
    // the cache is kept, so coming back to a list shows recent games straight away
    stop_loading();
    current = -1;
    failed = false;
    selected_id = -1;
    lookup_due = false;
    load_delay = 0;
    prefetch_slot = -1;
    prefetch_count = 0;
}

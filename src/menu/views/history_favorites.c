#include <stdarg.h>
#include "../bookkeeping.h"
#include "../fonts.h"
#include "../ui_components/constants.h"
#include "../sound.h"
#include "views.h"


#define VISIBLE_ITEMS (LIST_ENTRIES) /**< Number of single-line items that fit on screen */
#define FAST_SCROLL_ITEMS (10) /**< Items skipped by a fast scroll, same as the file list */

typedef enum {
    BOOKKEEPING_TAB_CONTEXT_HISTORY,
    BOOKKEEPING_TAB_CONTEXT_FAVORITE,
    BOOKKEEPING_TAB_CONTEXT_NONE
} bookkeeping_tab_context_t;


static bookkeeping_tab_context_t tab_context = BOOKKEEPING_TAB_CONTEXT_NONE;
static int selected_item = -1;
static bookkeeping_item_t *item_list;
static uint16_t item_max = 0;
static bool moving_item = false;
static int panel_item = -1;


static int item_first() {
    for(uint16_t i=0; i<item_max; i++) {
        if(item_list[i].bookkeeping_type != BOOKKEEPING_TYPE_EMPTY) {
            return i;
        }
    }
    return -1;
}

static void item_reset_selected(menu_t *menu) {
    selected_item = item_first();
}

static bool item_move_next() {
    int last = selected_item;

    do
    {
        selected_item++;

        if(selected_item >= item_max) {
            selected_item = last;
            return false;
        } else if(item_list[selected_item].bookkeeping_type != BOOKKEEPING_TYPE_EMPTY) {
            return true;
        }
    } while (true);  
}

static bool item_move_previous() {
    int last = selected_item;
    do
    {
        selected_item--;

        if(selected_item < 0) {
            selected_item = last;
            return false;
        } else if(item_list[selected_item].bookkeeping_type != BOOKKEEPING_TYPE_EMPTY) {
            return true;
        }
    } while (true);
}

static int item_count() {
    // lists are kept compacted, so the last populated slot marks the end
    for(int i = item_max - 1; i >= 0; i--) {
        if(item_list[i].bookkeeping_type != BOOKKEEPING_TYPE_EMPTY) {
            return i + 1;
        }
    }
    return 0;
}

static void item_move(menu_t *menu, bool forward, int steps) {
    bool moved = false;

    if(selected_item == -1) {
        return;
    }

    // Wrap around only once the end has been reached, so a fast scroll stops at the end first
    if(menu->settings.wrap_file_list_scrolling) {
        int first = item_first();
        int last = item_count() - 1;
        if(forward && selected_item == last) {
            selected_item = first;
            sound_play_effect(SFX_CURSOR);
            return;
        } else if(!forward && selected_item == first) {
            selected_item = last;
            sound_play_effect(SFX_CURSOR);
            return;
        }
    }

    for(int i = 0; i < steps; i++) {
        if(!(forward ? item_move_next() : item_move_previous())) {
            break;
        }
        moved = true;
    }

    if(moved) {
        sound_play_effect(SFX_CURSOR);
    }
}

static void item_reorder(menu_t *menu, bool forward) {
    int target = selected_item + (forward ? 1 : -1);

    if(target < 0 || target >= item_count() || item_list[target].bookkeeping_type == BOOKKEEPING_TYPE_EMPTY) {
        return;
    }

    bookkeeping_favorite_swap(&menu->bookkeeping, selected_item, target);
    selected_item = target;
    sound_play_effect(SFX_CURSOR);
}

static void item_reorder_finish(menu_t *menu) {
    if(moving_item) {
        moving_item = false;
        // saved once at the end rather than on every step, to avoid an SD card write per button press
        bookkeeping_save(&menu->bookkeeping);
    }
}

static void process(menu_t *menu) {
    int scroll_speed = menu->actions.go_fast ? FAST_SCROLL_ITEMS : 1;

    if(moving_item) {
        if(menu->actions.go_down) {
            item_reorder(menu, true);
            return;
        } else if(menu->actions.go_up) {
            item_reorder(menu, false);
            return;
        } else if(menu->actions.lz_context || menu->actions.enter || menu->actions.back || menu->actions.settings || menu->actions.options) {
            item_reorder_finish(menu);
            sound_play_effect(SFX_SETTING);
            return;
        } else if(menu->actions.go_left || menu->actions.go_right) {
            // drop the item and carry on to switch tabs
            item_reorder_finish(menu);
        } else {
            return;
        }
    }

    if(menu->actions.go_down) {
        item_move(menu, true, scroll_speed);
    } else if(menu->actions.go_up) {
        item_move(menu, false, scroll_speed);
    } else if((menu->actions.enter || menu->actions.settings) && selected_item != -1) {
                
        if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) {
            menu->load.load_favorite_id = selected_item;
            menu->load.load_history_id = -1;
        } else if(tab_context == BOOKKEEPING_TAB_CONTEXT_HISTORY) {
            menu->load.load_history_id = selected_item;
            menu->load.load_favorite_id = -1;
        }           

        if(item_list[selected_item].bookkeeping_type == BOOKKEEPING_TYPE_DISK) {
            menu->next_mode = MENU_MODE_LOAD_DISK;
            sound_play_effect(SFX_ENTER);
        } else if(item_list[selected_item].bookkeeping_type == BOOKKEEPING_TYPE_ROM) {
            // Start skips the ROM info screen
            menu->load_pending.rom_file = menu->actions.settings;
            menu->next_mode = MENU_MODE_LOAD_ROM;
            sound_play_effect(SFX_ENTER);
        }
    } else if (menu->actions.go_left) {
        if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) {
            menu->next_mode = MENU_MODE_HISTORY;
        } else if(tab_context == BOOKKEEPING_TAB_CONTEXT_HISTORY) {
            menu->next_mode = MENU_MODE_BROWSER;
        }
        sound_play_effect(SFX_CURSOR);       
    } else if (menu->actions.go_right) {
        if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) {
            menu->next_mode = MENU_MODE_BROWSER;
        } else if(tab_context == BOOKKEEPING_TAB_CONTEXT_HISTORY) {
            menu->next_mode = MENU_MODE_FAVORITE;
        }
        sound_play_effect(SFX_CURSOR);
    }else if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE && menu->actions.options && selected_item != -1) {
        bookkeeping_favorite_remove(&menu->bookkeeping, selected_item);
        item_reset_selected(menu);
        // the index may be unchanged but now points at another game
        ui_components_game_panel_free();
        sound_play_effect(SFX_SETTING);
    } else if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE && menu->actions.lz_context && selected_item != -1) {
        if(menu->settings.favorites_alphabetical) {
            // the list is kept sorted, so there is no manual order to change
            sound_play_effect(SFX_ERROR);
        } else {
            moving_item = true;
            sound_play_effect(SFX_SETTING);
        }
    }
}

// Copies the file name without its extension, for a tidier list
static void format_item_name(char *buffer, size_t size, path_t *path) {
    snprintf(buffer, size, "%s", path_has_value(path) ? path_last_get(path) : "");
    char *dot = strrchr(buffer, '.');
    if(dot != NULL && dot != buffer) {
        *dot = '\0';
    }
}

static void draw_list(menu_t *menu, surface_t *display) {
    int entries = item_count();
    int starting_position = 0;

    if (entries > VISIBLE_ITEMS && selected_item >= (VISIBLE_ITEMS / 2)) {
        starting_position = selected_item - (VISIBLE_ITEMS / 2);
        if (starting_position >= entries - VISIBLE_ITEMS) {
            starting_position = entries - VISIBLE_ITEMS;
        }
    }

    int list_y = VISIBLE_AREA_Y0 + TEXT_MARGIN_VERTICAL + TAB_HEIGHT + TEXT_OFFSET_VERTICAL;

    ui_components_scrollbar_draw(GAME_PANEL_X - LIST_SCROLLBAR_WIDTH, LIST_SCROLLBAR_Y, LIST_SCROLLBAR_WIDTH, LIST_SCROLLBAR_HEIGHT, selected_item, entries, VISIBLE_ITEMS);

    if(entries == 0) {
        rdpq_text_printf(
            &(rdpq_textparms_t) {
                .width = GAME_PANEL_LIST_WIDTH - (TEXT_MARGIN_HORIZONTAL * 2),
                .wrap = WRAP_WORD,
            },
            FNT_DEFAULT,
            VISIBLE_AREA_X0 + TEXT_MARGIN_HORIZONTAL,
            list_y + TEXT_MARGIN_VERTICAL,
            "^%02X%s",
            STL_GRAY,
            (tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE)
                ? "No favorites yet.\n\nPress L or Z on a game in the file list to add one."
                : "No games launched yet."
        );
        return;
    }

    char buffer[VISIBLE_ITEMS * 64];
    char name[256];
    buffer[0] = 0;

    int ending_position = starting_position + VISIBLE_ITEMS;
    if(ending_position > entries) {
        ending_position = entries;
    }

    for(int i = starting_position; i < ending_position; i++) {
        format_item_name(name, sizeof(name), item_list[i].primary_path);
        // trimmed to what fits on one line, the ellipsis wrap handles the rest
        snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "%.60s%s", name, (i + 1 < ending_position) ? "\n" : "");
    }

    int nbytes = strlen(buffer);
    rdpq_paragraph_t *layout = rdpq_paragraph_build(
        &(rdpq_textparms_t) {
            .width = GAME_PANEL_LIST_WIDTH - (TEXT_MARGIN_HORIZONTAL * 2),
            .height = LAYOUT_ACTIONS_SEPARATOR_Y - list_y,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .wrap = WRAP_ELLIPSES,
            .line_spacing = TEXT_LINE_SPACING_ADJUST,
        },
        FNT_DEFAULT,
        buffer,
        &nbytes
    );

    if(selected_item != -1 && layout->nlines > 0) {
        int highlight_height = (layout->bbox.y1 - layout->bbox.y0) / layout->nlines;
        int highlight_y = list_y + ((selected_item - starting_position) * highlight_height);

        ui_components_highlight_draw(
            VISIBLE_AREA_X0,
            highlight_y,
            VISIBLE_AREA_X0 + GAME_PANEL_LIST_WIDTH,
            highlight_y + highlight_height,
            moving_item ? FILE_LIST_MOVE_HIGHLIGHT_COLOR : FILE_LIST_HIGHLIGHT_COLOR
        );
    }

    rdpq_paragraph_render(layout, VISIBLE_AREA_X0 + TEXT_MARGIN_HORIZONTAL, list_y);
    rdpq_paragraph_free(layout);
}

static void draw_panel(menu_t *menu, surface_t *display) {
    if(selected_item == -1) {
        return;
    }

    bookkeeping_item_t *item = &item_list[selected_item];
    char name[256];
    char subtitle[64];
    char details[300];
    details[0] = 0;

    format_item_name(name, sizeof(name), item->primary_path);

    snprintf(subtitle, sizeof(subtitle), "%s %d of %d", (tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) ? "Favorite" : "History", selected_item + 1, item_count());

    if(item->bookkeeping_type == BOOKKEEPING_TYPE_DISK) {
        char rom_name[256];
        format_item_name(rom_name, sizeof(rom_name), item->secondary_path);
        snprintf(details, sizeof(details), "64DD disk%s%s", rom_name[0] ? "\nwith " : "", rom_name);
    }

    ui_components_game_panel_draw(name, subtitle, details);
}

static void panel_update(menu_t *menu) {
    // while a favorite is being moved the selected game stays the same, so keep its info
    if(!moving_item) {
        panel_item = selected_item;
    }

    ui_components_game_panel_select(panel_item);
    ui_components_game_panel_poll(menu);

    if(ui_components_game_panel_load_due() && selected_item != -1 && item_list[selected_item].bookkeeping_type == BOOKKEEPING_TYPE_ROM) {
        ui_components_game_panel_load(menu, item_list[selected_item].primary_path);
    }

    // preload the games around the selection, so moving to them is instant
    int offset = ui_components_game_panel_prefetch_offset();
    int count = item_count();
    if(offset != 0 && selected_item != -1 && count > 0) {
        bookkeeping_item_t *neighbour = &item_list[((selected_item + offset) % count + count) % count];
        if(neighbour->bookkeeping_type == BOOKKEEPING_TYPE_ROM) {
            ui_components_game_panel_prefetch(menu, neighbour->primary_path);
        }
    }
}

static void draw(menu_t *menu, surface_t *display) {
    rdpq_attach(display, NULL);

    ui_components_background_draw();

    if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) {
        ui_components_tabs_common_draw(2);
    } else if(tab_context == BOOKKEEPING_TAB_CONTEXT_HISTORY) {
        ui_components_tabs_common_draw(1);
    }

    ui_components_layout_draw_tabbed();

    draw_list(menu, display);

    draw_panel(menu, display);

    if(moving_item) {
        ui_components_actions_bar_text_draw(
            STL_DEFAULT,
            ALIGN_LEFT, VALIGN_TOP,
            "▲▼: Move game\n"
            "L|Z: Done\n"
        );
    } else if(selected_item != -1) {
        ui_components_actions_bar_text_draw(
            STL_DEFAULT,
            ALIGN_LEFT, VALIGN_TOP,
            "A: Game Info\n"
            "Start: Launch\n"
        );
        
        if(tab_context == BOOKKEEPING_TAB_CONTEXT_FAVORITE) {
            ui_components_actions_bar_text_draw(
                STL_DEFAULT,
                ALIGN_RIGHT, VALIGN_TOP,
                "R: Remove item\n"
                "^%02X%s^00\n",
                menu->settings.favorites_alphabetical ? STL_GRAY : STL_DEFAULT,
                menu->settings.favorites_alphabetical ? "Sorted A-Z" : "L|Z: Reorder"
            );
        }
    }

    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_CENTER, VALIGN_TOP,
        "◀ Change Tab ▶\n"
        "\n"
    );    

    rdpq_detach_show();   
}

void view_favorite_init (menu_t *menu) {
    tab_context = BOOKKEEPING_TAB_CONTEXT_FAVORITE;
    moving_item = false;
    item_list = menu->bookkeeping.favorite_items;
    item_max = FAVORITES_COUNT;

    ui_components_game_panel_free();
    panel_item = -1;
    item_reset_selected(menu);
}

void view_favorite_display (menu_t *menu, surface_t *display) {
    process(menu);
    panel_update(menu);
    draw(menu, display);

    if(menu->next_mode != MENU_MODE_FAVORITE) {
        ui_components_game_panel_free();
    }
}

void view_history_init (menu_t *menu) {
    tab_context = BOOKKEEPING_TAB_CONTEXT_HISTORY;
    moving_item = false;
    item_list = menu->bookkeeping.history_items;
    item_max = HISTORY_COUNT;

    ui_components_game_panel_free();
    panel_item = -1;
    item_reset_selected(menu);
}

void view_history_display (menu_t *menu, surface_t *display) {
    process(menu);
    panel_update(menu);
    draw(menu, display);

    if(menu->next_mode != MENU_MODE_HISTORY) {
        ui_components_game_panel_free();
    }
}

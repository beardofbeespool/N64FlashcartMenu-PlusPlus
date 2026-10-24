#include <libdragon.h>

#include "fonts.h"
#include "ui_components/constants.h"
#include "utils/fs.h"
#include "utils/utils.h"


/** @brief Bigger, bolder strokes that survive composite video and interlacing on CRTs. */
#define LARGE_FONT_PATH "rom:/Firple-Bold-Large.font64"

ui_layout_t ui_layout = {
    .large_text = false,
    .line_height = 18,
    .tab_height = 20,
    .actions_separator_y = 400,
    .list_entries = LIST_ENTRIES_MAX,
};

static rdpq_font_t *default_font = NULL;
static char *custom_path = NULL;


static void load_default_font (char *custom_font_path, bool large_text) {
    char *font_path = large_text ? LARGE_FONT_PATH : "rom:/Firple-Bold.font64";

    // a custom font replaces the normal size only, large text always uses the built-in large font
    if (!large_text && custom_font_path != NULL && strlen(custom_font_path) > 0) {
        // Only check file_exists if custom_font_path is a valid filesystem path (not rom:/)
        if (strncmp(custom_font_path, "rom:/", 5) != 0 && file_exists(custom_font_path)) {
            font_path = custom_font_path;
        }
    }

    default_font = rdpq_font_load(font_path);

    rdpq_font_style(default_font, STL_DEFAULT, &((rdpq_fontstyle_t) { .color = RGBA32(0xFF, 0xFF, 0xFF, 0xFF) }));
    rdpq_font_style(default_font, STL_GREEN, &((rdpq_fontstyle_t) { .color = RGBA32(0x70, 0xFF, 0x70, 0xFF) }));
    rdpq_font_style(default_font, STL_BLUE, &((rdpq_fontstyle_t) { .color = RGBA32(0x70, 0xBC, 0xFF, 0xFF) }));
    rdpq_font_style(default_font, STL_YELLOW, &((rdpq_fontstyle_t) { .color = RGBA32(0xFF, 0xFF, 0x70, 0xFF) }));
    rdpq_font_style(default_font, STL_ORANGE, &((rdpq_fontstyle_t) { .color = RGBA32(0xFF, 0x99, 0x00, 0xFF) }));
    rdpq_font_style(default_font, STL_RED, &((rdpq_fontstyle_t) { .color = RGBA32(0xFF, 0x40, 0x40, 0xFF) }));
    rdpq_font_style(default_font, STL_GRAY, &((rdpq_fontstyle_t) { .color = RGBA32(0xB8, 0xC0, 0xD8, 0xFF) }));

    rdpq_text_register_font(FNT_DEFAULT, default_font);
}

static int measure_line_height (void) {
    int nbytes = 5;
    rdpq_paragraph_t *layout = rdpq_paragraph_build(
        &(rdpq_textparms_t) { .line_spacing = TEXT_LINE_SPACING_ADJUST },
        FNT_DEFAULT,
        "A\nA\nA",
        &nbytes
    );
    // same measure the lists use to size their highlight rows
    int line_height = (layout->bbox.y1 - layout->bbox.y0) / layout->nlines;
    rdpq_paragraph_free(layout);
    return line_height;
}

static void update_layout (bool large_text) {
    ui_layout.large_text = large_text;
    ui_layout.line_height = measure_line_height();

    if (!large_text) {
        // the original fixed layout, every screen was designed around it
        ui_layout.tab_height = 20;
        ui_layout.actions_separator_y = 400;
        ui_layout.list_entries = LIST_ENTRIES_MAX;
        return;
    }

    int line_height = ui_layout.line_height;

    ui_layout.tab_height = MAX(20, line_height + 4);

    // the actions bar holds two lines of text under the separator
    int actions_bar_height = BORDER_THICKNESS + (TEXT_MARGIN_VERTICAL * 2) + TEXT_OFFSET_VERTICAL + (line_height * 2);
    ui_layout.actions_separator_y = MIN(400, VISIBLE_AREA_Y1 - actions_bar_height);

    int list_top = VISIBLE_AREA_Y0 + ui_layout.tab_height + TEXT_MARGIN_VERTICAL + TEXT_OFFSET_VERTICAL;
    int list_entries = (ui_layout.actions_separator_y - list_top) / line_height;
    ui_layout.list_entries = MAX(1, MIN(LIST_ENTRIES_MAX, list_entries));
}


void fonts_init (char *custom_font_path, bool large_text) {
    if (custom_font_path) {
        custom_path = strdup(custom_font_path);
    }
    load_default_font(custom_path, large_text);
    update_layout(large_text);
}

void fonts_set_large_text (bool large_text) {
    if ((default_font != NULL) && (large_text == ui_layout.large_text)) {
        return;
    }

    // text drawn in the previous frame may still be using the old font's textures
    rspq_wait();
    rdpq_text_unregister_font(FNT_DEFAULT);
    if (default_font) {
        rdpq_font_free(default_font);
        default_font = NULL;
    }

    load_default_font(custom_path, large_text);
    update_layout(large_text);
}

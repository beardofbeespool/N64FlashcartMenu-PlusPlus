/**
 * @file theme.c
 * @brief Selectable color themes for the menu UI.
 * @ingroup ui_components
 */

#include <libdragon.h>

#include "../ui_components.h"
#include "constants.h"

/** @brief Gold outline drawn around the selected row in most themes. */
#define GOLD RGBA32(0xFF, 0xD0, 0x40, 0xFF)
/** @brief Amber fill for a list row that is being moved. */
#define AMBER RGBA32(0xC0, 0x7A, 0x10, 0xFF)

/**
 * @brief Available themes, the first one is the default.
 *
 * Each theme uses a very dark tint of its color for the background, a saturated
 * mid tone for the selection and active tab, and a light tint for borders, so
 * white text stays readable everywhere.
 */
static const ui_theme_t themes[] = {
    {
        .name = "Navy",
        .background = RGBA32(0x0E, 0x16, 0x30, 0xFF),
        .border = RGBA32(0x8F, 0xB0, 0xF0, 0xFF),
        .highlight = RGBA32(0x2E, 0x5B, 0xD6, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x1C, 0x28, 0x4C, 0xFF),
        .scrollbar_inactive = RGBA32(0x2C, 0x3A, 0x62, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x12, 0x1C, 0x3C, 0xFF),
        .tab_inactive_border = RGBA32(0x3A, 0x4A, 0x7A, 0xFF),
        .tab_inactive_background = RGBA32(0x18, 0x22, 0x48, 0xFF),
        .progressbar_done = RGBA32(0x4A, 0x90, 0xFF, 0xFF),
    },
    {
        .name = "Red",
        .background = RGBA32(0x2A, 0x0A, 0x0E, 0xFF),
        .border = RGBA32(0xF0, 0xA0, 0xA8, 0xFF),
        .highlight = RGBA32(0xC0, 0x28, 0x3C, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x3E, 0x12, 0x18, 0xFF),
        .scrollbar_inactive = RGBA32(0x5E, 0x1C, 0x26, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x38, 0x0E, 0x14, 0xFF),
        .tab_inactive_border = RGBA32(0x7A, 0x2A, 0x36, 0xFF),
        .tab_inactive_background = RGBA32(0x45, 0x12, 0x19, 0xFF),
        .progressbar_done = RGBA32(0xF0, 0x40, 0x50, 0xFF),
    },
    {
        .name = "Purple",
        .background = RGBA32(0x1C, 0x0E, 0x30, 0xFF),
        .border = RGBA32(0xC8, 0xA8, 0xF8, 0xFF),
        .highlight = RGBA32(0x7A, 0x3C, 0xD0, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x2A, 0x18, 0x48, 0xFF),
        .scrollbar_inactive = RGBA32(0x42, 0x2A, 0x66, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x24, 0x14, 0x3E, 0xFF),
        .tab_inactive_border = RGBA32(0x56, 0x38, 0x7E, 0xFF),
        .tab_inactive_background = RGBA32(0x2E, 0x1A, 0x4A, 0xFF),
        .progressbar_done = RGBA32(0xA0, 0x60, 0xF0, 0xFF),
    },
    {
        .name = "Green",
        .background = RGBA32(0x0A, 0x24, 0x14, 0xFF),
        .border = RGBA32(0x9E, 0xE0, 0xB0, 0xFF),
        .highlight = RGBA32(0x23, 0x8A, 0x3E, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x10, 0x36, 0x1E, 0xFF),
        .scrollbar_inactive = RGBA32(0x1E, 0x54, 0x30, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x0E, 0x30, 0x1A, 0xFF),
        .tab_inactive_border = RGBA32(0x2A, 0x6A, 0x40, 0xFF),
        .tab_inactive_background = RGBA32(0x12, 0x3A, 0x20, 0xFF),
        .progressbar_done = RGBA32(0x40, 0xC0, 0x60, 0xFF),
    },
    {
        .name = "Yellow",
        .background = RGBA32(0x2A, 0x22, 0x08, 0xFF),
        .border = RGBA32(0xF8, 0xE0, 0x8A, 0xFF),
        .highlight = RGBA32(0xA0, 0x7C, 0x10, 0xFF),
        // gold would vanish against a gold selection
        .highlight_outline = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
        .move_highlight = RGBA32(0x2E, 0x5B, 0xD6, 0xFF),
        .scrollbar_bg = RGBA32(0x3C, 0x30, 0x0C, 0xFF),
        .scrollbar_inactive = RGBA32(0x5A, 0x4A, 0x14, 0xFF),
        .scrollbar_position = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
        .dialog_bg = RGBA32(0x36, 0x2C, 0x0A, 0xFF),
        .tab_inactive_border = RGBA32(0x7A, 0x64, 0x20, 0xFF),
        .tab_inactive_background = RGBA32(0x40, 0x34, 0x0C, 0xFF),
        .progressbar_done = RGBA32(0xE0, 0xB0, 0x20, 0xFF),
    },
    {
        .name = "Teal",
        .background = RGBA32(0x08, 0x26, 0x2A, 0xFF),
        .border = RGBA32(0x90, 0xE0, 0xE8, 0xFF),
        .highlight = RGBA32(0x1A, 0x8A, 0x96, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x0E, 0x36, 0x3C, 0xFF),
        .scrollbar_inactive = RGBA32(0x1A, 0x54, 0x5C, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x0C, 0x30, 0x36, 0xFF),
        .tab_inactive_border = RGBA32(0x24, 0x6A, 0x74, 0xFF),
        .tab_inactive_background = RGBA32(0x0E, 0x3A, 0x40, 0xFF),
        .progressbar_done = RGBA32(0x30, 0xC0, 0xD0, 0xFF),
    },
    {
        .name = "Pink",
        .background = RGBA32(0x2A, 0x0C, 0x22, 0xFF),
        .border = RGBA32(0xF8, 0xA8, 0xDC, 0xFF),
        .highlight = RGBA32(0xC0, 0x3C, 0x96, 0xFF),
        .highlight_outline = GOLD,
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x3E, 0x14, 0x32, 0xFF),
        .scrollbar_inactive = RGBA32(0x5E, 0x22, 0x50, 0xFF),
        .scrollbar_position = GOLD,
        .dialog_bg = RGBA32(0x38, 0x10, 0x2C, 0xFF),
        .tab_inactive_border = RGBA32(0x7A, 0x2E, 0x68, 0xFF),
        .tab_inactive_background = RGBA32(0x44, 0x16, 0x36, 0xFF),
        .progressbar_done = RGBA32(0xF0, 0x60, 0xC0, 0xFF),
    },
    {
        .name = "Classic Gray",
        .background = RGBA32(0x00, 0x00, 0x00, 0xFF),
        .border = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
        .highlight = RGBA32(0x7F, 0x7F, 0x7F, 0xFF),
        .highlight_outline = RGBA32(0xFF, 0xFF, 0xFF, 0xFF),
        .move_highlight = AMBER,
        .scrollbar_bg = RGBA32(0x3F, 0x3F, 0x3F, 0xFF),
        .scrollbar_inactive = RGBA32(0x5F, 0x5F, 0x5F, 0xFF),
        .scrollbar_position = RGBA32(0x7F, 0x7F, 0x7F, 0xFF),
        .dialog_bg = RGBA32(0x00, 0x00, 0x00, 0xFF),
        .tab_inactive_border = RGBA32(0x5F, 0x5F, 0x5F, 0xFF),
        .tab_inactive_background = RGBA32(0x3F, 0x3F, 0x3F, 0xFF),
        .progressbar_done = RGBA32(0x3B, 0x7C, 0xF5, 0xFF),
    },
};

#define THEME_COUNT ((int) (sizeof(themes) / sizeof(themes[0])))

const ui_theme_t *ui_theme = &themes[0];

int ui_theme_count (void) {
    return THEME_COUNT;
}

const char *ui_theme_name (int index) {
    if ((index < 0) || (index >= THEME_COUNT)) {
        return themes[0].name;
    }
    return themes[index].name;
}

void ui_theme_set (int index) {
    if ((index < 0) || (index >= THEME_COUNT)) {
        index = 0;
    }
    ui_theme = &themes[index];
}

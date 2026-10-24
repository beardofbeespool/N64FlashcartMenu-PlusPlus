/**
 * @file ui_components.h
 * @brief Menu Graphical User Interface Components
 * @ingroup menu
 */

#ifndef UI_COMPONENTS_H__
#define UI_COMPONENTS_H__

#include <libdragon.h>
#include "menu_state.h"
#include "fonts.h"


/** 
 * @brief File image Enumeration.
 * 
 * Enumeration for different types of file images used in the user interface.
 */
typedef enum {
    IMAGE_BOXART_FRONT,    /**< Boxart image from the front */
    IMAGE_BOXART_BACK,     /**< Boxart image from the back */
    IMAGE_BOXART_TOP,      /**< Boxart image from the top */
    IMAGE_BOXART_BOTTOM,   /**< Boxart image from the bottom */
    IMAGE_BOXART_LEFT,     /**< Boxart image from the left side */
    IMAGE_BOXART_RIGHT,    /**< Boxart image from the right side */
    IMAGE_GAMEPAK_FRONT,   /**< GamePak image from the front */
    IMAGE_GAMEPAK_BACK,    /**< GamePak image from the back */
    IMAGE_THUMBNAIL,       /**< File image thumbnail */
    IMAGE_TYPE_END         /**< List end marker */
} file_image_type_t;


typedef struct fat_file_attributes {
    bool is_read_only;    /**< Read-only attribute */
    bool is_hidden;       /**< Hidden attribute */
    bool is_system;       /**< System attribute */
    bool is_archive;      /**< Archive attribute */
} fat_file_attributes_t;


typedef struct zip_file_attributes {
    
    bool writeable;                             /**< File is writeable */
    bool encrypted;                             /**< File is encrypted */
    uint64_t compressed_size;                   /**< File size in bytes while compressed */
    uint32_t crc32;                             /**< Checksum for compressed files */
} zip_file_attributes_t;


typedef struct pak_file_attributes {
    bool is_controller_pak_dump;                /**< file is a controller pak dump */
    bool is_controller_pak_dump_note;           /**< file is a controller pak dump note */
} pak_file_attributes_t;


/** 
 * @brief File information Structure.
 * 
 * Structure with file information displayed used in the user interface.
 */
typedef struct {
    bool directory;                             /**< Directory rather than a file */
    time_t mtime;                               /**< Last modification time */
    uint64_t size;                              /**< File size in bytes */
    fat_file_attributes_t fat_file_attributes;  /**< FAT file attributes */
    zip_file_attributes_t zip_file_attributes;  /**< ZIP file attributes */
    pak_file_attributes_t pak_file_attributes;  /**< Additional attributes for pak files */
} file_info_t;

/**
 * @brief Draw a box component.
 * 
 * @param x0 Starting x-coordinate.
 * @param y0 Starting y-coordinate.
 * @param x1 Ending x-coordinate.
 * @param y1 Ending y-coordinate.
 * @param color Color of the box.
 */
void ui_components_box_draw(int x0, int y0, int x1, int y1, color_t color);

/**
 * @brief Draw the highlight for a selected list row.
 * 
 * Fills the row and draws an outline inside its bounds, so neighbouring rows are not covered.
 * 
 * @param x0 Starting x-coordinate.
 * @param y0 Starting y-coordinate.
 * @param x1 Ending x-coordinate.
 * @param y1 Ending y-coordinate.
 * @param color Fill color of the highlight.
 */
void ui_components_highlight_draw(int x0, int y0, int x1, int y1, color_t color);

/**
 * @brief Number of available color themes.
 */
int ui_theme_count(void);

/**
 * @brief Name of a color theme.
 * 
 * @param index Theme index.
 * @return The theme name, or the default theme's name for an invalid index.
 */
const char *ui_theme_name(int index);

/**
 * @brief Make a color theme active.
 * 
 * @param index Theme index, an invalid index selects the default theme.
 */
void ui_theme_set(int index);

/**
 * @brief Shrink the whole picture inward, so text stays clear of the screen edges and corners.
 * 
 * @param percent Margin on each side, as a percentage of the picture size (0 = full size).
 */
void ui_components_screen_margin_apply(int percent);

/**
 * @brief Select the item the game panel describes.
 * 
 * Call every frame. When the item changes the panel is cleared, and its info is
 * loaded only once the selection has rested on it, so scrolling stays fast.
 * 
 * @param id Identifier of the selected item, -1 for none.
 */
void ui_components_game_panel_select(int id);

/**
 * @brief Check whether the selected item's info should be loaded now.
 * 
 * @return true once, when the selection has rested long enough.
 */
bool ui_components_game_panel_load_due(void);

/**
 * @brief Load ROM info and box art for the selected item.
 * 
 * @param menu Pointer to the menu structure.
 * @param rom_path Path to the N64 ROM.
 */
void ui_components_game_panel_load(menu_t *menu, path_t *rom_path);

/**
 * @brief Draw the game panel to the right of a narrow list.
 * 
 * @param name Name of the selected item, replaced by the game's release name when known.
 * @param subtitle Gray line under the name, such as the position in the list.
 * @param details Text shown when no ROM info is loaded, such as the item type.
 */
void ui_components_game_panel_draw(const char *name, const char *subtitle, const char *details);

/**
 * @brief Free the game panel's ROM info and box art, and reset its selection.
 */
void ui_components_game_panel_free(void);

/**
 * @brief Draw a border component.
 * 
 * @param x0 Starting x-coordinate.
 * @param y0 Starting y-coordinate.
 * @param x1 Ending x-coordinate.
 * @param y1 Ending y-coordinate.
 */
void ui_components_border_draw(int x0, int y0, int x1, int y1);

/**
 * @brief Draw the layout component with tabs.
 */
void ui_components_layout_draw_tabbed(void);

/**
 * @brief Draw the layout component.
 */
void ui_components_layout_draw(void);

/**
 * @brief Draw a progress bar component.
 * 
 * @param x0 Starting x-coordinate.
 * @param y0 Starting y-coordinate.
 * @param x1 Ending x-coordinate.
 * @param y1 Ending y-coordinate.
 * @param progress Progress value (0.0 to 1.0).
 */
void ui_components_progressbar_draw(int x0, int y0, int x1, int y1, float progress);

/**
 * @brief Draw a seek bar component.
 * 
 * @param progress Progress value (0.0 to 1.0).
 */
void ui_components_seekbar_draw(float progress);

/**
 * @brief Draw a loader component.
 * 
 * @param position Position value (0.0 to 1.0).
 * @param msg Message to display, truncated to 30 characters.
 */
void ui_components_loader_draw(float progress, const char *msg);

/**
 * @brief Draw a scrollbar component.
 * 
 * @param x Starting x-coordinate.
 * @param y Starting y-coordinate.
 * @param width Width of the scrollbar.
 * @param height Height of the scrollbar.
 * @param position Current position.
 * @param items Total number of items.
 * @param visible_items Number of visible items.
 */
void ui_components_scrollbar_draw(int x, int y, int width, int height, int position, int items, int visible_items);

/**
 * @brief Draw a list scrollbar component.
 * 
 * @param position Current position.
 * @param items Total number of items.
 * @param visible_items Number of visible items.
 */
void ui_components_list_scrollbar_draw(int position, int items, int visible_items);

/**
 * @brief Draw a dialog component.
 * 
 * @param width Width of the dialog.
 * @param height Height of the dialog.
 */
void ui_components_dialog_draw(int width, int height);

/**
 * @brief Draw a message box component.
 * 
 * @param fmt Format string for the message.
 * @param ... Additional arguments for the format string.
 */
void ui_components_messagebox_draw(char *fmt, ...);

/**
 * @brief Draw the main text component.
 * 
 * @param style The font style.
 * @param align Horizontal alignment.
 * @param valign Vertical alignment.
 * @param fmt Format string for the text.
 * @param ... Additional arguments for the format string.
 */
void ui_components_main_text_draw(menu_font_type_t style, rdpq_align_t align, rdpq_valign_t valign, char *fmt, ...);

/**
 * @brief Draw the actions bar text component.
 * 
 * @param style The font style.
 * @param align Horizontal alignment.
 * @param valign Vertical alignment.
 * @param fmt Format string for the text.
 * @param ... Additional arguments for the format string.
 */
void ui_components_actions_bar_text_draw(menu_font_type_t style, rdpq_align_t align, rdpq_valign_t valign, char *fmt, ...);

/**
 * @brief Initialize the background component.
 * 
 * @param cache_location Location of the cache.
 */
void ui_components_background_init(char *cache_location);

/**
 * @brief Free the background component resources.
 */
void ui_components_background_free(void);

/**
 * @brief Free and delete the cached background image.
 */
void ui_components_background_clear(void);

/**
 * @brief Replace the background image.
 * 
 * @param image New background image.
 */
void ui_components_background_replace_image(surface_t *image);

/**
 * @brief Draw the background component.
 */
void ui_components_background_draw(void);

/**
 * @brief Return the current background image surface, or NULL if none is set.
 */
surface_t *ui_components_background_get_image(void);

/**
 * @brief Reload the background image from cache (call after temporarily freeing).
 */
void ui_components_background_reload(void);

/**
 * @brief Free only the in-memory image and display list, keeping the cache intact.
 */
void ui_components_background_image_free_only(void);

/**
 * @brief Draw the file list component.
 * 
 * @param list List of entries.
 * @param entries Number of entries.
 * @param selected Index of the selected entry.
 */
void ui_components_file_list_draw(entry_t *list, int entries, int selected);

/**
 * @brief Draw the file list in the left column, leaving room for the game panel.
 * 
 * File sizes are left out, the game panel shows details for the selected entry.
 * 
 * @param list Pointer to the list of file entries.
 * @param entries Number of entries in the list.
 * @param selected Index of the currently selected entry.
 */
void ui_components_file_list_draw_split(entry_t *list, int entries, int selected);

/**
 * @brief Free file list component cached resources.
 */
void ui_components_file_list_free(void);

/**
 * @brief Context menu structure.
 */
typedef struct component_context_menu {
    int row_count; /**< Number of rows in the context menu */
    int row_selected; /**< Index of the selected row */
    bool hide_pending; /**< Flag to indicate if hiding is pending */
    struct component_context_menu *parent; /**< Pointer to the parent context menu */
    struct component_context_menu *submenu; /**< Pointer to the submenu */
    int (*get_default_selection)(menu_t *menu); /**< Optional function to get the default selected row */
    struct {
        const char *text; /**< Text of the menu item */
        void (*action)(menu_t *menu, void *arg); /**< Action function for the menu item */
        void *arg; /**< Argument for the action function */
        struct component_context_menu *submenu; /**< Pointer to the submenu */
    } list[]; /**< List of menu items */
} component_context_menu_t;

#define COMPONENT_CONTEXT_MENU_LIST_END { .text = NULL } /**< End marker for the context menu list */

/**
 * @brief Initialize the context menu component.
 * 
 * @param cm Pointer to the context menu structure.
 */
void ui_components_context_menu_init(component_context_menu_t *cm);

/**
 * @brief Show the context menu component.
 * 
 * @param cm Pointer to the context menu structure.
 */
void ui_components_context_menu_show(component_context_menu_t *cm);

/**
 * @brief Process the context menu component.
 * 
 * @param menu Pointer to the menu structure.
 * @param cm Pointer to the context menu structure.
 * @return True if the context menu was processed, false otherwise.
 */
bool ui_components_context_menu_process(menu_t *menu, component_context_menu_t *cm);

/**
 * @brief Draw the context menu component.
 * 
 * @param cm Pointer to the context menu structure.
 */
void ui_components_context_menu_draw(component_context_menu_t *cm);

/**
 * @brief Box Art Structure.
 */
typedef struct {
    bool loading; /**< Flag to indicate if the box art is loading */
    surface_t *image; /**< Pointer to the box art image */
} component_boxart_t;

/**
 * @brief Initialize the box art component.
 * 
 * @param storage_prefix Prefix for the storage location.
 * @param game_code Game code for the box art.
 * @param rom_title Title of the ROM (may be NULL). If used, it is sanitized for filesystem safety.
 * @param current_image_view Current image view type.
 * @return Pointer to the initialized box art component.
 */
component_boxart_t *ui_components_boxart_init(const char *storage_prefix, const char *game_code, const char *rom_title, file_image_type_t current_image_view);

/**
 * @brief Free the box art component resources.
 * 
 * @param b Pointer to the box art component.
 */
void ui_components_boxart_free(component_boxart_t *b);

/**
 * @brief Draw the box art component.
 * 
 * @param b Pointer to the box art component.
 */
void ui_components_boxart_draw(component_boxart_t *b);

/**
 * @brief Draw the tabs component.
 * 
 * @param text Array of tab labels.
 * @param count Number of tabs.
 * @param selected Index of the selected tab.
 * @param width Width of the tabs.
 */
void ui_components_tabs_draw(const char **text, int count, int selected, float width);

/**
 * @brief Draw the common part of the tabs component.
 * 
 * @param selected Index of the selected tab.
 */
void ui_components_tabs_common_draw(int selected);

/**
 * @brief Draw a value editor component.
 * 
 * @param header_text Array of header text for the values.
 * @param value_text Array of value text to be displayed.
 * @param count Number of values.
 * @param selected Index of the selected value.
 * @param width_adjustment Negative width adjustment of each value box.
 */
void ui_component_value_editor(const char **header_text, const char **value_text, int count, int selected, float width_adjustment);

/**
 * @brief Draw the file info component.
 * 
 * @param filename Name of the file for which to show the information.
 * @param info Metadata information of the file to be displayed.
 */
void ui_components_file_info_draw (char* filename, file_info_t *info);

#endif /* UI_COMPONENTS_H__ */

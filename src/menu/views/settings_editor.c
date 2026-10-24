#include <stdbool.h>
#include "../sound.h"
#include "../settings.h"
#include "../ui_components/constants.h"
#include "utils/utils.h"
#include "views.h"

static bool show_message_reset_settings = false;
static int scroll_line = 0;
static int scroll_max = 0;

static const char *format_switch (bool state) {
    switch (state) {
        case true: return "On";
        case false: return "Off";
    }
}

#ifdef FEATURE_AUTOLOAD_ROM_ENABLED
static void set_loading_progress_bar_enabled_type (menu_t *menu, void *arg) {
    menu->settings.loading_progress_bar_enabled = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}
#endif

static void set_protected_entries_type (menu_t *menu, void *arg) {
    menu->settings.show_protected_entries = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);

    menu->browser.reload = true;
}

static void set_use_saves_folder_type (menu_t *menu, void *arg) {
    menu->settings.use_saves_folder = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}

static void clear_saves_root_directory (menu_t *menu, void *arg) {
    free(menu->settings.saves_root_directory);
    menu->settings.saves_root_directory = strdup("");
    settings_save(&menu->settings);
}

static void set_show_saves_folder_type (menu_t *menu, void *arg) {
    menu->settings.show_saves_folder = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);

    menu->browser.reload = true;
}

static void set_show_save_files_type (menu_t *menu, void *arg) {
    menu->settings.show_save_files = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);

    menu->browser.reload = true;
}

static void set_show_cheat_files_type (menu_t *menu, void *arg) {
    menu->settings.show_cheat_files = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);

    menu->browser.reload = true;
}
    
static void set_soundfx_enabled_type (menu_t *menu, void *arg) {
    menu->settings.soundfx_enabled = (bool)(uintptr_t)(arg);
    sound_use_sfx(menu->settings.soundfx_enabled);
    settings_save(&menu->settings);
}

static void set_bgm_enabled_type (menu_t *menu, void *arg) {
    menu->settings.bgm_enabled = (bool)(uintptr_t)(arg);
    sound_use_bgm(menu->settings.bgm_enabled);
    settings_save(&menu->settings);
}

static void set_pal60_type (menu_t *menu, void *arg) {
    bool pal60_try_enable = (bool)(uintptr_t)(arg);
    tv_type_t tv_type = get_tv_type();
    // FIXME: we can check it is supported by adding a warning and setting it, with a confirmation.
    if (pal60_try_enable && (tv_type == TV_PAL)) {
        //enable it without needing to reboot the console.
        // FIXME: Add message box to press a button as confirmation. 
        // Set VI timing so it will use 60Hz signal.
        vi_set_timing_preset(&VI_TIMING_PAL60);

        // FIXME: timeout and restore to PAL 50Hz if message not shown, 
        //vi_set_timing_preset(&VI_TIMING_PAL);
        
    }
    else if (!pal60_try_enable && (tv_type == TV_PAL)){
        //disable it without needing to reboot the console.
        // Set VI timing so it will use 50Hz signal.
        vi_set_timing_preset(&VI_TIMING_PAL);
        
    }
    else {
        //not PAL, cannot enable PAL60
        pal60_try_enable = false;
    }
    
    menu->settings.pal60_enabled = pal60_try_enable;
    settings_save(&menu->settings);

    // the timing preset resets the output area
    ui_components_screen_margin_apply(menu->settings.screen_margin);
}

static void set_theme (menu_t *menu, void *arg) {
    menu->settings.theme = (int)(uintptr_t)(arg);
    ui_theme_set(menu->settings.theme);
    settings_save(&menu->settings);
}

static void set_favorites_alphabetical (menu_t *menu, void *arg) {
    menu->settings.favorites_alphabetical = (bool)(uintptr_t)(arg);
    menu->bookkeeping.favorites_alphabetical = menu->settings.favorites_alphabetical;
    if (menu->settings.favorites_alphabetical) {
        bookkeeping_favorites_sort(&menu->bookkeeping);
        bookkeeping_save(&menu->bookkeeping);
    }
    settings_save(&menu->settings);
}

static void set_large_text (menu_t *menu, void *arg) {
    menu->settings.large_text = (bool)(uintptr_t)(arg);
    fonts_set_large_text(menu->settings.large_text);
    settings_save(&menu->settings);
}

static void set_screen_margin (menu_t *menu, void *arg) {
    menu->settings.screen_margin = (int)(uintptr_t)(arg);
    ui_components_screen_margin_apply(menu->settings.screen_margin);
    settings_save(&menu->settings);
}

static void set_wrap_file_list_scrolling_type (menu_t *menu, void *arg) {
    menu->settings.wrap_file_list_scrolling = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}

#ifndef FEATURE_AUTOLOAD_ROM_ENABLED
static void set_use_rom_fast_reboot_enabled_type (menu_t *menu, void *arg) {
    menu->settings.rom_fast_reboot_enabled = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}
#endif

#ifdef BETA_SETTINGS
static void set_show_browser_file_extensions_type(menu_t *menu, void *arg) {
    menu->settings.show_browser_file_extensions = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
    menu->browser.reload = true;
}

static void set_show_browser_rom_tags_type (menu_t *menu, void *arg) {
    menu->settings.show_browser_rom_tags = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}

static void set_rumble_enabled_type (menu_t *menu, void *arg) {
    menu->settings.rumble_enabled = (bool)(uintptr_t)(arg);
    settings_save(&menu->settings);
}

// static void set_use_default_settings (menu_t *menu, void *arg) {
//     // FIXME: add implementation
//     menu->browser.reload = true;
// }
#endif

static void remove_background_image (menu_t *menu, void *arg) {
    (void)arg;
    (void)menu;
    ui_components_background_clear();
}

#ifdef FEATURE_AUTOLOAD_ROM_ENABLED
static int get_loading_progress_bar_enabled_current_selection (menu_t *menu) {
    return menu->settings.loading_progress_bar_enabled ? 0 : 1;
}

static component_context_menu_t set_loading_progress_bar_enabled_context_menu = {
    .get_default_selection = get_loading_progress_bar_enabled_current_selection,
    .list = {
        {.text = "On", .action = set_loading_progress_bar_enabled_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_loading_progress_bar_enabled_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};
#endif

static int get_protected_entries_current_selection (menu_t *menu) {
    return menu->settings.show_protected_entries ? 0 : 1;
}

static component_context_menu_t set_protected_entries_type_context_menu = {
    .get_default_selection = get_protected_entries_current_selection,
    .list = {
        {.text = "On", .action = set_protected_entries_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_protected_entries_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_soundfx_enabled_current_selection (menu_t *menu) {
    return menu->settings.soundfx_enabled ? 0 : 1;
}

static component_context_menu_t set_soundfx_enabled_type_context_menu = {
    .get_default_selection = get_soundfx_enabled_current_selection,
    .list = {
        {.text = "On", .action = set_soundfx_enabled_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_soundfx_enabled_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_bgm_enabled_current_selection (menu_t *menu) {
    return menu->settings.bgm_enabled ? 0 : 1;
}

static component_context_menu_t set_bgm_enabled_type_context_menu = {
    .get_default_selection = get_bgm_enabled_current_selection,
    .list = {
        {.text = "On", .action = set_bgm_enabled_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_bgm_enabled_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_use_saves_folder_current_selection (menu_t *menu) {
    return menu->settings.use_saves_folder ? 0 : 1;
}

static component_context_menu_t set_use_saves_folder_type_context_menu = {
    .get_default_selection = get_use_saves_folder_current_selection,
    .list = {
        {.text = "On", .action = set_use_saves_folder_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_use_saves_folder_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_show_saves_folder_current_selection (menu_t *menu) {
    return menu->settings.show_saves_folder ? 0 : 1;
}

static component_context_menu_t set_show_saves_folder_type_context_menu = {
    .get_default_selection = get_show_saves_folder_current_selection,
    .list = {
        {.text = "On", .action = set_show_saves_folder_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_show_saves_folder_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_show_save_files_current_selection (menu_t *menu) {
    return menu->settings.show_save_files ? 0 : 1;
}

static component_context_menu_t set_show_save_files_type_context_menu = {
    .get_default_selection = get_show_save_files_current_selection,
    .list = {
        {.text = "On", .action = set_show_save_files_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_show_save_files_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_show_cheat_files_current_selection (menu_t *menu) {
    return menu->settings.show_cheat_files ? 0 : 1;
}

static component_context_menu_t set_show_cheat_files_type_context_menu = {
    .get_default_selection = get_show_cheat_files_current_selection,
    .list = {
        {.text = "On", .action = set_show_cheat_files_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_show_cheat_files_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_pal60_current_selection (menu_t *menu) {
    return menu->settings.pal60_enabled ? 0 : 1;
}

static component_context_menu_t set_pal60_type_context_menu = {
    .get_default_selection = get_pal60_current_selection,
    .list = {
        {.text = "On", .action = set_pal60_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_pal60_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_wrap_file_list_scrolling_current_selection (menu_t *menu) {
    return menu->settings.wrap_file_list_scrolling ? 0 : 1;
}

static component_context_menu_t set_wrap_file_list_scrolling_context_menu = {
    .get_default_selection = get_wrap_file_list_scrolling_current_selection,
    .list = {
        {.text = "On", .action = set_wrap_file_list_scrolling_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_wrap_file_list_scrolling_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

#ifndef FEATURE_AUTOLOAD_ROM_ENABLED
static int get_use_rom_fast_reboot_current_selection (menu_t *menu) {
    return menu->settings.rom_fast_reboot_enabled ? 0 : 1;
}

static component_context_menu_t set_use_rom_fast_reboot_context_menu = {
    .get_default_selection = get_use_rom_fast_reboot_current_selection,
    .list = {
        {.text = "On", .action = set_use_rom_fast_reboot_enabled_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_use_rom_fast_reboot_enabled_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};
#endif

#ifdef BETA_SETTINGS
static int get_show_browser_file_extensions_current_selection (menu_t *menu) {
    return menu->settings.show_browser_file_extensions ? 0 : 1;
}

static component_context_menu_t set_show_browser_file_extensions_context_menu = {
    .get_default_selection = get_show_browser_file_extensions_current_selection,
    .list = {
        { .text = "On", .action = set_show_browser_file_extensions_type, .arg = (void *)(uintptr_t)(true) },
        { .text = "Off", .action = set_show_browser_file_extensions_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_show_browser_rom_tags_current_selection (menu_t *menu) {
    return menu->settings.show_browser_rom_tags ? 0 : 1;
}

static component_context_menu_t set_show_browser_rom_tags_context_menu = {
    .get_default_selection = get_show_browser_rom_tags_current_selection,
    .list = {
        {.text = "On", .action = set_show_browser_rom_tags_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_show_browser_rom_tags_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_rumble_enabled_current_selection (menu_t *menu) {
    return menu->settings.rumble_enabled ? 0 : 1;
}

static component_context_menu_t set_rumble_enabled_type_context_menu = {
    .get_default_selection = get_rumble_enabled_current_selection,
    .list = {
        {.text = "On", .action = set_rumble_enabled_type, .arg = (void *)(uintptr_t)(true) },
        {.text = "Off", .action = set_rumble_enabled_type, .arg = (void *)(uintptr_t)(false) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};
#endif

static int get_theme_current_selection (menu_t *menu) {
    return menu->settings.theme;
}

// Order and names must match the themes in ui_components/theme.c
static component_context_menu_t set_theme_context_menu = {
    .get_default_selection = get_theme_current_selection,
    .list = {
        {.text = "Navy", .action = set_theme, .arg = (void *)(uintptr_t)(0) },
        {.text = "Red", .action = set_theme, .arg = (void *)(uintptr_t)(1) },
        {.text = "Purple", .action = set_theme, .arg = (void *)(uintptr_t)(2) },
        {.text = "Green", .action = set_theme, .arg = (void *)(uintptr_t)(3) },
        {.text = "Yellow", .action = set_theme, .arg = (void *)(uintptr_t)(4) },
        {.text = "Teal", .action = set_theme, .arg = (void *)(uintptr_t)(5) },
        {.text = "Pink", .action = set_theme, .arg = (void *)(uintptr_t)(6) },
        {.text = "Classic Gray", .action = set_theme, .arg = (void *)(uintptr_t)(7) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_screen_margin_current_selection (menu_t *menu) {
    switch (menu->settings.screen_margin) {
        case 0: return 0;
        case 5: return 1;
        case 10: return 2;
        default: return 3;
    }
}

static component_context_menu_t set_screen_margin_context_menu = {
    .get_default_selection = get_screen_margin_current_selection,
    .list = {
        {.text = "Off", .action = set_screen_margin, .arg = (void *)(uintptr_t)(0) },
        {.text = "Small (5%)", .action = set_screen_margin, .arg = (void *)(uintptr_t)(5) },
        {.text = "Medium (10%)", .action = set_screen_margin, .arg = (void *)(uintptr_t)(10) },
        {.text = "Large (15%)", .action = set_screen_margin, .arg = (void *)(uintptr_t)(15) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_favorites_alphabetical_current_selection (menu_t *menu) {
    return menu->settings.favorites_alphabetical ? 1 : 0;
}

static component_context_menu_t set_favorites_alphabetical_context_menu = {
    .get_default_selection = get_favorites_alphabetical_current_selection,
    .list = {
        {.text = "Manual (new at bottom)", .action = set_favorites_alphabetical, .arg = (void *)(uintptr_t)(false) },
        {.text = "A-Z", .action = set_favorites_alphabetical, .arg = (void *)(uintptr_t)(true) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static int get_large_text_current_selection (menu_t *menu) {
    return menu->settings.large_text ? 1 : 0;
}

static component_context_menu_t set_large_text_context_menu = {
    .get_default_selection = get_large_text_current_selection,
    .list = {
        {.text = "Normal", .action = set_large_text, .arg = (void *)(uintptr_t)(false) },
        {.text = "Large (easier on CRTs)", .action = set_large_text, .arg = (void *)(uintptr_t)(true) },
    COMPONENT_CONTEXT_MENU_LIST_END,
}};

static component_context_menu_t options_context_menu = { .list = {
    { .text = "Text Size", .submenu = &set_large_text_context_menu },
    { .text = "Color Theme", .submenu = &set_theme_context_menu },
    { .text = "Screen Margin", .submenu = &set_screen_margin_context_menu },
    { .text = "Favorites Order", .submenu = &set_favorites_alphabetical_context_menu },
    { .text = "Show Hidden Files", .submenu = &set_protected_entries_type_context_menu },
    { .text = "Sound Effects", .submenu = &set_soundfx_enabled_type_context_menu },
    { .text = "Background Music", .submenu = &set_bgm_enabled_type_context_menu },
    { .text = "Use Saves Folder", .submenu = &set_use_saves_folder_type_context_menu },
    { .text = "Saves Root: Off", .action = clear_saves_root_directory },
    { .text = "Show Saves Folder", .submenu = &set_show_saves_folder_type_context_menu },
    { .text = "Show Save Files", .submenu = &set_show_save_files_type_context_menu },
    { .text = "Show Cheat Files", .submenu = &set_show_cheat_files_type_context_menu },
    { .text = "PAL60 Mode", .submenu = &set_pal60_type_context_menu },
    { .text = "Wrap File List", .submenu = &set_wrap_file_list_scrolling_context_menu },
    #ifdef FEATURE_AUTOLOAD_ROM_ENABLED
    { .text = "ROM Loading Bar", .submenu = &set_loading_progress_bar_enabled_context_menu },
#else
    { .text = "Fast Reboot ROM", .submenu = &set_use_rom_fast_reboot_context_menu },
#endif
#ifdef BETA_SETTINGS
    { .text = "Hide ROM Extensions", .submenu = &set_show_browser_file_extensions_context_menu },
    { .text = "Hide ROM Tags", .submenu = &set_show_browser_rom_tags_context_menu },
    { .text = "Rumble Feedback", .submenu = &set_rumble_enabled_type_context_menu },
    // { .text = "Restore Defaults", .action = set_use_default_settings },
#endif
    { .text = "Remove Background", .action = remove_background_image },

    COMPONENT_CONTEXT_MENU_LIST_END,
}};


static void process (menu_t *menu) {
    if (ui_components_context_menu_process(menu, &options_context_menu)) {
        return;
    }

    // the settings don't fit on screen (with large text even less so), so they scroll
    if (menu->actions.go_down && (scroll_line < scroll_max)) {
        scroll_line++;
        sound_play_effect(SFX_CURSOR);
        return;
    } else if (menu->actions.go_up && (scroll_line > 0)) {
        scroll_line--;
        sound_play_effect(SFX_CURSOR);
        return;
    }

    if (menu->actions.enter) {
        if (show_message_reset_settings) {
            settings_reset_to_defaults();
            menu_show_error(menu, "Reboot N64 to take effect!");
            show_message_reset_settings = false;
        } else {
            ui_components_context_menu_show(&options_context_menu);
        }
        sound_play_effect(SFX_SETTING);
    } else if (menu->actions.back) {
        if (show_message_reset_settings) {
            show_message_reset_settings = false;
        } else {
            menu->next_mode = MENU_MODE_BROWSER;
        }
        sound_play_effect(SFX_EXIT);
    } else if (menu->actions.options){
        show_message_reset_settings = true;
    }
}

static void draw (menu_t *menu, surface_t *d) {
    rdpq_attach(d, NULL);

    ui_components_background_draw();

    ui_components_layout_draw();

	ui_components_main_text_draw(
        STL_DEFAULT,
        ALIGN_CENTER, VALIGN_TOP,
        "MENU SETTINGS EDITOR\n"
        "\n"
    );

    char settings_text[2048];
    snprintf(
        settings_text, sizeof(settings_text),
        "  Default Directory : %s\n"
        "  Saves Root Folder : %s\n\n"
        "To change the following menu settings, press 'A':\n"
        "     Text Size         : %s\n"
        "     Color Theme       : %s\n"
        "     Screen Margin     : %d%%\n"
        "     Favorites Order   : %s\n"
        "     Show Hidden Files : %s\n"
        "     Sound Effects     : %s\n"
        "     Background Music  : %s\n"
        "     Use Saves folder  : %s\n"
        "     Show Saves folder : %s\n"
        "     Show Save files   : %s\n"
        "     Show Cheat files  : %s\n"
        "*    PAL60 Mode        : %s\n"
        "     Wrap File List    : %s\n"
#ifdef FEATURE_AUTOLOAD_ROM_ENABLED
        "     Autoload ROM      : %s\n"
        "     ROM Loading Bar   : %s\n"
#else
        "     Fast Reboot ROM   : %s\n"
#endif
#ifdef BETA_SETTINGS
        "     Hide ROM Extension: %s\n"
        "     Hide ROM Tags     : %s\n"
        "     Rumble Feedback   : %s\n"
#endif
        "\n"
        "* PAL60 may blank the display. To undo it, edit menu/config.ini on the SD card.\n"
        ,
        menu->settings.default_directory,
        (menu->settings.saves_root_directory[0] != '\0') ? menu->settings.saves_root_directory : "Off (next to ROMs)",
        menu->settings.large_text ? "Large" : "Normal",
        ui_theme_name(menu->settings.theme),
        menu->settings.screen_margin,
        menu->settings.favorites_alphabetical ? "A-Z" : "Manual",
        format_switch(menu->settings.show_protected_entries),
        format_switch(menu->settings.soundfx_enabled),
        format_switch(menu->settings.bgm_enabled),
        format_switch(menu->settings.use_saves_folder),
        format_switch(menu->settings.show_saves_folder),
        format_switch(menu->settings.show_save_files),
        format_switch(menu->settings.show_cheat_files),
        format_switch(menu->settings.pal60_enabled),
        format_switch(menu->settings.wrap_file_list_scrolling),
#ifdef FEATURE_AUTOLOAD_ROM_ENABLED
        format_switch(menu->settings.rom_autoload_enabled),
        format_switch(menu->settings.loading_progress_bar_enabled)
#else
        format_switch(menu->settings.rom_fast_reboot_enabled)
#endif
#ifdef BETA_SETTINGS
        ,
        format_switch(menu->settings.show_browser_file_extensions),
        format_switch(menu->settings.show_browser_rom_tags),
        format_switch(menu->settings.rumble_enabled)
#endif
    );

    // show the lines that fit below the two title lines, starting at the scroll position
    int total_lines = 1;
    for (char *c = settings_text; *c; c++) {
        if (*c == '\n') {
            total_lines++;
        }
    }
    int visible_lines = ((LAYOUT_ACTIONS_SEPARATOR_Y - VISIBLE_AREA_Y0 - (TEXT_MARGIN_VERTICAL * 2)) / ui_layout.line_height) - 2;
    scroll_max = MAX(0, total_lines - visible_lines);
    scroll_line = MIN(scroll_line, scroll_max);

    char *visible_text = settings_text;
    for (int line = 0; (line < scroll_line) && *visible_text; visible_text++) {
        if (*visible_text == '\n') {
            line++;
        }
    }

    ui_components_main_text_draw(
        STL_DEFAULT,
        ALIGN_LEFT, VALIGN_TOP,
        "\n\n%s",
        visible_text
    );

    if (scroll_max > 0) {
        ui_components_list_scrollbar_draw(scroll_line, scroll_max + 1, 1);
    }

    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_LEFT, VALIGN_TOP,
        "A: Change\n"
        "B: Back"
    );

    ui_components_actions_bar_text_draw(
        STL_DEFAULT,
        ALIGN_RIGHT, VALIGN_TOP,
        "R: Reset settings\n"
        "\n"
    );

    ui_components_context_menu_draw(&options_context_menu);

    if (show_message_reset_settings) {
        ui_components_messagebox_draw(
            "Reset settings?\n\n"
            "A: Yes, B: Back"
        );
    }

    rdpq_detach_show();
}


void view_settings_init (menu_t *menu) {
    ui_components_context_menu_init(&options_context_menu);
    scroll_line = 0;

}

void view_settings_display (menu_t *menu, surface_t *display) {
    process(menu);
    
    draw(menu, display);
}

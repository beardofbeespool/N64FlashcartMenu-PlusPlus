/**
 * @file confirm.c
 * @brief Yes/no prompt shown before actions that are hard to undo.
 * @ingroup ui_components
 */

#include <stdarg.h>
#include <stdio.h>

#include "../sound.h"
#include "../ui_components.h"

static bool visible = false;
static char message[512];
static void (*confirm_action)(menu_t *menu, void *arg) = NULL;
static void *confirm_arg = NULL;


void ui_components_confirm_show (void (*action)(menu_t *menu, void *arg), void *arg, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    confirm_action = action;
    confirm_arg = arg;
    visible = true;
}

void ui_components_confirm_hide (void) {
    visible = false;
    confirm_action = NULL;
    confirm_arg = NULL;
}

bool ui_components_confirm_process (menu_t *menu) {
    if (!visible) {
        return false;
    }

    if (menu->actions.enter) {
        void (*action)(menu_t *menu, void *arg) = confirm_action;
        void *arg = confirm_arg;
        // hidden first, so the action can show another prompt or an error
        ui_components_confirm_hide();
        sound_play_effect(SFX_SETTING);
        if (action) {
            action(menu, arg);
        }
    } else if (menu->actions.back) {
        ui_components_confirm_hide();
        sound_play_effect(SFX_EXIT);
    }

    // every other button is ignored while the prompt is up
    return true;
}

void ui_components_confirm_draw (void) {
    if (visible) {
        ui_components_messagebox_draw("%s\n\nA: Yes    B: No", message);
    }
}

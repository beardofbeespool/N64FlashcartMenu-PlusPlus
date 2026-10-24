/**
 * @file fonts.h
 * @brief Menu fonts
 * @ingroup menu 
 */

#ifndef FONTS_H__
#define FONTS_H__

#include <stdbool.h>

/**
 * @brief Font type enumeration.
 * 
 * This enumeration defines the different types of fonts that can be used
 * in the menu system.
 */
typedef enum {
    FNT_DEFAULT = 1, /**< Default font type */
} menu_font_type_t;

/**
 * @brief Font style enumeration.
 * 
 * This enumeration defines the different styles of fonts that can be used
 * in the menu system.
 */
typedef enum {
    STL_DEFAULT = 0, /**< Default font style */
    STL_GREEN,       /**< Green font style */
    STL_BLUE,        /**< Blue font style */
    STL_YELLOW,      /**< Yellow font style */
    STL_ORANGE,      /**< Orange font style */
    STL_RED,         /**< Red font style */
    STL_GRAY,        /**< Gray font style */
} menu_font_style_t;

/**
 * @brief Initialize fonts.
 * 
 * This function initializes the fonts used in the menu system. It can load
 * custom fonts from the specified path.
 * 
 * @param custom_font_path Path to the custom font file, used for normal size text only.
 * @param large_text Use the large font, and the layout that fits it.
 */
void fonts_init(char *custom_font_path, bool large_text);

/**
 * @brief Switch between normal and large text, updating the layout to match.
 * 
 * @param large_text Use the large font.
 */
void fonts_set_large_text(bool large_text);

#endif /* FONTS_H__ */

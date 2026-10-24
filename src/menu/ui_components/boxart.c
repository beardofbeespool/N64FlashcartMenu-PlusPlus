/**
 * @file boxart.c
 * @brief Implementation of the boxart UI component.
 * @ingroup ui_components
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../ui_components.h"
#include "../path.h"
#include "../png_decoder.h"
#include "constants.h"
#include "utils/fs.h"

#define OLD_BOXART_DIRECTORY       "menu/boxart"
#define METADATA_BASE_DIRECTORY    "menu/metadata"
#define HOMEBREW_ID_SUBDIRECTORY   "homebrew"

/** @brief Decoded box art is saved next to its PNG with this extension, so later loads skip decoding. */
#define BOXART_CACHE_EXTENSION     ".cache"
#define BOXART_CACHE_MAGIC         (0x42584331) // "BXC1"

/** @brief Header of a box art cache file, followed by the RGBA16 pixels as stored in the surface. */
typedef struct {
    uint32_t magic;
    uint32_t png_size;  /**< Size of the PNG it was decoded from, a different size means the PNG was replaced */
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    uint16_t reserved;
} boxart_cache_header_t;

/**
 * @brief Get the cache file path for a box art PNG.
 *
 * @param png_path Path to the PNG file.
 * @return Newly allocated path with the extension swapped, or NULL.
 */
static char *cache_path_get(const char *png_path) {
    size_t length = strlen(png_path);
    char *extension = strrchr(png_path, '.');
    if (extension) {
        length = extension - png_path;
    }

    char *cache_path = malloc(length + strlen(BOXART_CACHE_EXTENSION) + 1);
    if (cache_path) {
        memcpy(cache_path, png_path, length);
        strcpy(cache_path + length, BOXART_CACHE_EXTENSION);
    }
    return cache_path;
}

/**
 * @brief Load decoded box art from its cache file.
 *
 * A single read of about 35 KB, much faster than decoding the PNG again.
 *
 * @param cache_path Path to the cache file.
 * @param png_size Size of the current PNG, the cache is ignored if it was made from a different one.
 * @return The image, or NULL if there is no usable cache.
 */
static surface_t *cache_load(const char *cache_path, uint32_t png_size) {
    FILE *f = fopen(cache_path, "rb");
    if (f == NULL) {
        return NULL;
    }
    setbuf(f, NULL);

    size_t max_size = sizeof(boxart_cache_header_t) + (TEX_FORMAT_PIX2BYTES(FMT_RGBA16, BOXART_WIDTH_MAX) + 8) * BOXART_HEIGHT_MAX;
    uint8_t *data = malloc(max_size);
    surface_t *image = NULL;

    if (data) {
        size_t read = fread(data, 1, max_size, f);
        boxart_cache_header_t *header = (boxart_cache_header_t *)(data);
        size_t pixels_size = header->stride * header->height;

        if ((read >= sizeof(boxart_cache_header_t)) &&
            (header->magic == BOXART_CACHE_MAGIC) &&
            (header->png_size == png_size) &&
            (header->width > 0) && (header->width <= BOXART_WIDTH_MAX) &&
            (header->height > 0) && (header->height <= BOXART_HEIGHT_MAX) &&
            (read == sizeof(boxart_cache_header_t) + pixels_size)) {
            image = calloc(1, sizeof(surface_t));
            if (image) {
                *image = surface_alloc(FMT_RGBA16, header->width, header->height);
                if (image->buffer && (image->stride == header->stride)) {
                    memcpy(image->buffer, data + sizeof(boxart_cache_header_t), pixels_size);
                } else {
                    if (image->buffer) {
                        surface_free(image);
                    }
                    free(image);
                    image = NULL;
                }
            }
        }
    }

    free(data);
    fclose(f);
    return image;
}

/**
 * @brief Save decoded box art next to its PNG, see cache_load().
 *
 * @param cache_path Path to the cache file.
 * @param png_size Size of the PNG it was decoded from.
 * @param image Decoded image.
 */
static void cache_save(const char *cache_path, uint32_t png_size, surface_t *image) {
    size_t pixels_size = image->stride * image->height;
    uint8_t *data = malloc(sizeof(boxart_cache_header_t) + pixels_size);
    if (data == NULL) {
        return;
    }

    boxart_cache_header_t header = {
        .magic = BOXART_CACHE_MAGIC,
        .png_size = png_size,
        .width = image->width,
        .height = image->height,
        .stride = image->stride,
    };
    memcpy(data, &header, sizeof(header));
    memcpy(data + sizeof(header), image->buffer, pixels_size);

    FILE *f = fopen(cache_path, "wb");
    if (f) {
        setbuf(f, NULL);
        bool written = (fwrite(data, sizeof(header) + pixels_size, 1, f) == 1);
        fclose(f);
        if (!written) {
            remove(cache_path);
        }
    }

    free(data);
}

/**
 * @brief PNG decoder callback for boxart image loading.
 *
 * Sets the loading flag to false and assigns the decoded image to the boxart component.
 *
 * @param err PNG decoder error code.
 * @param decoded_image Pointer to the decoded image surface.
 * @param callback_data Pointer to the boxart component (component_boxart_t *).
 */
static void png_decoder_callback(png_err_t err, surface_t *decoded_image, void *callback_data) {
    component_boxart_t *b = (component_boxart_t *)(callback_data);
    b->loading = false;
    b->image = decoded_image;

    if ((err == PNG_OK) && decoded_image && b->cache_path) {
        cache_save(b->cache_path, b->png_size, decoded_image);
    }
    free(b->cache_path);
    b->cache_path = NULL;
}

/**
 * @brief Initialize and load the boxart component for a game.
 *
 * Attempts to locate and load the appropriate boxart image for the given game code and ROM title.
 *
 * @param storage_prefix The storage prefix (e.g., SD card root).
 * @param game_code The 4-character game code.
 * @param rom_title Title of the ROM (may be NULL). If used, it is sanitized for filesystem safety.
 * @param current_image_view The current image view type (front, back, etc.).
 * @return Pointer to the initialized boxart component, or NULL on failure.
 */
component_boxart_t *ui_components_boxart_init(const char *storage_prefix, const char *game_code, const char *rom_title, file_image_type_t current_image_view) {
    component_boxart_t *b;
    char boxart_path[32];

    if ((b = calloc(1, sizeof(component_boxart_t))) == NULL) {
        return NULL;
    }

    b->loading = true;

    path_t *path = path_init(storage_prefix, METADATA_BASE_DIRECTORY);

    if (game_code[1] == 'E' && game_code[2] == 'D') {
        // This is using a homebrew ROM ID, use the title for the file name instead.
        // Create a null-terminated copy of the title for safe string operations
        char safe_title[21];  // 20 chars + null terminator
        memcpy(safe_title, rom_title, 20);
        safe_title[20] = '\0';
        
        snprintf(boxart_path, sizeof(boxart_path), HOMEBREW_ID_SUBDIRECTORY"/%s", safe_title);
        path_push(path, boxart_path);
    }
    else {
        snprintf(boxart_path, sizeof(boxart_path), "%c/%c/%c/%c", game_code[0], game_code[1], game_code[2], game_code[3]);
        path_push(path, boxart_path);

        if (!directory_exists(path_get(path))) { // Allow boxart to not specify the region code.
            path_pop(path);
        }

        if (!directory_exists(path_get(path))) { // Fallback to the old boxart directory if metadata directory doesn't exist.
            
            // TODO: As a deprecated path, eventually this should be removed.
            path_free(path);
            path = path_init(storage_prefix, OLD_BOXART_DIRECTORY);
            path_push(path, boxart_path);

            if (!directory_exists(path_get(path))) { // Allow boxart to not specify the region code.
                path_pop(path);
            }
        }
    }

    debugf("Boxart: Using path %s\n", boxart_path);

    if (directory_exists(path_get(path))) {
        switch (current_image_view) {
            case IMAGE_GAMEPAK_FRONT:
                path_push(path, "gamepak_front.png");
                break;
            case IMAGE_GAMEPAK_BACK:
                path_push(path, "gamepak_back.png");
                break;
            case IMAGE_BOXART_BACK:
                path_push(path, "boxart_back.png");
                break;
            case IMAGE_BOXART_LEFT:
                path_push(path, "boxart_left.png");
                break;
            case IMAGE_BOXART_RIGHT:
                path_push(path, "boxart_right.png");
                break;
            case IMAGE_BOXART_BOTTOM:
                path_push(path, "boxart_bottom.png");
                break;
            case IMAGE_BOXART_TOP:
                path_push(path, "boxart_top.png");
                break;
            default:
                path_push(path, "boxart_front.png");
                break;
        }

        int64_t png_size = file_exists(path_get(path)) ? file_get_size(path_get(path)) : -1;
        if (png_size > 0) {
            char *cache_path = cache_path_get(path_get(path));

            surface_t *cached = cache_path ? cache_load(cache_path, png_size) : NULL;
            if (cached) {
                free(cache_path);
                b->loading = false;
                b->image = cached;
                path_free(path);
                return b;
            }

            if (png_decoder_start(path_get(path), BOXART_WIDTH_MAX, BOXART_HEIGHT_MAX, png_decoder_callback, b) == PNG_OK) {
                // saved once decoded, see png_decoder_callback()
                b->cache_path = cache_path;
                b->png_size = png_size;
                path_free(path);
                return b;
            }
            free(cache_path);
        }
    }
#ifdef FEATURE_DEPRECATED_FUNCTIONALITY
    else { // deprecated compatibility mode

        char file_name[9];

        // reset the directory path used for boxart.
        path_free(path);
        path = path_init(storage_prefix, OLD_BOXART_DIRECTORY);

        snprintf(file_name, sizeof(file_name), "%c%c%c%c.png", game_code[0], game_code[1], game_code[2], game_code[3]);
        path_push(path, file_name);

        if (file_exists(path_get(path))) {
            if (png_decoder_start(path_get(path), BOXART_WIDTH_MAX, BOXART_HEIGHT_MAX, png_decoder_callback, b) == PNG_OK) {
                path_free(path);
                return b;
            }
        }

        path_pop(path);
        snprintf(file_name, sizeof(file_name), "%c%c%c.png", game_code[0], game_code[1], game_code[2]);
        path_push(path, file_name);

        if (file_exists(path_get(path))) {
            if (png_decoder_start(path_get(path), BOXART_WIDTH_MAX, BOXART_HEIGHT_MAX, png_decoder_callback, b) == PNG_OK) {
                path_free(path);
                return b;
            }
        } else {
            path_pop(path);

            snprintf(file_name, sizeof(file_name), "%c%c.png", game_code[1], game_code[2]);
            path_push(path, file_name);
            if (file_exists(path_get(path))) {
                if (png_decoder_start(path_get(path), BOXART_WIDTH_MAX, BOXART_HEIGHT_MAX, png_decoder_callback, b) == PNG_OK) {
                    path_free(path);
                    return b;
                }
            }
        }
    }
#endif
    // TODO: return default image.

    path_free(path);
    free(b);

    return NULL;
}

/**
 * @brief Free the boxart component and its resources.
 *
 * @param b Pointer to the boxart component.
 */
void ui_components_boxart_free(component_boxart_t *b) {
    if (b) {
        if (b->loading) {
            png_decoder_abort();
        }
        free(b->cache_path);
        if (b->image) {
            surface_free(b->image);
            free(b->image);
        }
        free(b);
    }
}

/**
 * @brief Draw the boxart image or a loading placeholder.
 *
 * Draws the loaded boxart image at the appropriate position, or a placeholder if not loaded.
 *
 * @param b Pointer to the boxart component.
 */
void ui_components_boxart_draw(component_boxart_t *b) {
    int box_x = BOXART_X;
    int box_y = BOXART_Y;
    if (b && b->image && b->image->width <= BOXART_WIDTH_MAX && b->image->height <= BOXART_HEIGHT_MAX) {
        rdpq_mode_push();
            rdpq_set_mode_copy(false);
            if (b->image->height == BOXART_HEIGHT_MAX) {
                box_x = BOXART_X_JP;
                box_y = BOXART_Y_JP;
            } else if (b->image->width == BOXART_WIDTH_DD && b->image->height == BOXART_HEIGHT_DD) {
                box_x = BOXART_X_DD;
                box_y = BOXART_Y_DD;
            }
            rdpq_tex_blit(b->image, box_x, box_y, NULL);
        rdpq_mode_pop();
    } else {
        ui_components_box_draw(
            BOXART_X,
            BOXART_Y,
            BOXART_X + BOXART_WIDTH,
            BOXART_Y + BOXART_HEIGHT,
            BOXART_LOADING_COLOR
        );
    }
}

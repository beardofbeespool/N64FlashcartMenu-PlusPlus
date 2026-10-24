![Build](https://github.com/polprzewodnikowy/N64FlashcartMenu/actions/workflows/build.yml/badge.svg)
![GitHub Org's stars](https://img.shields.io/github/stars/Polprzewodnikowy/N64FlashcartMenu)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/Polprzewodnikowy/N64FlashcartMenu.svg)](http://isitmaintained.com/project/Polprzewodnikowy/N64FlashcartMenu "Average time to resolve an issue")
[![Percentage of issues still open](http://isitmaintained.com/badge/open/Polprzewodnikowy/N64FlashcartMenu.svg)](http://isitmaintained.com/project/Polprzewodnikowy/N64FlashcartMenu "Percentage of issues still open")
[![#yourfirstpr](https://img.shields.io/badge/first--timers--only-friendly-blue.svg)](https://github.com/Polprzewodnikowy/N64FlashcartMenu/blob/main/CONTRIBUTING.md)

# N64FlashcartMenu
An open source menu for N64 flashcarts that aims to support as many as possible.  
This menu is not affiliated with any particular flashcart and does not necessarily expose all possible firmware features.

> [!TIP]
> Help sponsor development [NetworkFusion on Ko-Fi](https://ko-fi.com/networkfusion). Or submit your Pull Request.

> [!TIP]
> New users are invited to read the latest [Documentation / User Guide](./docs/00_index.md).

## About this fork
This is a modified build of [N64FlashcartMenu](https://github.com/Polprzewodnikowy/N64FlashcartMenu) V0.3.5, not an official release. It adds these features for the SummerCart64:
* **Autoload last game:** launching a game makes it the autoload target, so the console boots straight back into your last game. Hold `START` while powering on to get the menu instead.
* **100 favorites and 100 history entries:** instead of 8 each.
* **Split-view file list, Favorites and History:** a one-line-per-entry list on the left, and the selected entry's title, type, size or game details, and box art on the right.
* **Favorites order:** new favorites go to the bottom of the list, and you can arrange them by hand (pick a game up with `L`/`Z`, move it with Up/Down, drop it with `L`/`Z`), or keep them sorted A-Z (*Menu Settings → Favorites Order*).
* **Wrap-around lists:** Up at the top of the file list, Favorites or History jumps to the bottom, and Down at the bottom jumps to the top.
* **Color themes:** Navy (default), Red, Purple, Green, Yellow, Teal, Pink or Classic Gray, chosen in *Menu Settings → Color Theme*. The selected row always gets a contrasting outline.
* **Screen margin:** shrinks the whole picture inward so text stays clear of the screen edges and corners (*Menu Settings → Screen Margin*: Off, 5% (default), 10% or 15%).
* **Helper scripts** for the PC: install the box art pack onto the card, and rename old saves to match your ROMs. See [Scripts](#scripts).
* **Quick launch:** press `START` on a game in the file list, History or Favorites to launch it without opening the game info screen.
* **One-button favorites:** press `L` or `Z` on a game in the file list, or D-pad/C `▲` on the game info screen, to add or remove it. Favorited games are marked with a yellow `*` in the file list.
* **Optional saves root folder:** keep all saves in one folder that mirrors your ROM folders, instead of next to each ROM. Off by default.

### Controls added or changed
| Screen | Button | Action |
|---|---|---|
| File list | `START` on an N64 or emulator ROM | Launch the game immediately |
| File list | `START` on a folder or other file | Settings menu (unchanged) |
| File list | `L` / `Z` on an N64 ROM | Add to / remove from favorites |
| File list | `R` → *Menu settings and info* | Settings menu, for folders that contain only games |
| File list | `R` → *Set current directory as saves root* | Use the current folder as the saves root |
| File list | `A` on a ROM | Game info screen (unchanged) |
| History / Favorites | `START` on an N64 ROM | Launch the game immediately |
| Favorites | `L` / `Z` | Pick up the selected game (amber highlight); Up/Down moves it, `L` / `Z`, `A` or `B` drops it |
| Favorites | `R` | Remove the selected game (unchanged) |
| Game info screen | D-pad `▲` or C-`▲` | Add to / remove from favorites |
| Menu Settings | *Saves Root: Off* | Turn the saves root off again |
| Menu Settings | *Color Theme* | Pick a color theme, applied immediately |
| Menu Settings | *Favorites Order* | *Manual* (new favorites at the bottom, `L`/`Z` reorders) or *A-Z* (always sorted by file name) |
| Menu Settings | *Screen Margin* | Move everything away from the screen edges, applied immediately |

### What changed
* `src/menu/views/load_rom.c`: after a successful ROM load, the ROM's folder and filename are saved as the autoload target. This is skipped when the boot was itself an autoload, so the SD card is not rewritten on every power-on.
* `src/menu/views/load_rom.c`: removed the `free()` calls in `set_autoload_type`, which crashed when setting autoload from the ROM information screen.
* `src/menu/views/startup.c`: the hold-`START` path now assigns heap strings (`strdup("")`) instead of string literals, so setting a new autoload afterwards does not free a literal. Added `#include <string.h>`.
* `src/menu/bookkeeping.h`: raised `FAVORITES_COUNT` and `HISTORY_COUNT` from 8 to 100.
* `src/menu/ini_parser.c`: raised `INI_MAX_PAIRS` from 64 to 512. Each favorite uses 3 keys, so the old cap stopped saving after about 21 favorites.
* `src/menu/bookkeeping.c`: empty history and favorite slots are no longer written to the history file, so its size follows the number of entries actually used. Existing files still load.
* `src/menu/views/history_favorites.c`: the History and Favorites tabs are a split view. The left column is a scrolling one-line-per-game list (file names without extensions) with its scrollbar as the divider; the right panel shows the selected game's title, position in the list, release date, players, Expansion Pak and Rumble Pak info, and box art. The info and box art load after the selection rests for half a second, so scrolling stays fast. `START` launches the selected game, `L`/`Z` reorders favorites, and the lists wrap around.
* `src/menu/bookkeeping.c`: new `bookkeeping_favorite_swap()` for reordering. The new order is saved once, when the game is dropped.
* `src/menu/settings.c`: `wrap_file_list_scrolling` now defaults to on. The settings schema revision is now 2, and older configs have wrap-around switched on once when first loaded.
* `src/menu/ui_components/constants.h`, `src/menu/fonts.c`: new default color palette, and a lighter gray text color for readability on navy.
* `src/menu/ui_components/common.c`: new `ui_components_highlight_draw()`, a filled row with an outline, used by the file list, Favorites/History, context menus and the cheat editor. New `ui_components_screen_margin_apply()`, which uses VI borders to shrink the picture (with resampling, so letters stay even).
* `src/menu/ui_components/theme.c` (new), `constants.h`: the UI colors come from the active theme. `menu.c` applies the saved theme and screen margin at startup; `settings_editor.c` adds both options and re-applies the margin after a PAL60 change (which resets the video output).
* `src/menu/ui_components/game_panel.c` (new): the info panel beside narrow lists, shared by the file list and Favorites/History. Once the selection rests for half a second it loads in three small steps on consecutive frames (ROM header, metadata, then start the box art decode), so no single frame stalls.
* `src/menu/rom_info.c`: new `rom_info_load_header()` and `rom_info_load_database_meta()` for the panel. `rom_config_load()` also looks for `<rom>.ini`, `<rom>.meta` and `<rom>.metadata.ini`, and each missing file means scanning the whole ROM folder, which made the menu stutter in large folders.
* `src/menu/bookkeeping.c`: favorites are appended at the end instead of inserted at the top, a game that is already a favorite keeps its place, and adding to a full list is refused with a message instead of dropping a favorite. New `bookkeeping_favorites_sort()` for the A-Z order (`favorites_alphabetical` in `menu/config.ini`).
* `src/menu/ui_components/file_list.c`, `src/menu/views/browser.c`: the file list draws as a narrow column without file sizes, with the panel showing the entry's type, size, position and favorite status, and full game details for N64 ROMs.
* `scripts/install_boxart.py`, `scripts/match_saves.py` (new): PC helper scripts, see below.
* `src/menu/actions.c`, `src/menu/menu_state.h`: new `favorite` action, triggered once per press of D-pad up or C-up (the analog stick is ignored so a nudge can't toggle it).
* `src/menu/bookkeeping.c`: new `bookkeeping_favorite_find()` to check whether a game is already a favorite.
* `src/menu/views/load_rom.c`: the game info screen shows the favorite status under the title and toggles it with `▲`. A quick launch skips this screen, but still stops at the Expansion Pak warning when a game needs one.
* `src/menu/views/browser.c`, `src/menu/ui_components/file_list.c`: `START` quick launch, `L`/`Z` favorite toggle, the `*` marker, the saves root option, and a *Menu settings and info* entry in the `R` menu.
* `src/menu/views/load_emulator.c`: emulator ROMs can be quick launched from the file list.
* `src/menu/cart_load.c`: one `get_save_path()` helper builds the save path for N64 and emulator games, including the saves root option.
* `src/menu/settings.c`, `src/menu/settings.h`, `src/menu/views/settings_editor.c`: new `saves_root_directory` setting, shown in Menu Settings with an option to turn it off.

### Building
The upstream autoload code is disabled by default, so it must be enabled with a build flag. Open the repo in the `N64FlashcartMenu-ci` dev container, then:

```
mkdir -p build output
make clean
make all FLAGS="-DFEATURE_AUTOLOAD_ROM_ENABLED"
```

Copy `output/sc64menu.n64` to the root of your SD card.

### Scripts
Both scripts need only Python 3 (no extra packages) and run on Linux, Windows and macOS. Replace `/run/media/$USER/SC64` with wherever your SD card is mounted.

**Box art:** `scripts/install_boxart.py` downloads the newest [metadata pack](https://github.com/n64-tools/n64-flashcart-menu-metadata/releases) (about 54 MB, cached in `~/.cache/sc64-tools`) and extracts it into `menu/metadata/` on the card. Files that are already up to date are skipped.
```
python3 scripts/install_boxart.py /run/media/$USER/SC64              # everything
python3 scripts/install_boxart.py /run/media/$USER/SC64 --only-mine  # only N64 games found on the card
```

**Saves:** `scripts/match_saves.py` works out which ROM each save belongs to, from the file name, the title inside the ROM header, region tags and the save size the game uses, and gives it the name the menu expects, `<rom folder>/saves/<rom name>.sav`. It only shows the plan until you add `--apply`.
```
# saves already in n64/saves: rename them in place
python3 scripts/match_saves.py /run/media/$USER/SC64/n64
python3 scripts/match_saves.py /run/media/$USER/SC64/n64 --apply
# saves somewhere else: copy them in
python3 scripts/match_saves.py --saves ~/old-card-backup --roms /run/media/$USER/SC64/n64 --apply
```
* Saves already in the right folder are renamed, and each rename is logged to `saves/rename_log.txt`. If the data has to change, the original is kept beside it as `<name>.orig`. Saves that already have the right name are left alone.
* Saves from another folder are copied, never modified. Existing saves are only replaced with `--overwrite` (keeping a `.bak`).
* EEPROM saves of the wrong size (512 vs 2048 bytes) are trimmed or padded, and EEPROM data is taken out of combined Mupen64Plus/RetroArch `.srm` files.
* SRAM/FlashRAM inside a combined `.srm`, and other size mismatches, are skipped with a reason. Use a tool such as [N64SaveConverter](https://github.com/Ninjiteu/N64SaveConverter) for those.
* `--swap-words` byte-swaps SRAM/FlashRAM saves from Project64 (`.sra`/`.fla`).
* `--next-to-rom`, or `--saves-root <folder> --sd-root <card>`, match the menu's other save locations.
* Matches marked `check` scored lower; read them before using `--apply`, or raise `--min-score`.

### Behavior notes
* The console's reset button goes through startup again, so it relaunches the last game. Hold `START` while resetting to reach the menu.
* If the autoload ROM is moved, renamed or deleted, hold `START` at power-on to get back to the menu.
* The autoload target is stored in the `[autoload]` section of `menu/config.ini` on the SD card.
* In the Favorites tab, Up/Down moves one entry and the fast-scroll button jumps 8. `R` removes the selected favorite.
* When all 100 favorite slots are full, adding another shows "Favorites are full" and changes nothing. History drops its oldest entry when full.
* In A-Z order, games are sorted by file name, and `L`/`Z` in the Favorites tab does nothing. Switching from A-Z back to Manual keeps the sorted order as the starting point.
* Box art in the panels needs the metadata pack in `menu/metadata/` (see `scripts/install_boxart.py`). Without it the panel shows the title and details only.
* Inside a ZIP archive, and for folders, the panel shows the entry type and size only.
* The screen margin uses the VI's resampling, which softens the picture very slightly. Set it to Off on displays where the edges are already clear.
* Favorites and history share one file, which is rewritten on every game launch. With 100 favorites this is roughly 15–20 KB.
* Only N64 ROMs can be favorited. Emulator games (NES, SNES, Game Boy, …) can be quick launched but don't appear in History or Favorites.
* 64DD disks in History or Favorites open their load screen with `START`, the same as `A`.
* With the saves root set to `/saves`, `sd:/snes/Game.sfc` saves to `sd:/saves/snes/Game.sav`. The folders are created on first save. Existing saves are not moved, so move your `.sav` files before switching. While a saves root is set, the *Use Saves Folder* setting is ignored.
* The saves root is stored as `saves_root_directory` in the `[menu]` section of `menu/config.ini`. Empty means off.
* Tested on a SummerCart64 only.

## Flashcart Support
This menu aims to support as many N64 flashcarts as possible.  
The current state of support is:

### Supported
* SummerCart64
* 64Drive

### Work in Progress
* EverDrive-64 (X and V series)
* ED64P (clones)

### Not yet planned
* Doctor V64
* PicoCart
* DaisyDrive


## Current (notable) menu features
* Fully Open Source.
* Loads all known N64 games, even if they are byteswapped.
* Fully emulates the 64DD and loads 64DD disks (SummerCart64 only).
* Emulator support (NES, SNES, GB, GBC, SMS, GG, CHF) ROMs.
* N64 ROM box art image support.
* Background image (PNG) support.
* Comprehensive ROM save database (including homebrew headers).
* Comprehensive ROM information display.
* Real Time Clock support.
* Music playback (MP3).
* Menu sound effects.
* N64 ROM fast reboot option (on reset).
* ROM history and favorites.  
* ROM Datel code editor.
* Zip archive browsing and file extraction.
* Controller Pak backup and restore (including individual notes).
* Game art image switching.


## Aims
* Support as many N64 Flashcarts as possible.
* Be open source, using permissively licensed third-party libraries.
* Be testable in an emulated environment (Ares).
* Encourage active development from community members and N64 FlashCart owners.
* Support as many common mods and features as possible (flashcart dependent).


## Flashcart specific information

### SummerCart64
Download the latest `sc64menu.n64` file from the [releases](https://github.com/Polprzewodnikowy/N64FlashcartMenu/releases/) page, then put it in the root directory of your SD card.  
  
> [!TIP]
> A quick video tutorial can be found here:
>
> [![Video tutorial](https://img.youtube.com/vi/IGX0XXf0wgo/default.jpg)](https://www.youtube.com/shorts/IGX0XXf0wgo)


### 64drive
* Ensure the cart has the latest [firmware](https://64drive.retroactive.be/support.php) installed.
* Download the latest `menu.bin` file from the [releases](https://github.com/Polprzewodnikowy/N64FlashcartMenu/releases/) page, then put it in the root directory of your SD card.


# Contributors
The features in this project were made possible by the [contributors](https://github.com/Polprzewodnikowy/N64FlashcartMenu/graphs/contributors).

# License
This project is released under the [GNU AFFERO GENERAL PUBLIC LICENSE](LICENSE.md) as compatible with all other dependent project licenses.  
Other compatible or commercial license options may be available upon request with permissions of the original `N64FlashcartMenu` project authors / maintainers.  
* [Mateusz Faderewski / Polprzewodnikowy](https://github.com/Polprzewodnikowy)
* [Robin Jones / NetworkFusion](https://github.com/networkfusion)

# Open source software and licenses used
## Libraries
* [libdragon](https://github.com/DragonMinded/libdragon/tree/preview) - [UNLICENSE License](https://github.com/DragonMinded/libdragon/blob/preview/LICENSE.md)
* [libspng](https://github.com/randy408/libspng) - [BSD 2-Clause License](https://github.com/randy408/libspng/blob/master/LICENSE)
* [minimp3](https://github.com/lieff/minimp3) - [CC0 1.0 Universal](https://github.com/lieff/minimp3/blob/master/LICENSE)
* [miniz](https://github.com/richgel999/miniz) - [MIT License](https://github.com/richgel999/miniz/blob/master/LICENSE)
* [dr_flac](https://github.com/mackron/dr_libs) - [MIT License](https://github.com/mackron/dr_libs/blob/master/LICENSE)

## Sounds
See [License](https://pixabay.com/en/service/license-summary/) for the following sounds:
* [Cursor sound](https://pixabay.com/en/sound-effects/click-buttons-ui-menu-sounds-effects-button-7-203601/) by Skyscraper_seven (Free to use)
* [Actions (Enter, Back) sound](https://pixabay.com/en/sound-effects/menu-button-user-interface-pack-190041/) by Liecio (Free to use)
* [Error sound](https://pixabay.com/en/sound-effects/error-call-to-attention-129258/) by Universfield (Free to use)

See [License](https://creativecommons.org/licenses/by/4.0/) for the following sounds:
* [Background Music](https://www.playonloop.com/2017-music-loops/flying-dreams/) POL-flying-dreams-short

## Emulators
* [neon64v2](https://github.com/hcs64/neon64v2) by *hcs64* - [ISC License](https://github.com/hcs64/neon64v2/blob/master/LICENSE.txt)
* [sodium64](https://github.com/Hydr8gon/sodium64) by *Hydr8gon* - [GPL-3.0 License](https://github.com/Hydr8gon/sodium64/blob/master/LICENSE)
* [gb64](https://github.com/lambertjamesd/gb64) by *lambertjamesd* - [MIT License](https://github.com/lambertjamesd/gb64/blob/master/LICENSE)
* [smsPlus64](https://github.com/fhoedemakers/smsplus64) by *fhoedmakers* - [GPL-3.0 License](https://github.com/fhoedemakers/smsplus64/blob/main/LICENSE)
* [Press-F-Ultra](https://github.com/celerizer/Press-F-Ultra) by *celerizer* - [MIT License](https://github.com/celerizer/Press-F-Ultra/blob/master/LICENSE)

## Fonts
* [Firple](https://github.com/negset/Firple) by *negset* - (SIL Open Font License 1.1)
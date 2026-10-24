#!/usr/bin/env python3
"""Match old N64 save files to your ROMs and copy them under the names the menu expects.

The menu loads a game's save from <rom folder>/saves/<rom name>.sav, so saves
from an old card, another flashcart or an emulator have to be renamed to match
your current ROM files. This script works out which ROM each save belongs to by
comparing names (file name and the title inside the ROM header), region tags,
and the save size the game is known to use.

Nothing is changed unless you pass --apply. Saves from other folders are copied,
never modified, and existing saves are never overwritten unless you pass
--overwrite (the old file is then kept as .bak).

Examples:
    # saves already in <rom folder>/saves: rename them in place
    python3 match_saves.py /run/media/$USER/SC64/n64
    python3 match_saves.py /run/media/$USER/SC64/n64 --apply
    # saves somewhere else: copy them into <rom folder>/saves
    python3 match_saves.py --saves ~/old-card/saves --roms /run/media/$USER/SC64/n64 --apply

Saves that are already in their destination folder are renamed, not copied, and
every rename is logged to rename_log.txt in that folder so it can be undone. If
the data has to change (for example an EEPROM save of the wrong size), the
original is kept next to it with an extra .orig extension.

Only uses the Python standard library, so it runs on Linux, Windows and macOS.
"""

import argparse
import difflib
import os
import re
import shutil
import sys
from pathlib import Path

ROM_EXTENSIONS = {".z64", ".n64", ".v64", ".rom"}
SAVE_EXTENSIONS = {".sav", ".eep", ".eeprom", ".sra", ".srm", ".fla", ".flashram", ".ram"}

# Save sizes the menu requires, see SAVE_SIZE in src/flashcart/flashcart.c
SAVE_SIZES = {"E4": 512, "E16": 2048, "S": 32768, "SB": 98304, "S1M": 131072, "F": 131072, "-": 0}
SAVE_NAMES = {"E4": "EEPROM 4K", "E16": "EEPROM 16K", "S": "SRAM", "SB": "SRAM banked", "S1M": "SRAM 1M", "F": "FlashRAM", "-": "no save"}

# Mupen64Plus / RetroArch keep every save type of a game in one .srm file
COMBINED_SRM_SIZE = 0x48800
COMBINED_SRM_EEPROM = (0x0, 0x800)

# Game code -> save type, generated from the database in src/menu/rom_info.c.
# 3-character codes match any region, 4-character codes one region; the first match wins.
SAVE_DATABASE = """
NK4J:S NK4:E4 NSMJ:E4 NSM:E4 NWRJ:E4 NWR:E4 N3HJ:S N3H:- ND3J:E16 ND3:- ND4J:E16 ND4:- NDKJ:E4
NPDJ:E16 NPD:E16 NSVE:E4 NSV:E4 NWTJ:E4 NWT:- ZSE:- ZSA:- CLB:E4 NAB:E4 NAD:E4 NAG:E4 NB6:E4
NBC:E4 NBD:E4 NBH:E4 NBK:E4 NBM:E4 NBN:E4 NBV:E4 NCG:E4 NCH:E4 NCR:E4 NCT:E4 NCU:E4 NCX:E4
NDQ:E4 NDR:E4 NDU:E4 NDY:E4 NEA:E4 NER:E4 NF2:E4 NFG:E4 NFH:E4 NFW:E4 NFX:E4 NFY:E4 NGE:E4
NGL:E4 NGU:E4 NGV:E4 NHA:E4 NHF:E4 NHP:E4 NIC:E4 NIJ:E4 NIR:E4 NJK:E4 NJM:E4 NK2:E4 NKA:E4
NKI:E4 NKT:E4 NLB:E4 NLL:E4 NLR:E4 NMG:E4 NMI:E4 NML:E4 NMO:E4 NMR:E4 NMS:E4 NMU:E4 NMW:E4
NMZ:E4 NN6:E4 NNA:E4 NOS:E4 NO2:E4 NP2:E4 NPG:E4 NPT:E4 NPW:E4 NPY:E4 NRA:E4 NRC:E4 NRS:E4
NS3:E4 NS6:E4 NSA:E4 NSC:E4 NSN:E4 NSS:E4 NSU:E4 NSW:E4 NT6:E4 NTB:E4 NTC:E4 NTJ:E4 NTM:E4
NTN:E4 NTP:E4 NTR:E4 NTW:E4 NTX:E4 NVL:E4 NVY:E4 NWC:E4 NWQ:E4 NWU:E4 NXO:E4 NYK:E4 N3D:E16
NB7:E16 NCW:E16 NCZ:E16 ND2:E16 ND6:E16 NDO:E16 NEP:E16 NEV:E16 NFU:E16 NGC:E16 NGT:E16 NIM:E16
NM8:E16 NMV:E16 NMX:E16 NNB:E16 NPP:E16 NR7:E16 NRZ:E16 NUB:E16 NYS:E16 CFZ:S CPS:S CZL:S NA2:S
NAL:S NB5:S NDD:S NFZ:S NG6:S NGP:S NHY:S NIB:S NJ5:S NJG:S NKG:S NMF:S NOB:S NP4:S NP6:S NPA:S
NPE:S NPM:S NPS:S NRE:S NRI:S NS4:S NSI:S NT3:S NTE:S NUM:S NUT:S NVB:S NVP:S NW2:S NWL:S NWX:S
NYW:S NZL:S CDZ:SB CP2:F NAF:F NCC:F NCV:F NCK:F NDA:F NDP:F NJF:F NKJ:F NM6:F NMQ:F NPF:F
NPN:F NPO:F NRH:F NSQ:F NT9:F NW4:F NZS:F NP3:F N22:- N2M:- N2P:- N2V:- N32:- N3P:- N3T:- N4W:-
N64:- N7I:- N8I:- N8M:- N8W:- N9B:- N9C:- N9F:- N9H:- N9M:- NAC:- NAH:- NAI:- NAM:- NAR:- NAS:-
NAY:- NB2:- NB3:- NB4:- NB8:- NB9:- NBA:- NBE:- NBF:- NBI:- NBJ:- NBL:- NBO:- NBP:- NBQ:- NBR:-
NBS:- NBU:- NBW:- NBX:- NBY:- NBZ:- NCB:- NCD:- NCE:- NCL:- NCO:- NCS:- NDC:- NDE:- NDF:- NDH:-
NDM:- NDN:- NDQ:- NDS:- NDT:- NDW:- NDZ:- NEG:- NET:- NF9:- NFB:- NFD:- NFF:- NFL:- NFO:- NFQ:-
NFR:- NFS:- NG2:- NG5:- NGA:- NGB:- NGD:- NGM:- NGN:- NGR:- NGS:- NGX:- NH5:- NH9:- NHC:- NHG:-
NHK:- NHL:- NHM:- NHN:- NHO:- NHS:- NHT:- NHV:- NHW:- NHX:- NIS:- NIV:- NJ2:- NJ3:- NJA:- NJE:-
NJL:- NJP:- NJQ:- NKE:- NKK:- NKM:- NKR:- NL2:- NLC:- NLG:- NM3:- NM4:- NM9:- NMA:- NMB:- NMD:-
NMJ:- NMM:- NMT:- NMY:- NN2:- NNC:- NNL:- NNM:- NNR:- NNS:- NO7:- NOF:- NOH:- NOM:- NOW:- NP9:-
NPB:- NPC:- NPK:- NPL:- NPR:- NPU:- NPX:- NPZ:- NQ2:- NQ8:- NQ9:- NQB:- NQC:- NQK:- NR2:- NR3:-
NR6:- NRD:- NRG:- NRK:- NRO:- NRP:- NRR:- NRT:- NRU:- NRV:- NRW:- NRX:- NS2:- NSB:- NSD:- NSF:-
NSG:- NSH:- NSJ:- NSK:- NSL:- NSO:- NSP:- NST:- NSX:- NSY:- NSZ:- NT2:- NT4:- NTA:- NTD:- NTF:-
NTH:- NTI:- NTK:- NTO:- NTQ:- NTS:- NTT:- NTU:- NV3:- NV8:- NVC:- NVG:- NVR:- NW3:- NW8:- NWB:-
NWD:- NWF:- NWG:- NWI:- NWK:- NWM:- NWN:- NWO:- NWP:- NWS:- NWV:- NWW:- NWZ:- NX2:- NX3:- NXF:-
NXG:- NY2:- NYP:- NYW:- NZO:-
"""

REGION_SYNONYMS = {
    "usa": "usa", "us": "usa", "u": "usa", "america": "usa", "ntsc-u": "usa",
    "europe": "europe", "eur": "europe", "eu": "europe", "e": "europe", "pal": "europe",
    "japan": "japan", "jpn": "japan", "jp": "japan", "j": "japan", "ntsc-j": "japan",
    "germany": "germany", "ger": "germany", "g": "germany", "de": "germany",
    "france": "france", "fr": "france", "f": "france",
    "australia": "australia", "aus": "australia", "a": "australia",
    "spain": "spain", "s": "spain", "italy": "italy", "i": "italy",
}


def load_database():
    entries = []
    for token in SAVE_DATABASE.split():
        code, save_type = token.split(":")
        entries.append((code, save_type))
    return entries


DATABASE = load_database()


def lookup_save_type(game_code):
    if not game_code:
        return None
    for code, save_type in DATABASE:
        if game_code[:len(code)] == code:
            return save_type
    return None


def read_header(path):
    """Return (internal title, game code) from an N64 ROM header, or (None, None)."""
    try:
        with open(path, "rb") as f:
            header = bytearray(f.read(0x40))
    except OSError:
        return None, None
    if len(header) < 0x40:
        return None, None
    magic = bytes(header[:4])
    if magic == b"\x37\x80\x40\x12":  # byte-swapped (.v64)
        for i in range(0, 0x40, 2):
            header[i], header[i + 1] = header[i + 1], header[i]
    elif magic == b"\x40\x12\x37\x80":  # little-endian (.n64)
        for i in range(0, 0x40, 4):
            header[i:i + 4] = header[i:i + 4][::-1]
    elif magic != b"\x80\x37\x12\x40":
        return None, None
    title = bytes(header[0x20:0x34]).decode("ascii", "replace").strip("\x00 ")
    code = bytes(header[0x3B:0x3F])
    code = code.decode("ascii") if all(32 < c < 127 for c in code) else None
    return title, code


def regions_of(name):
    regions = set()
    for group in re.findall(r"\(([^)]*)\)", name):
        for part in re.split(r"[,/+]", group):
            region = REGION_SYNONYMS.get(part.strip().lower())
            if region:
                regions.add(region)
    return regions


def normalize(name):
    """Turn a file name or header title into comparable words."""
    name = name.lower()
    name = re.sub(r"[\(\[][^\)\]]*[\)\]]", " ", name)        # (USA), [!], (Rev 1) ...
    name = re.sub(r"-[0-9a-f]{8}$", " ", name.strip())       # Mupen64Plus "TITLE-1A2B3C4D"
    name = name.replace("&", " and ").replace("'", "")
    name = re.sub(r"^(.*?),\s*the\b", r"the \1", name)       # "Legend of Zelda, The - ..."
    name = re.sub(r"[^a-z0-9]+", " ", name)
    words = [w for w in name.split() if w not in ("the", "a", "an")]
    return " ".join(words)


def similarity(a, b):
    if not a or not b:
        return 0.0
    sequence = difflib.SequenceMatcher(None, a, b).ratio()
    wa, wb = set(a.split()), set(b.split())
    words = len(wa & wb) / len(wa | wb)
    return 0.5 * sequence + 0.5 * words


class Rom:
    def __init__(self, path):
        self.path = path
        self.stem = path.stem
        self.title, self.code = read_header(path)
        self.save_type = lookup_save_type(self.code)
        self.names = {normalize(self.stem)}
        if self.title:
            self.names.add(normalize(self.title))
        self.regions = regions_of(self.stem)


class Save:
    def __init__(self, path):
        self.path = path
        self.size = path.stat().st_size
        self.name = normalize(path.stem)
        self.regions = regions_of(path.stem)


def size_fit(save, rom):
    """Score adjustment and note for how well the save's size fits the game's save type."""
    if rom.save_type is None:
        return 0.0, "save type unknown"
    expected = SAVE_SIZES[rom.save_type]
    if rom.save_type == "-":
        return -0.2, "game has no cartridge save"
    if save.size == expected:
        return 0.1, "size matches"
    if rom.save_type in ("E4", "E16") and save.size in (512, 2048):
        return 0.0, "EEPROM size will be adjusted"
    if save.size == COMBINED_SRM_SIZE:
        return 0.0, "combined emulator .srm"
    return -0.25, f"size {save.size} but game uses {SAVE_NAMES[rom.save_type]} ({expected})"


def score(save, rom):
    name_score = max(similarity(save.name, n) for n in rom.names)
    bonus, note = size_fit(save, rom)
    if save.regions and rom.regions:
        bonus += 0.05 if (save.regions & rom.regions) else -0.15
    return name_score + bonus, note


def convert(save, rom, swap_words):
    """Return the save data in the format the menu expects, or (None, reason)."""
    data = save.path.read_bytes()
    expected = SAVE_SIZES.get(rom.save_type) if rom.save_type else None

    if expected is None:
        # unknown game, copy as is and let the menu check the size
        return data, None
    if len(data) == COMBINED_SRM_SIZE and len(data) != expected:
        if rom.save_type in ("E4", "E16"):
            start, end = COMBINED_SRM_EEPROM
            return data[start:start + expected], None
        return None, "SRAM/FlashRAM inside a combined .srm needs converting by hand (e.g. N64SaveConverter)"
    if rom.save_type in ("E4", "E16") and len(data) != expected and len(data) in (512, 2048):
        # only the first bytes are used by a 4K game; a 16K game's extra space starts blank
        data = data[:expected] if len(data) > expected else data + b"\xff" * (expected - len(data))
    if len(data) != expected:
        return None, f"size {len(data)} does not fit {SAVE_NAMES[rom.save_type]} ({expected})"
    if swap_words and rom.save_type in ("S", "SB", "S1M", "F"):
        data = b"".join(data[i:i + 4][::-1] for i in range(0, len(data), 4))
    return data, None


def destination_for(rom, args):
    if args.saves_root:
        relative = rom.path.parent.resolve().relative_to(args.sd_root.resolve())
        return args.saves_root / relative / f"{rom.stem}.sav"
    if args.next_to_rom:
        return rom.path.with_suffix(".sav")
    return rom.path.parent / "saves" / f"{rom.stem}.sav"


def find_files(folder, extensions, skip=()):
    for dirpath, dirnames, filenames in os.walk(folder):
        dirnames[:] = [d for d in dirnames if d not in skip]
        for filename in filenames:
            if Path(filename).suffix.lower() in extensions:
                yield Path(dirpath) / filename


def main():
    parser = argparse.ArgumentParser(description="Match old N64 saves to ROMs and give them matching names.")
    parser.add_argument("folder", type=Path, nargs="?", help="ROM folder whose saves are in its saves subfolder, e.g. /run/media/$USER/SC64/n64")
    parser.add_argument("--saves", type=Path, help="folder with the old save files, searched recursively (default: <folder>/saves)")
    parser.add_argument("--roms", type=Path, help="folder with your N64 ROMs, searched recursively (default: <folder>)")
    parser.add_argument("--apply", action="store_true", help="rename or copy the matched saves (default: only show the plan)")
    parser.add_argument("--min-score", type=float, default=0.6, help="lowest match score to accept (default 0.6)")
    parser.add_argument("--overwrite", action="store_true", help="replace saves that already exist, keeping a .bak copy")
    parser.add_argument("--next-to-rom", action="store_true", help="put saves next to the ROM instead of in a saves folder")
    parser.add_argument("--saves-root", type=Path, help="put saves under this folder, mirroring the ROM folders (needs --sd-root)")
    parser.add_argument("--sd-root", type=Path, help="SD card mount point, used with --saves-root")
    parser.add_argument("--swap-words", action="store_true", help="byte-swap SRAM/FlashRAM saves from Project64 (.sra/.fla)")
    args = parser.parse_args()

    if args.saves_root and not args.sd_root:
        sys.exit("--saves-root needs --sd-root")
    if args.folder:
        args.roms = args.roms or args.folder
        args.saves = args.saves or (args.folder / "saves")
    if not args.roms or not args.saves:
        parser.error("give a ROM folder, or both --saves and --roms")

    roms = [Rom(p) for p in find_files(args.roms.expanduser(), ROM_EXTENSIONS, skip=("saves",))]
    saves = [Save(p) for p in find_files(args.saves.expanduser(), SAVE_EXTENSIONS)]
    if not roms:
        sys.exit(f"No N64 ROMs found in {args.roms}")
    if not saves:
        sys.exit(f"No save files found in {args.saves}")
    print(f"{len(saves)} saves, {len(roms)} ROMs\n")

    candidates = []
    for s in saves:
        for r in roms:
            value, note = score(s, r)
            candidates.append((value, s, r, note))
    candidates.sort(key=lambda c: c[0], reverse=True)

    # best matches first, each save and each ROM used once
    matched, used_saves, used_roms = [], set(), set()
    for value, s, r, note in candidates:
        if value < args.min_score or id(s) in used_saves or id(r) in used_roms:
            continue
        matched.append((value, s, r, note))
        used_saves.add(id(s))
        used_roms.add(id(r))

    changed = skipped = already = 0
    for value, s, r, note in sorted(matched, key=lambda m: m[1].path.name.lower()):
        destination = destination_for(r, args)
        if s.path.resolve() == destination.resolve():
            already += 1
            continue
        in_place = s.path.parent.resolve() == destination.parent.resolve()
        confidence = "high" if value >= 0.8 else "check"
        action = "rename to" if in_place else "copy to"
        print(f"[{confidence} {min(value, 1.0):.2f}] {s.path.name}\n    {action} {destination.name if in_place else destination}\n       ({note})")
        data, problem = convert(s, r, args.swap_words)
        if problem:
            print(f"       SKIPPED: {problem}")
            skipped += 1
            continue
        if destination.exists() and not args.overwrite:
            print("       SKIPPED: a save already exists there (use --overwrite)")
            skipped += 1
            continue
        if args.apply:
            destination.parent.mkdir(parents=True, exist_ok=True)
            if destination.exists():
                shutil.copy2(destination, destination.with_suffix(".sav.bak"))
            if in_place:
                original = s.path.read_bytes()
                if data == original:
                    os.replace(s.path, destination)
                else:
                    # keep the untouched original beside the converted save
                    destination.write_bytes(data)
                    os.replace(s.path, s.path.with_name(s.path.name + ".orig"))
                with open(destination.parent / "rename_log.txt", "a", encoding="utf-8") as log:
                    log.write(f"{s.path.name} -> {destination.name}\n")
            else:
                destination.write_bytes(data)
            changed += 1

    unmatched = [s for s in saves if id(s) not in used_saves]
    if unmatched:
        print(f"\nNo confident match for {len(unmatched)} saves:")
        for s in sorted(unmatched, key=lambda s: s.path.name.lower()):
            best = next((c for c in candidates if c[1] is s), None)
            hint = f" (closest: {best[2].stem}, score {best[0]:.2f})" if best else ""
            print(f"    {s.path.name}{hint}")

    print()
    if already:
        print(f"{already} saves already have the right name.")
    if args.apply:
        print(f"Renamed or copied {changed} saves, skipped {skipped}.")
    else:
        print(f"Dry run: {len(matched) - skipped - already} saves would be renamed or copied, {skipped} skipped. Check the list, then add --apply.")


if __name__ == "__main__":
    main()

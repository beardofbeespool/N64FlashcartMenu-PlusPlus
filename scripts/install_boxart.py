#!/usr/bin/env python3
"""Install the N64FlashcartMenu box art and game metadata pack onto an SD card.

Downloads the newest release-metadata.zip from
https://github.com/n64-tools/n64-flashcart-menu-metadata/releases and extracts
it to <sd card>/menu/metadata/, where the menu looks for box art.

Examples:
    python3 install_boxart.py /run/media/$USER/SC64
    python3 install_boxart.py /run/media/$USER/SC64 --only-mine
    python3 install_boxart.py /run/media/$USER/SC64 --zip ~/Downloads/release-metadata.zip

Only uses the Python standard library, so it runs on Linux, Windows and macOS.
"""

import argparse
import json
import os
import sys
import urllib.request
import zipfile
from pathlib import Path

REPO = "n64-tools/n64-flashcart-menu-metadata"
ASSET_NAME = "release-metadata.zip"
ROM_EXTENSIONS = {".z64", ".n64", ".v64", ".rom"}
USER_AGENT = "sc64-install-boxart"


def fetch_json(url):
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def find_release(tag):
    # Releases are marked as pre-releases, so /releases/latest returns 404; list them instead.
    releases = fetch_json(f"https://api.github.com/repos/{REPO}/releases?per_page=20")
    for release in releases:
        if tag and release["tag_name"] != tag:
            continue
        for asset in release["assets"]:
            if asset["name"] == ASSET_NAME:
                return release["tag_name"], asset["browser_download_url"], asset["size"]
    raise SystemExit(f"No {ASSET_NAME} found in the releases of {REPO}" + (f" for tag {tag}" if tag else ""))


def download(url, destination, size):
    destination.parent.mkdir(parents=True, exist_ok=True)
    partial = destination.with_suffix(".part")
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(request, timeout=60) as response, open(partial, "wb") as out:
        done = 0
        while True:
            chunk = response.read(1 << 20)
            if not chunk:
                break
            out.write(chunk)
            done += len(chunk)
            print(f"\r  Downloading: {done / 1e6:5.1f} / {size / 1e6:.1f} MB", end="", flush=True)
    print()
    partial.replace(destination)


def read_game_code(path):
    """Return the 4-character game code from an N64 ROM header, or None."""
    try:
        with open(path, "rb") as f:
            header = bytearray(f.read(0x40))
    except OSError:
        return None
    if len(header) < 0x40:
        return None
    magic = bytes(header[:4])
    if magic == b"\x37\x80\x40\x12":  # byte-swapped (.v64)
        for i in range(0, 0x40, 2):
            header[i], header[i + 1] = header[i + 1], header[i]
    elif magic == b"\x40\x12\x37\x80":  # little-endian (.n64)
        for i in range(0, 0x40, 4):
            header[i:i + 4] = header[i:i + 4][::-1]
    elif magic != b"\x80\x37\x12\x40":  # not a big-endian (.z64) ROM either
        return None
    code = bytes(header[0x3B:0x3F])
    if not all(32 < c < 127 for c in code):
        return None
    return code.decode("ascii")


def scan_game_codes(sd_root):
    codes = set()
    for dirpath, dirnames, filenames in os.walk(sd_root):
        # the menu's own folder never holds games
        if Path(dirpath) == sd_root and "menu" in dirnames:
            dirnames.remove("menu")
        for filename in filenames:
            if Path(filename).suffix.lower() in ROM_EXTENSIONS:
                code = read_game_code(Path(dirpath) / filename)
                if code:
                    codes.add(code)
    return codes


def wanted(parts, game_ids):
    """parts is the path inside metadata/, e.g. ['N', 'S', 'M', 'E', 'boxart_front.png']."""
    if game_ids is None:
        return True
    if parts[0] == "homebrew":
        return True
    return len(parts) >= 4 and "".join(parts[:3]) in game_ids


def extract(zip_path, target, game_ids, force):
    written = skipped = 0
    with zipfile.ZipFile(zip_path) as archive:
        members = [m for m in archive.infolist() if not m.is_dir()]
        for index, member in enumerate(members, 1):
            parts = member.filename.split("/")
            # refuse anything that could escape the target folder
            if parts[0] != "metadata" or any(p in ("", ".", "..") for p in parts[1:]) or member.filename.startswith("/"):
                continue
            parts = parts[1:]
            if not wanted(parts, game_ids):
                continue
            destination = target.joinpath(*parts)
            if destination.exists() and not force and destination.stat().st_size == member.file_size:
                skipped += 1
                continue
            destination.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(member) as source, open(destination, "wb") as out:
                out.write(source.read())
            written += 1
            if written % 50 == 0:
                print(f"\r  Copied {written} files ({index}/{len(members)} checked)", end="", flush=True)
    print(f"\r  Copied {written} files, {skipped} already up to date." + " " * 20)


def main():
    parser = argparse.ArgumentParser(description="Install N64FlashcartMenu box art and metadata onto an SD card.")
    parser.add_argument("sd_root", type=Path, help="where the SD card is mounted, e.g. /run/media/$USER/SC64")
    parser.add_argument("--only-mine", action="store_true", help="only install art for N64 ROMs found on the card")
    parser.add_argument("--zip", type=Path, help="use an already downloaded release-metadata.zip")
    parser.add_argument("--release", help="install a specific release tag instead of the newest")
    parser.add_argument("--force", action="store_true", help="overwrite files that already exist")
    args = parser.parse_args()

    sd_root = args.sd_root.expanduser().resolve()
    if not sd_root.is_dir():
        sys.exit(f"{sd_root} is not a folder. Is the SD card mounted?")
    if not (sd_root / "sc64menu.n64").exists() and not (sd_root / "menu").is_dir():
        print(f"Warning: {sd_root} has no sc64menu.n64 or menu folder, continuing anyway.")

    if args.zip:
        zip_path = args.zip.expanduser()
        if not zip_path.is_file():
            sys.exit(f"{zip_path} not found")
    else:
        tag, url, size = find_release(args.release)
        cache = Path(os.environ.get("XDG_CACHE_HOME", Path.home() / ".cache")) / "sc64-tools"
        zip_path = cache / f"metadata-{tag}.zip"
        print(f"Metadata release {tag}")
        if zip_path.exists() and zip_path.stat().st_size == size:
            print(f"  Using cached download {zip_path}")
        else:
            download(url, zip_path, size)

    game_ids = None
    if args.only_mine:
        print("Scanning the card for N64 ROMs...")
        codes = scan_game_codes(sd_root)
        game_ids = {code[:3] for code in codes}
        print(f"  Found {len(codes)} different games.")
        if not game_ids:
            sys.exit("No N64 ROMs found, nothing to install. Run without --only-mine to install everything.")

    target = sd_root / "menu" / "metadata"
    print(f"Installing into {target}")
    extract(zip_path, target, game_ids, args.force)
    print("Done. Eject the card safely before removing it.")


if __name__ == "__main__":
    main()

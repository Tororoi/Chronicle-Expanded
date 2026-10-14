# Chronicle-Expanded

> **Work in progress:** this repository is the move of the Dark Cloud Expanded mod to the PC port of
> [Chronicle](https://github.com/TheMoonPeople/Chronicle), the Dark Cloud decompilation project. Everything
> below this section is Chronicle's own README.

## Pulling in upstream changes

`upstream` is Chronicle (`TheMoonPeople/Chronicle`, fetch only); `origin` is this repository.

```sh
git fetch upstream
git merge --no-edit upstream/master
git submodule update --init --recursive   # picks up any change to the Kazarov pin
git push
```

Conflicts land on the guarded edits described below; keep the `#ifdef CHRONICLE_EXPANDED` around them.

## Conventions

The mod keeps its changes to Chronicle's own files small, so upstream merges stay easy and the PS2 build
still matches retail.

- **The switch.** The CMake option `CHRONICLE_EXPANDED` (on by default) defines the macro of the same name
  for the port build only. `-DCHRONICLE_EXPANDED=OFF` builds the vanilla port.
- **The mod's code lives in `expanded/`:**
  - `expanded/src/`: compiled into the port. Functions are in namespace `expanded`.
  - `expanded/include/expanded/`: headers, included as `"expanded/<name>.hpp"`.
  - `expanded/src/data/`: what is built from the extracted disc data (see "Game data built at
    extraction"); it includes no game header and is compiled into `dcdata` too.
  - `expanded/tests/`: `*_test.cpp` cases, run with the port's tests.
- **Edits to Chronicle's files (`ps2/`, `port/`, `tools/dcdata/`) are guarded hooks:**
  - Wrap each in `#ifdef CHRONICLE_EXPANDED`; one that replaces code keeps retail's in the `#else`.
  - Keep each to a call into `expanded::` or a constant; the logic lives in `expanded/src`.
  - Don't reformat, rename or move the code around them.
  - `grep -rn CHRONICLE_EXPANDED ps2 port tools/dcdata` lists every one.

  ```cpp
  #ifdef CHRONICLE_EXPANDED
      if (!(heal_counter < expanded::kHealPeriod)) {
  #else
      if (!(heal_counter < 240)) {
  #endif
  ```
- **The truncation report.** Edits to `ps2/` and `port/` shift `docs/port/truncations.md`'s line numbers,
  which CI checks on Linux: regenerate it with `scripts/port/truncations.py --build <Linux build dir>`,
  and on a merge conflict in it, take either side and regenerate.
- **Whole-function copies** only when rewriting most of a function: put the copy in `expanded/src` as a
  `PORT_OVERRIDE`, under the retail function's name.
- **Fixes to retail bugs go to Chronicle first**, from a fork of Chronicle on a branch from
  `upstream/master`, and are cherry-picked here if they are needed before upstream merges them.
- **Comments** are short and say what the code does.

## Game data built at extraction

What the mod makes from the disc's own files can't ship with it, so `dcdata extract` builds it into the
data directory after extracting, and the game's first-run extraction does the same; the vanilla build
writes none of it. For a data directory extracted before, rebuild just these files:

```sh
port/build/pc/dcdata expanded data
```

It writes only what changed and exits 1 if anything failed to build. What it builds:

- `expanded/town/<ally>.chr`: each ally's town model (`expanded/docs/town-models.md`).

---

# Chronicle

[![NTSC Progress]](https://decomp.dev/TheMoonPeople/Chronicle/ntsc)
[![PAL Progress]](https://decomp.dev/TheMoonPeople/Chronicle/pal)

[NTSC Progress]: https://decomp.dev/TheMoonPeople/Chronicle/ntsc.svg?mode=shield&label=NTSC&measure=matched_code_percent
[PAL Progress]: https://decomp.dev/TheMoonPeople/Chronicle/pal.svg?mode=shield&label=PAL&measure=matched_code_percent
[progress_link]: https://decomp.dev/TheMoonPeople/Chronicle/

[<img src="https://decomp.dev/TheMoonPeople/Chronicle/ntsc.svg?w=512&h=256" width="512" height="256" alt="A visual">][progress_link]

Chronicle is a decompilation project and port of Dark Cloud for the PlayStation 2.

This project produces 100% matching text and data sections for the main executable for the NTSC 1.02 and PAL review versions, and 100% matching `TITLE.BIN` and `DUN.BIN` overlays. Matching the main executable's debug symbols may be explored in the future.

# Building and running

1. Clone the repository with `git clone --recurse-submodules https://github.com/TheMoonPeople/Chronicle.git`.
   The build system is Kazarov, in `tools/kazarov`; the first build fetches it if it is missing.
2. Place your disc image in `rom/<region>/`: `rom/pal/Dark Cloud (PAL).iso` for the PAL release, or
   `rom/ntsc/Dark Cloud (NTSC).iso` for NTSC 1.02. An image at `rom/Dark Cloud (<REGION>).iso` is also accepted.
3. Run `./build.sh <configuration>`, or `build.bat <configuration>` on Windows.

`./run.sh <configuration> [arguments]` builds the same way and then launches the result. PS2
configurations boot the disc image in PCSX2; port configurations start the game, with the arguments
passed on to it.

| Configuration | Builds |
|---|---|
| `ps2-pal` (`ps2`, `pal`) | the PAL PS2 executable and disc image |
| `ps2-ntsc` (`ntsc`) | the NTSC 1.02 PS2 executable and disc image |
| `linux-x64-release` (`linux`) | the optimized Linux port, in `port/build/pc-release` |
| `linux-x64` (`linux-x64-debug`) | the Debug Linux port, in `port/build/pc` |
| `macos-arm64-release` (`macos`) | the optimized Apple Silicon port, in `port/build/macos-arm64-release` |
| `macos-arm64` | the Debug Apple Silicon port, in `port/build/macos-arm64` |
| `windows-x64-release` (`windows`) | the optimized Windows port |
| `windows-x64` | the Debug Windows port |

With no configuration, the scripts use the port for the host's operating system. PS2 builds must name
their configuration. The Linux and PS2 configurations build in a container, which needs Docker or Podman;
the macOS and Windows ports build on the host, with the dependencies in `docs/MACOS.md` and `docs/WINDOWS.md` installed.

`CLEAN=1` discards the configuration's build directory before building, and `JOBS=N` sets the number of
parallel jobs. The configurations and their settings are listed in `kazarov.conf` and `tools/kazarov/README.md`.

## The PC port

`PLATFORM=PC` builds the game's code as a native Linux, macOS and Windows program on SDL3 and Vulkan
(`docs/PC.md`, `docs/MACOS.md`). On Linux it is packaged as a Flatpak, built by CI as
`chronicle.flatpak`; `docs/FLATPAK.md` covers installing it. It needs your own PAL disc: the first
start asks for the disc image and extracts the game's files from it.

### Translations and the message font

Game text can come from JSON language files (one per language, laid out as Minecraft's are) and be drawn from a
TrueType font so it stays sharp at any window size. The first start writes the language files from your own
game data; the Dark Cloud community font is bundled. See `docs/LOCALIZATION.md`.

## Repository layout

- `ps2/src`, `ps2/include`: matching game sources and headers.
- `ps2/config`, `ps2/asm`: per-region split configuration and generated reference assembly.
- `ps2/CMakeLists.txt`, `ps2/cmake`: PS2 build rules.
- `port/src`, `port/include`: native replacements and platform support.
- `port/CMakeLists.txt`, `port/flatpak`: native build rules and Linux packaging.
- `scripts`: build, port and tooling scripts.
- `tools`: compilers, the Kazarov build system (`tools/kazarov`) and helpers.
- `rom`: disc images and their checksums.

## Documentation

Source documentation is available on [GitHub Pages](https://themoonpeople.github.io/Chronicle/).

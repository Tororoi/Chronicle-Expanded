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
  - `expanded/include/expanded/`: headers, included as `"expanded/<folder>/<name>.hpp"`.
  - `expanded/tests/`: `<name>_test.cpp` cases, run with the port's tests. With `DC_TEST_DATA` naming an
    extracted data directory, `ExpandedData.BuildsWhatTheModsToolsBuild` builds everything `dcdata expanded`
    makes and checks each file's bytes; it takes a few seconds (`ctest -R ExpandedData --timeout 60`). A
    change that is meant to alter a built file updates its hash there.
    A case puts back what it changes, so the cases pass in any order in one process: `test_support.hpp`
    has a scratch folder, settings and a data directory set for one case, and a floor of monsters set up as
    the game sets them up. A case that rolls seeds `SeedModRandom` first.
  - `expanded/lang/`: the mod's own text in each language (`en_us.json`...): its monsters' and abilities'
    names and its new messages, keyed as `docs/LOCALIZATION.md` describes. The build copies it to
    `lang/expanded/` beside the executable, read after `lang/` and before the save folder's `lang/`, so a
    translation there still wins.
  - `expanded/docs/`: how the larger features work (`bosses.md`, `gemrons.md`, `town-models.md`), and the
    miniboss loot tables (`miniboss-loot.md`).
  - All three use the same folders, and a file in a folder drops the folder's name (`weapons/stats.cpp`,
    not `weapons/weapon_stats.cpp`):
    - `core/`: shared plumbing: options, per-save state, menu text, languages.
    - `data/`: what is built from the extracted disc data (see "Game data built at extraction"); it
      includes no game header and is compiled into `dcdata` too.
    - `debug/`: developer tools that only debug mode shows, such as the enemy roster.
    - `dungeon/`: dungeon systems shared by several features, such as the shot effects.
    - `enemies/`: monsters: the stat scaling every stat change goes through, the features built on it, and how
      monsters move, act and revive.
    - `items/`: changes to the game's items (non-weapon).
    - `status/`: status effects and new abilities' effects on monsters and the player (Confuse, poison,
      freeze).
    - `town/`: town features.
    - `weapons/`: weapon tables (stats, abilities, buildups), ownership, Super Steve's sphere
      inheritance, and helpers several weapons share (`kick_back`).
    - `weapons/<character>/<weapon>`: one weapon's effect, with its Super Steve sphere form in the
      same file.
- **Edits to Chronicle's files (`ps2/`, `port/`, `tools/dcdata/`) are guarded hooks:**
  - Wrap each in `#ifdef CHRONICLE_EXPANDED`; one that replaces code keeps retail's in the `#else`.
  - Keep each to a call into `expanded::` or a constant; the logic lives in `expanded/src`.
  - Where several features act at one site, the hook calls one dispatch (`dungeon/dungeon_hooks`) that calls
    them in turn. A hook repeated through a file's functions can be a file-local helper holding the `#ifdef`
    (`ScriptMonster` in runscript_opcodes.cpp).
  - Don't reformat, rename or move the code around them.
  - Hook the copy of a function that runs: port/src replaces some of ps2/src's (`PORT_OVERRIDE`), and a hook
    in a replaced one never runs. The build stops on one (`expanded/cmake/check_hooks.py`).
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
- **The changelog.** A PR that changes behaviour adds its entry to `CHANGELOG.md`; an upstream sync adds the
  new upstream changes under each section's "From Chronicle" group.

## Game data built at extraction

What the mod makes from the disc's own files can't ship with it, so `dcdata extract` builds it into the
data directory after extracting, and the game's first-run extraction does the same; the vanilla build
writes none of it. For a data directory extracted before, rebuild just these files:

```sh
port/build/pc/dcdata expanded data
```

It writes only what changed and exits 1 if anything failed to build. What it builds:

- `expanded/town/<ally>.chr`: each ally's town model (`expanded/docs/town-models.md`).
- `expanded/monstor/c16a.chr`: Minotaur Joe with his defeat scene's stagger and fall, for his death on a
  regular floor (`expanded/docs/bosses.md`).
- `expanded/chara/spirit_cat.chr`: the Divine Beast Title's cat, the s86 town cat with the e01 town cat's
  float-up grafted into its motion.
- `expanded/enemies/script_damage.txt`: each monster script's strongest melee hit and shot, which the
  enemy stat scaling starts from; without it monster damage is left as it is.

---

<div align="center">

<img src="https://i.imgur.com/pCMkgRM.png" alt="Chronicle" width="700">

**A decompilation project and modern port of Dark Cloud for the PlayStation 2.**

[![Documentation](https://img.shields.io/badge/Documentation-2563EB?style=for-the-badge&logo=readthedocs&logoColor=white)](https://themoonpeople.github.io/Chronicle/)
[![Discord](https://img.shields.io/badge/Discord-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/RGuE998sDY)

[![NTSC Progress](https://decomp.dev/TheMoonPeople/Chronicle/ntsc.svg?mode=shield&label=NTSC&measure=matched_code_percent)](https://decomp.dev/TheMoonPeople/Chronicle/ntsc)
[![PAL Progress](https://decomp.dev/TheMoonPeople/Chronicle/pal.svg?mode=shield&label=PAL&measure=matched_code_percent)](https://decomp.dev/TheMoonPeople/Chronicle/pal)

[<img src="https://decomp.dev/TheMoonPeople/Chronicle/ntsc.svg?w=512&h=256" width="512" alt="Decompilation progress">](https://decomp.dev/TheMoonPeople/Chronicle/)

</div>

## Overview

**Chronicle** is a decompilation project and port of _Dark Cloud_ for the PlayStation 2.

The project aims to produce **100% matching text and data sections** for:

- The main executable of the **NTSC 1.02** release.
- The main executable of the **PAL review** version.
- The `TITLE.BIN` and `DUN.BIN` overlays.

## Building and Running

### Prerequisites

- A copy of _Dark Cloud_ (ideally the PAL version).

### 1. Clone the repository

```bash
git clone --recurse-submodules https://github.com/TheMoonPeople/Chronicle.git
cd Chronicle
```

### 2. Provide a disc image

Place your disc image in the corresponding region directory:

| Region    | Path                             |
| :-------- | :------------------------------- |
| PAL       | `rom/pal/Dark Cloud (PAL).iso`   |
| NTSC 1.02 | `rom/ntsc/Dark Cloud (NTSC).iso` |

### 3. Build the project

**Linux / macOS**

```bash
./build.sh <configuration>
```

**Windows**

```batch
build.bat <configuration>
```

### 4. Run the project

```bash
./run.sh <configuration> [arguments]
```

Native runs through `run.bat` or `run.sh` keep runtime files in the ignored
`port/run/` directory: extracted data in `data/`, saves and settings in `save/`,
and soundtrack files, icons and relative screenshot output alongside them.
Relative input and output arguments resolve from `port/run/`; use absolute paths
for files elsewhere. Build output stays in `port/build/`.

### Build Configurations

| Configuration / task  | Platform        | Build                               |
| :-------------------- | :-------------- | :---------------------------------- |
| `ps2-pal`             | PlayStation 2   | PAL executable and disc image       |
| `ps2-ntsc`            | PlayStation 2   | NTSC 1.02 executable and disc image |
| `linux-x64-release`   | Linux x64       | Release                             |
| `linux-x64`           | Linux x64       | Debug                               |
| `macos-arm64-release` | macOS ARM64     | Release                             |
| `macos-arm64`         | macOS ARM64     | Debug                               |
| `windows-x64-release` | Windows x64     | Release                             |
| `windows-x64`         | Windows x64     | Debug                               |
| `switch`              | Nintendo Switch | Homebrew `.nro` (see [`docs/SWITCH.md`](docs/SWITCH.md)) |
| `assembleDebug`       | Android         | Debug APK (experimental)            |
| `assembleProfile`     | Android         | Profile APK (experimental)          |

Run Android Gradle tasks from `port/android` with `gradlew.bat <task>` on
Windows; see the [Android build
documentation](https://github.com/TheMoonPeople/Chronicle/blob/master/port/android/README.md).

### Build Options

The root build scripts accept these environment variables:

| Variable  | Description                                                           |
| :-------- | :-------------------------------------------------------------------- |
| `CLEAN=1` | Removes the selected configuration's build directory before building. |
| `JOBS=N`  | Sets the number of parallel build jobs.                               |

For example:

```bash
CLEAN=1 JOBS=8 ./build.sh linux-x64-release
```

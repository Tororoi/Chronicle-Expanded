# Changelog

Every change from the retail Dark Cloud that a player of Chronicle-Expanded gets, grouped by kind of change.
Each section lists this repository's own changes first, then those that come from the upstream PC port,
[Chronicle](https://github.com/TheMoonPeople/Chronicle), under "From Chronicle".

## Platforms and packaging

### From Chronicle

- Native PC port for Linux, macOS (Apple Silicon) and Windows on SDL3 and Vulkan. TheMoonPeople/Chronicle#1, TheMoonPeople/Chronicle#44, TheMoonPeople/Chronicle#51
- Plays from your own PAL or NTSC disc image: the first start asks for it and extracts the game's files. TheMoonPeople/Chronicle#61
- Linux Flatpak package. adf65d80
- Game icons for every platform. TheMoonPeople/Chronicle#77
- Discord Rich Presence showing the character and the town or dungeon floor, with a toggle. TheMoonPeople/Chronicle#68, TheMoonPeople/Chronicle#69

## Display and graphics

### From Chronicle

- Any window size or resolution, windowed or fullscreen; the Resolution row lists every display mode. TheMoonPeople/Chronicle#67, TheMoonPeople/Chronicle#82
- Widescreen: the 3D view fills the window, the dungeon HUD and town clock sit in the window's corners, and a 4:3 letterbox setting is kept. e928c0ee, TheMoonPeople/Chronicle#145
- Full-screen 4:3 cutscene stills and full-size menu pictures stretch to the window's edges. TheMoonPeople/Chronicle#122, TheMoonPeople/Chronicle#126
- Interface Size setting scales the HUD and menus. e928c0ee
- The game runs at 60 Hz on PAL data too, without PAL's 50 Hz speed-up. TheMoonPeople/Chronicle#53
- Smooth motion at high frame rates, with V-Sync and Frame Limit settings. 733c40f6
- FPS counter (F3) in the game's font, with a setting for what it shows. TheMoonPeople/Chronicle#79, TheMoonPeople/Chronicle#84, TheMoonPeople/Chronicle#85, TheMoonPeople/Chronicle#132
- Anisotropic texture filtering. TheMoonPeople/Chronicle#118
- Depth of field, heat haze and water refraction are drawn at full resolution without retail's blocky blur; the heat haze ramps smoothly to the horizon. c49d92b8, TheMoonPeople/Chronicle#120
- Optional settings to draw distant town buildings, villagers and dungeon monsters in full detail and with their shadows, softened by the town's haze. e928c0ee, TheMoonPeople/Chronicle#176
- Georama mode draws town parts at walking's level of detail, and the part on the cursor at its finest. TheMoonPeople/Chronicle#173

## Controls

### From Chronicle

- Keyboard and mouse: mouse look in every camera, first-person walking with WASD, optional mouse-wheel zoom and an adjustable vertical camera return. TheMoonPeople/Chronicle#98, TheMoonPeople/Chronicle#99
- Bindings page in Options to rebind keys and mouse buttons. TheMoonPeople/Chronicle#125
- The mouse works as a pointer in the Options, Register Name, pause menu and save screens. TheMoonPeople/Chronicle#67, TheMoonPeople/Chronicle#115, TheMoonPeople/Chronicle#150
- Gamepad sticks scale like a DualShock 2's, with sensitivity and invert settings. TheMoonPeople/Chronicle#57
- Gyro camera: off, always, in first person, or while a button is held. TheMoonPeople/Chronicle#70, TheMoonPeople/Chronicle#71, TheMoonPeople/Chronicle#75
- DualSense: touchpad as the menu mouse, lightbar showing the active character's life, player light and rumble strength. TheMoonPeople/Chronicle#116
- DualSense and DualShock 4 touchpad click is Select. TheMoonPeople/Chronicle#123
- Steam Controller (2026) support. TheMoonPeople/Chronicle#43
- Button symbols for the pad in use (PS3, PS4, PS5 in colour, Xbox, Switch, Steam Deck, Steam Controller, keyboard), or the originals. TheMoonPeople/Chronicle#112, TheMoonPeople/Chronicle#128, TheMoonPeople/Chronicle#152

## Audio

### From Chronicle

- Mono, stereo or surround (stereo spread to 5.1), and a master volume. 5673e6e0, TheMoonPeople/Chronicle#136
- Custom soundtrack: map the game's tracks to your own WAV, FLAC or Ogg recordings in `soundtrack.json`. 5673e6e0

## Saves

### From Chronicle

- No memory cards: each save is a file in its own numbered folder, with no 12-save limit. TheMoonPeople/Chronicle#66
- Saves from a PCSX2 memory card export can be copied in by hand. TheMoonPeople/Chronicle#66
- The game's options are global settings, no longer stored in each save. TheMoonPeople/Chronicle#67

## Text and languages

### From Chronicle

- Sharp message text from a TrueType font (the Dark Cloud Compendium Community Font), with a Text Font setting to go back to the original bitmaps. TheMoonPeople/Chronicle#112, TheMoonPeople/Chronicle#143
- Game text can be replaced from per-language JSON files, exported from your own disc. TheMoonPeople/Chronicle#112
- Menu captions, area name cards, floor banners and boss names are redrawn as sharp text per language. TheMoonPeople/Chronicle#127, TheMoonPeople/Chronicle#174
- Text, Symbol, Area Name, Floor Label and Boss Name Shadow settings. TheMoonPeople/Chronicle#112, TheMoonPeople/Chronicle#127
- The language select offers only the languages the disc carries; NTSC plays in American English with no select. TheMoonPeople/Chronicle#97
- The dungeon floor number sits below the plate's "Floor" word instead of over it. TheMoonPeople/Chronicle#156

## Options

### From Chronicle

- New settings screen with Game, Display, Audio, Controls, Text, Accessibility and Bindings pages; changes apply at once and are kept for every save. TheMoonPeople/Chronicle#65, TheMoonPeople/Chronicle#67, TheMoonPeople/Chronicle#100, TheMoonPeople/Chronicle#130
- Always Win QTEs (Accessibility): button-prompt events still play but always end in a perfect. TheMoonPeople/Chronicle#96
- Quit Game action. TheMoonPeople/Chronicle#144, TheMoonPeople/Chronicle#155, TheMoonPeople/Chronicle#171

## Fixes

### From Chronicle

- Treasure chests weight items by the dungeon's own rate list, so the Flapping Duster, Goddess Ring and Aga's Sword can drop. TheMoonPeople/Chronicle#76
- Sixteen regular monsters that never dropped anything (Hornet, Ghost, Lich, Cave Bat, Gol, Sil and others) now drop money and items. TheMoonPeople/Chronicle#142
- Treant's chests are no longer lost by finishing Matataki's river before forest floor 8. TheMoonPeople/Chronicle#146, TheMoonPeople/Chronicle#153
- The Broken Dagger glitch in the item shop is gone. TheMoonPeople/Chronicle#158
- A bomb's blast throws the player away from it, not always the same way. TheMoonPeople/Chronicle#181
- The HUD's weapon icon keeps its colours after closing a menu on the PAL disc in English. TheMoonPeople/Chronicle#91

## Game mechanics

- A hit on a frozen player breaks the freeze. #4

### From Chronicle

- Element Quick Select (off by default): D-pad Up in a dungeon picks the equipped weapon's element. TheMoonPeople/Chronicle#111

## Weapons

- The Heal ability heals every 3 seconds instead of 4. #3
- Ungaga's charge shots wear his weapon only when fired, not again for every hit. #5
- New Confuse ability on Babel's Spear (and its SynthSphere): 5% of hits confuse a non-boss monster for 20 seconds, turning it on other monsters. #6

## Enemies

- Enemy poison lasts 30 seconds, and each tick's damage scales with the monster's resistance. #7

## Towns

- Play as any ally in town: commit them in the party menu with the dungeon's change sequence, and they walk with full animations. #9, #10, #11, #12, #13
- Each ally takes ladders their own way: Xiao leaps, Goro hops, Ruby floats, Osmond flies and Ungaga refuses. #14

## Fishing

### From Chronicle

- Fish sizes taper smoothly into each species' maximum instead of piling up at it. TheMoonPeople/Chronicle#141

## Developer tools

### From Chronicle

- Developer/boot menu, debug mode from `config.json` or a pad chord, and debug shortcuts under Select+L2. TheMoonPeople/Chronicle#119, dda4b7f0
- Items and weapons given at start with `--give`. TheMoonPeople/Chronicle#117
- Text wipe (F9) compares the original picture text with the new runtime text. TheMoonPeople/Chronicle#174

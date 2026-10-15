# Changelog

Every change from the retail Dark Cloud that a player of Chronicle-Expanded gets, grouped by kind of change.
Each section lists this repository's own changes first, then those that come from the upstream PC port,
[Chronicle](https://github.com/TheMoonPeople/Chronicle), under "From Chronicle".

## Platforms and packaging

### From Chronicle

- Native PC port for Linux, macOS (Apple Silicon) and Windows on SDL3 and Vulkan. [Chronicle#1](https://github.com/TheMoonPeople/Chronicle/pull/1), [Chronicle#44](https://github.com/TheMoonPeople/Chronicle/pull/44), [Chronicle#51](https://github.com/TheMoonPeople/Chronicle/pull/51)
- Plays from your own PAL or NTSC disc image: the first start asks for it and extracts the game's files. [Chronicle#61](https://github.com/TheMoonPeople/Chronicle/pull/61)
- Linux Flatpak package. [adf65d80](https://github.com/TheMoonPeople/Chronicle/commit/adf65d80)
- Game icons for every platform. [Chronicle#77](https://github.com/TheMoonPeople/Chronicle/pull/77)
- Discord Rich Presence showing the character and the town or dungeon floor, with a toggle. [Chronicle#68](https://github.com/TheMoonPeople/Chronicle/pull/68), [Chronicle#69](https://github.com/TheMoonPeople/Chronicle/pull/69)

## Display and graphics

### From Chronicle

- Any window size or resolution, windowed or fullscreen; the Resolution row lists every display mode. [Chronicle#67](https://github.com/TheMoonPeople/Chronicle/pull/67), [Chronicle#82](https://github.com/TheMoonPeople/Chronicle/pull/82)
- Widescreen: the 3D view fills the window, the dungeon HUD and town clock sit in the window's corners, and a 4:3 letterbox setting is kept. [e928c0ee](https://github.com/TheMoonPeople/Chronicle/commit/e928c0ee), [Chronicle#145](https://github.com/TheMoonPeople/Chronicle/pull/145)
- Full-screen 4:3 cutscene stills and full-size menu pictures stretch to the window's edges. [Chronicle#122](https://github.com/TheMoonPeople/Chronicle/pull/122), [Chronicle#126](https://github.com/TheMoonPeople/Chronicle/pull/126)
- Interface Size setting scales the HUD and menus. [e928c0ee](https://github.com/TheMoonPeople/Chronicle/commit/e928c0ee)
- The game runs at 60 Hz on PAL data too, without PAL's 50 Hz speed-up. [Chronicle#53](https://github.com/TheMoonPeople/Chronicle/pull/53)
- Smooth motion at high frame rates, with V-Sync and Frame Limit settings. [733c40f6](https://github.com/TheMoonPeople/Chronicle/commit/733c40f6)
- FPS counter (F3) in the game's font, with a setting for what it shows. [Chronicle#79](https://github.com/TheMoonPeople/Chronicle/pull/79), [Chronicle#84](https://github.com/TheMoonPeople/Chronicle/pull/84), [Chronicle#85](https://github.com/TheMoonPeople/Chronicle/pull/85), [Chronicle#132](https://github.com/TheMoonPeople/Chronicle/pull/132)
- Anisotropic texture filtering. [Chronicle#118](https://github.com/TheMoonPeople/Chronicle/pull/118)
- Depth of field, heat haze and water refraction are drawn at full resolution without retail's blocky blur; the heat haze ramps smoothly to the horizon. [c49d92b8](https://github.com/TheMoonPeople/Chronicle/commit/c49d92b8), [Chronicle#120](https://github.com/TheMoonPeople/Chronicle/pull/120)
- Optional settings to draw distant town buildings, villagers and dungeon monsters in full detail and with their shadows, softened by the town's haze. [e928c0ee](https://github.com/TheMoonPeople/Chronicle/commit/e928c0ee), [Chronicle#176](https://github.com/TheMoonPeople/Chronicle/pull/176)
- Georama mode draws town parts at walking's level of detail, and the part on the cursor at its finest. [Chronicle#173](https://github.com/TheMoonPeople/Chronicle/pull/173)

## Controls

### From Chronicle

- Keyboard and mouse: mouse look in every camera, first-person walking with WASD, optional mouse-wheel zoom and an adjustable vertical camera return. [Chronicle#98](https://github.com/TheMoonPeople/Chronicle/pull/98), [Chronicle#99](https://github.com/TheMoonPeople/Chronicle/pull/99)
- Bindings page in Options to rebind keys and mouse buttons. [Chronicle#125](https://github.com/TheMoonPeople/Chronicle/pull/125)
- The mouse works as a pointer in the Options, Register Name, pause menu and save screens. [Chronicle#67](https://github.com/TheMoonPeople/Chronicle/pull/67), [Chronicle#115](https://github.com/TheMoonPeople/Chronicle/pull/115), [Chronicle#150](https://github.com/TheMoonPeople/Chronicle/pull/150)
- Gamepad sticks scale like a DualShock 2's, with sensitivity and invert settings. [Chronicle#57](https://github.com/TheMoonPeople/Chronicle/pull/57)
- Gyro camera: off, always, in first person, or while a button is held. [Chronicle#70](https://github.com/TheMoonPeople/Chronicle/pull/70), [Chronicle#71](https://github.com/TheMoonPeople/Chronicle/pull/71), [Chronicle#75](https://github.com/TheMoonPeople/Chronicle/pull/75)
- DualSense: touchpad as the menu mouse, lightbar showing the active character's life, player light and rumble strength. [Chronicle#116](https://github.com/TheMoonPeople/Chronicle/pull/116)
- DualSense and DualShock 4 touchpad click is Select. [Chronicle#123](https://github.com/TheMoonPeople/Chronicle/pull/123)
- Steam Controller (2026) support. [Chronicle#43](https://github.com/TheMoonPeople/Chronicle/pull/43)
- Button symbols for the pad in use (PS3, PS4, PS5 in colour, Xbox, Switch, Steam Deck, Steam Controller, keyboard), or the originals. [Chronicle#112](https://github.com/TheMoonPeople/Chronicle/pull/112), [Chronicle#128](https://github.com/TheMoonPeople/Chronicle/pull/128), [Chronicle#152](https://github.com/TheMoonPeople/Chronicle/pull/152)

## Audio

### From Chronicle

- Mono, stereo or surround (stereo spread to 5.1), and a master volume. [5673e6e0](https://github.com/TheMoonPeople/Chronicle/commit/5673e6e0), [Chronicle#136](https://github.com/TheMoonPeople/Chronicle/pull/136)
- Custom soundtrack: map the game's tracks to your own WAV, FLAC or Ogg recordings in `soundtrack.json`. [5673e6e0](https://github.com/TheMoonPeople/Chronicle/commit/5673e6e0)

## Saves

### From Chronicle

- No memory cards: each save is a file in its own numbered folder, with no 12-save limit. [Chronicle#66](https://github.com/TheMoonPeople/Chronicle/pull/66)
- Saves from a PCSX2 memory card export can be copied in by hand. [Chronicle#66](https://github.com/TheMoonPeople/Chronicle/pull/66)
- The game's options are global settings, no longer stored in each save. [Chronicle#67](https://github.com/TheMoonPeople/Chronicle/pull/67)

## Text and languages

### From Chronicle

- Sharp message text from a TrueType font (the Dark Cloud Compendium Community Font), with a Text Font setting to go back to the original bitmaps. [Chronicle#112](https://github.com/TheMoonPeople/Chronicle/pull/112), [Chronicle#143](https://github.com/TheMoonPeople/Chronicle/pull/143)
- Game text can be replaced from per-language JSON files, exported from your own disc. [Chronicle#112](https://github.com/TheMoonPeople/Chronicle/pull/112)
- Menu captions, area name cards, floor banners and boss names are redrawn as sharp text per language. [Chronicle#127](https://github.com/TheMoonPeople/Chronicle/pull/127), [Chronicle#174](https://github.com/TheMoonPeople/Chronicle/pull/174)
- Text, Symbol, Area Name, Floor Label and Boss Name Shadow settings. [Chronicle#112](https://github.com/TheMoonPeople/Chronicle/pull/112), [Chronicle#127](https://github.com/TheMoonPeople/Chronicle/pull/127)
- The language select offers only the languages the disc carries; NTSC plays in American English with no select. [Chronicle#97](https://github.com/TheMoonPeople/Chronicle/pull/97)
- The dungeon floor number sits below the plate's "Floor" word instead of over it. [Chronicle#156](https://github.com/TheMoonPeople/Chronicle/pull/156)

## Options

### From Chronicle

- New settings screen with Game, Display, Audio, Controls, Text, Accessibility and Bindings pages; changes apply at once and are kept for every save. [Chronicle#65](https://github.com/TheMoonPeople/Chronicle/pull/65), [Chronicle#67](https://github.com/TheMoonPeople/Chronicle/pull/67), [Chronicle#100](https://github.com/TheMoonPeople/Chronicle/pull/100), [Chronicle#130](https://github.com/TheMoonPeople/Chronicle/pull/130)
- Always Win QTEs (Accessibility): button-prompt events still play but always end in a perfect. [Chronicle#96](https://github.com/TheMoonPeople/Chronicle/pull/96)
- Quit Game action. [Chronicle#144](https://github.com/TheMoonPeople/Chronicle/pull/144), [Chronicle#155](https://github.com/TheMoonPeople/Chronicle/pull/155), [Chronicle#171](https://github.com/TheMoonPeople/Chronicle/pull/171)

## Fixes

### From Chronicle

- Treasure chests weight items by the dungeon's own rate list, so the Flapping Duster, Goddess Ring and Aga's Sword can drop. [Chronicle#76](https://github.com/TheMoonPeople/Chronicle/pull/76)
- Sixteen regular monsters that never dropped anything (Hornet, Ghost, Lich, Cave Bat, Gol, Sil and others) now drop money and items. [Chronicle#142](https://github.com/TheMoonPeople/Chronicle/pull/142)
- Treant's chests are no longer lost by finishing Matataki's river before forest floor 8. [Chronicle#146](https://github.com/TheMoonPeople/Chronicle/pull/146), [Chronicle#153](https://github.com/TheMoonPeople/Chronicle/pull/153)
- The Broken Dagger glitch in the item shop is gone. [Chronicle#158](https://github.com/TheMoonPeople/Chronicle/pull/158)
- A bomb's blast throws the player away from it, not always the same way. [Chronicle#181](https://github.com/TheMoonPeople/Chronicle/pull/181)
- The HUD's weapon icon keeps its colours after closing a menu on the PAL disc in English. [Chronicle#91](https://github.com/TheMoonPeople/Chronicle/pull/91)

## Game mechanics

- A hit on a frozen player breaks the freeze. [#4](https://github.com/Tororoi/Chronicle-Expanded/pull/4)

### From Chronicle

- Element Quick Select (off by default): D-pad Up in a dungeon picks the equipped weapon's element. [Chronicle#111](https://github.com/TheMoonPeople/Chronicle/pull/111)

## Weapons

- The Heal ability heals every 3 seconds instead of 4. [#3](https://github.com/Tororoi/Chronicle-Expanded/pull/3)
- Ungaga's charge shots wear his weapon only when fired, not again for every hit. [#5](https://github.com/Tororoi/Chronicle-Expanded/pull/5)
- New Confuse ability: 5% of hits confuse a non-boss monster for 20 seconds, turning it on other monsters. A weapon's SynthSphere passes it on. [#6](https://github.com/Tororoi/Chronicle-Expanded/pull/6)

### Weapon stats and abilities

| Weapon | Change | PR |
|---|---|---|
| Evilcise | Always has Poor | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Macho Sword | Always has Abs Up | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Heaven's Cloud | Max Attack 102 → 180, max Magic 85 → 180, attachment slots 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Lamb's Sword | Attachment slots 2 → 3; transforms and gains its power boost at 50% WHP instead of 20% | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Brave Ark | Attachment slots 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Sand Breaker | Attachment slots 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Choora | Attachment slots 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| 7 Branch Sword | WHP 30 → 47, Endurance 40 → 47, Magic 35 → 37; Dino Slayer, Undead Buster, Sea Killer, Stone Breaker, Plant Buster, Sky Hunter and Mimic Breaker 0 → 7, Beast Buster and Mage Slayer 0 → 8, Metal Breaker 0 → 10 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Chronicle 2 | Max Attack 350 → 999 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Bone Slingshot | 50% chance of Fragile | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Hardshooter | 50% chance of Fragile | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Matador | Always has Critical | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Frozen Tuna | Always has Stop; max Attack 56 → 100, max Magic 30 → 678, attachment slots 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142), [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Platinum Ring | Ice 20 → 0, Holy 0 → 20 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Crystal Ring | Holy 20 → 0, Ice 0 → 20 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Pocklekul | Attack 35 → 28, Magic 55 → 28, Holy 20 → 0 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Thorn Armlet | Stone Breaker 0 → 20, Beast Buster 0 → 20, max Attack 60 → 70, max Magic 46 → 65 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Destruction Ring | Attack 50 → 60, Metal Breaker 30 → 40, max Magic 99 → 140 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Goddess Ring | Always has Heal | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Athena's Armlet | Always has Abs Up | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| DeSanga | Always has Drain | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Babel's Spear | Has Confuse; attachment slots 3 → 4 | [#6](https://github.com/Tororoi/Chronicle-Expanded/pull/6), [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Blessing Gun | Max Attack 56 → 87, max Magic 30 → 80 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Skunk | Max Attack 72 → 143, max Magic 55 → 105 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |

### Buildup paths

Changes to which weapons build up into which, [#144](https://github.com/Tororoi/Chronicle-Expanded/pull/144). ★ marks a final form.

#### Toan

> Heaven's Cloud is a final form (it built up into Dark Cloud). Choora builds up only into Maneater (no longer into Heaven's Cloud).

```
Baselard
├─ Shamshir → Dusack
│  ├─ Brave Ark → Dark Cloud → 7th Heaven ★
│  └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★
└─ Sax → Dusack
   ├─ Brave Ark → Dark Cloud → 7th Heaven ★
   └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Gladius
├─ Small Sword → Tsukikage → Heaven's Cloud ★
└─ Chopper
   ├─ Choora → Maneater → Atlamillia Sword → Chronicle Sword ★
   └─ Dusack
      ├─ Brave Ark → Dark Cloud → 7th Heaven ★
      └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Wise Owl Sword → Lamb's Sword → Atlamillia Sword → Chronicle Sword ★

Crysknife
├─ Small Sword → Tsukikage → Heaven's Cloud ★
└─ Sand Breaker → Antique Sword → Brave Ark → Dark Cloud → 7th Heaven ★

Buster Sword → Claymore → Cross Hinder → Big Bang → Sword of Zeus ★

Kitchen Knife
├─ Chopper
│  ├─ Choora → Maneater → Atlamillia Sword → Chronicle Sword ★
│  └─ Dusack
│     ├─ Brave Ark → Dark Cloud → 7th Heaven ★
│     └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★
└─ Sax → Dusack
   ├─ Brave Ark → Dark Cloud → 7th Heaven ★
   └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Sun Sword → Big Bang → Sword of Zeus ★

Serpent Sword
├─ Tsukikage → Heaven's Cloud ★
└─ Evilcise → Drain Seeker → Dark Cloud → 7th Heaven ★

Macho Sword
├─ Aga's Sword → Atlamillia Sword → Chronicle Sword ★
└─ Cross Hinder → Big Bang → Sword of Zeus ★

Mardan Eins → Mardan Twei → Arise Mardan ★

Bone Rapier → Evilcise → Drain Seeker → Dark Cloud → 7th Heaven ★
```

#### Xiao

> Hardshooter builds up only into Double Impact (no longer into Matador). Double Impact builds up into Matador instead of Divine Beast Title.

```
Steel Slingshot → Hardshooter → Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★

Bandit Slingshot
├─ Hardshooter → Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★
└─ Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★

Steve → Super Steve ★

Bone Slingshot → Flamingo → Dragon's Y → Divine Beast Title → Angel Shooter → Angel Gear ★
```

#### Goro

> Big Bucks Hammer builds up only into Magical Hammer (no longer into Gaia Hammer). Frozen Tuna is a final form (it built up into Turtle Shell).

```
Steel Hammer → Plate Hammer → Magical Hammer
├─ Gaia Hammer → Tall Hammer ★
└─ Last Judgement → Tall Hammer ★

Turtle Shell
├─ Magical Hammer
│  ├─ Gaia Hammer → Tall Hammer ★
│  └─ Last Judgement → Tall Hammer ★
└─ Battle Ax → Satan's Ax → Inferno ★

Big Bucks Hammer → Magical Hammer
├─ Gaia Hammer → Tall Hammer ★
└─ Last Judgement → Tall Hammer ★

Trial Hammer → Gaia Hammer → Tall Hammer ★

Frozen Tuna ★
```

#### Ruby

> Platinum Ring builds up into Crystal Ring or Thorn Armlet (was Crystal Ring or Fairy's Ring). Thorn Armlet builds up into Destruction Ring (was Platinum Ring). Pocklekul builds up into Fairy's Ring, Goddess Ring or Satan's Ring (was Fairy's Ring only).

```
Bandit's Ring
├─ Crystal Ring
│  ├─ Goddess Ring → Athena's Armlet → Secret Armlet ★
│  └─ Satan's Ring → Mobius Ring ★
└─ Pocklekul
   ├─ Fairy's Ring → Destruction Ring → Mobius Ring ★
   ├─ Goddess Ring → Athena's Armlet → Secret Armlet ★
   └─ Satan's Ring → Mobius Ring ★

Platinum Ring
├─ Crystal Ring
│  ├─ Goddess Ring → Athena's Armlet → Secret Armlet ★
│  └─ Satan's Ring → Mobius Ring ★
└─ Thorn Armlet → Destruction Ring → Mobius Ring ★
```

#### Ungaga

> No changes.

```
Javelin
├─ DeSanga → Cactus → Terra Sword → Babel's Spear ★
└─ Partisan → Cactus → Terra Sword → Babel's Spear ★

Halberd → Scorpion
├─ Mirage
│  ├─ Terra Sword → Babel's Spear ★
│  └─ Hercules' Wrath ★
└─ Cactus → Terra Sword → Babel's Spear ★

5 Foot Nail
├─ Scorpion
│  ├─ Mirage
│  │  ├─ Terra Sword → Babel's Spear ★
│  │  └─ Hercules' Wrath ★
│  └─ Cactus → Terra Sword → Babel's Spear ★
└─ Partisan → Cactus → Terra Sword → Babel's Spear ★
```

#### Osmond

> Jackal and Snail no longer build up into Blessing Gun. Skunk is a final form (it built up into Hexa Blaster).

```
Jackal → Swallow → G Crusher → Star Breaker ★

Blessing Gun → Skunk ★

Snail → Hexa Blaster → Supernova ★
```

## Enemies

- Enemy poison lasts 30 seconds, and each tick's damage scales with the monster's resistance. [#7](https://github.com/Tororoi/Chronicle-Expanded/pull/7)

## Towns

- Play as any ally in town: commit them in the party menu with the dungeon's change sequence, and they walk with full animations. [#9](https://github.com/Tororoi/Chronicle-Expanded/pull/9), [#10](https://github.com/Tororoi/Chronicle-Expanded/pull/10), [#11](https://github.com/Tororoi/Chronicle-Expanded/pull/11), [#12](https://github.com/Tororoi/Chronicle-Expanded/pull/12), [#13](https://github.com/Tororoi/Chronicle-Expanded/pull/13)
- Each ally takes ladders their own way: Xiao leaps, Goro hops, Ruby floats, Osmond flies and Ungaga refuses. [#14](https://github.com/Tororoi/Chronicle-Expanded/pull/14)

## Fishing

### From Chronicle

- Fish sizes taper smoothly into each species' maximum instead of piling up at it. [Chronicle#141](https://github.com/TheMoonPeople/Chronicle/pull/141)

## Developer tools

### From Chronicle

- Developer/boot menu, debug mode from `config.json` or a pad chord, and debug shortcuts under Select+L2. [Chronicle#119](https://github.com/TheMoonPeople/Chronicle/pull/119), [dda4b7f0](https://github.com/TheMoonPeople/Chronicle/commit/dda4b7f0)
- Items and weapons given at start with `--give`. [Chronicle#117](https://github.com/TheMoonPeople/Chronicle/pull/117)
- Text wipe (F9) compares the original picture text with the new runtime text. [Chronicle#174](https://github.com/TheMoonPeople/Chronicle/pull/174)

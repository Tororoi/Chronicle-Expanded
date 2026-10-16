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
- Fruit of Eden can be sold and thrown away. [#PR](https://github.com/Tororoi/Chronicle-Expanded/pull/PR)

### From Chronicle

- Element Quick Select (off by default): D-pad Up in a dungeon picks the equipped weapon's element. [Chronicle#111](https://github.com/TheMoonPeople/Chronicle/pull/111)

## Weapons

- The Heal ability heals every 3 seconds instead of 4. [#3](https://github.com/Tororoi/Chronicle-Expanded/pull/3)
- Ungaga's charge shots wear his weapon only when fired, not again for every hit. [#5](https://github.com/Tororoi/Chronicle-Expanded/pull/5)
- New Confuse ability: 5% of hits confuse a non-boss monster for 20 seconds, turning it on other monsters. A weapon's SynthSphere passes it on. [#6](https://github.com/Tororoi/Chronicle-Expanded/pull/6)
- Super Steve shows the weapon icon of the SynthSphere attached to it on the dungeon screen, and a slingshot's sphere makes it fire that slingshot's pellets. [#145](https://github.com/Tororoi/Chronicle-Expanded/pull/145)
- Baselard "Heavy Hand": every hit throws the monster back about 17 units, and so does every pellet of Super Steve carrying the Baselard's SynthSphere. [#146](https://github.com/Tororoi/Chronicle-Expanded/pull/146)

### Weapon stats and abilities

| Weapon | Ability | WHP | AT | ED | SP | MP | Elements | Anti | Slots | PR |
|---|---|---|---|---|---|---|---|---|---|---|
| Evilcise | Poor (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Macho Sword | Abs Up (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Heaven's Cloud |  |  | max 102 → 180 |  |  | max 85 → 180 |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Lamb's Sword¹ |  |  |  |  |  |  |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Brave Ark |  |  |  |  |  |  |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Sand Breaker |  |  |  |  |  |  |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Choora |  |  |  |  |  |  |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| 7 Branch Sword |  | 30 → 47 | 65 → 67 | 40 → 47 | 80 → 77 | 35 → 37 | All 20 → 17 | All 0 → 7 |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142), [#147](https://github.com/Tororoi/Chronicle-Expanded/pull/147) |
| Chronicle 2 |  |  | max 350 → 999 |  |  |  |  |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Bone Slingshot | Fragile (50% chance) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Hardshooter | Fragile (50% chance) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Matador | Critical (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Frozen Tuna | Stop (always) |  | max 56 → 100 |  |  | max 30 → 678 |  |  | 2 → 3 | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142), [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Platinum Ring |  |  |  |  |  |  | Ice 20 → 0, Holy 0 → 20 |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Crystal Ring |  |  |  |  |  |  | Holy 20 → 0, Ice 0 → 20 |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Pocklekul |  |  | 35 → 28 |  |  | 55 → 28 |  |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142), [#147](https://github.com/Tororoi/Chronicle-Expanded/pull/147) |
| Thorn Armlet |  |  | max 60 → 70 |  |  | max 46 → 65 |  | Stone Breaker 0 → 20, Beast Buster 0 → 20 |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Destruction Ring |  |  | 50 → 60 |  |  | max 99 → 140 |  | Metal Breaker 30 → 40 |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Goddess Ring | Heal (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Athena's Armlet | Abs Up (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| DeSanga | Drain (always) |  |  |  |  |  |  |  |  | [#143](https://github.com/Tororoi/Chronicle-Expanded/pull/143) |
| Babel's Spear | Confuse |  |  |  |  |  |  |  | 3 → 4 | [#6](https://github.com/Tororoi/Chronicle-Expanded/pull/6), [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Blessing Gun |  |  | max 56 → 87 |  |  | max 30 → 80 |  |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |
| Skunk |  |  | max 72 → 143 |  |  | max 55 → 105 |  |  |  | [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142) |

¹ Lamb's Sword also transforms and gains its power boost at 50% WHP instead of 20%. [#142](https://github.com/Tororoi/Chronicle-Expanded/pull/142)

### Buildup paths

Changes to which weapons build up into which, [#144](https://github.com/Tororoi/Chronicle-Expanded/pull/144). ★ marks a final form.

#### Toan

> **Mod changes:** Choora builds up to Maneater only (Heaven's Cloud path removed). Heaven's Cloud is a terminal weapon (Dark Cloud path removed).

```
Baselard
  ├─ Sax ─────┐
  └─ Shamshir ─┴─ Dusack ─┬─ Brave Ark → Dark Cloud → 7th Heaven ★
                           └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Gladius
  ├─ Small Sword → Tsukikage → Heaven's Cloud ★
  └─ Chopper ─┬─ Choora → Maneater → Atlamillia Sword → Chronicle Sword ★
               └─ Dusack ─┬─ Brave Ark → Dark Cloud → 7th Heaven ★
                           └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Crysknife
  ├─ Small Sword → Tsukikage → Heaven's Cloud ★
  └─ Sandbreaker → Antique Sword → Brave Ark → Dark Cloud → 7th Heaven ★

Buster Sword → Claymore → Cross Hinder → Big Bang → Sword of Zeus ★

Wise Owl Sword → Lamb's Sword → Atlamillia Sword → Chronicle Sword ★

Bone Rapier → Evilcise → Drainseeker → Dark Cloud → 7th Heaven ★

Kitchen Knife
  ├─ Sax ─────────────────┐
  └─ Chopper ─┬─ Choora → Maneater → Atlamillia Sword → Chronicle Sword ★
               └──────────┴─ Dusack ─┬─ Brave Ark → Dark Cloud → 7th Heaven ★
                                      └─ 7 Branch Sword → Atlamillia Sword → Chronicle Sword ★

Sun Sword → Big Bang → Sword of Zeus ★

Macho Sword
  ├─ Aga's Sword → Atlamillia Sword → Chronicle Sword ★
  └─ Cross Hinder → Big Bang → Sword of Zeus ★

Serpent Sword
  ├─ Tsukikage → Heaven's Cloud ★
  └─ Evilcise → Drainseeker → Dark Cloud → 7th Heaven ★

Mardan Eins → Mardan Twei → Arise Mardan ★

Chronicle 2 ★
```

#### Xiao

> **Mod changes:** Hardshooter builds up to Double Impact only (Matador path removed). Double Impact builds up to Matador only (was Divine Beast Title), so every Steel and Bandit line runs through Matador before Divine Beast Title.

```
Steel Slingshot → Hardshooter → Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★

Bone Slingshot → Flamingo → Dragon's Y → Divine Beast Title → Angel Shooter → Angel Gear ★

Bandit Slingshot ─┬─ Hardshooter → Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★
                  └─ Double Impact → Matador → Divine Beast Title → Angel Shooter → Angel Gear ★

Steve → Super Steve ★
```

#### Goro

> **Mod changes:** Big Bucks Hammer builds up to Magical Hammer only (direct Gaia Hammer path removed). Frozen Tuna is a terminal weapon (Turtle Shell path removed).

```
Steel Hammer → Plate Hammer → Magical Hammer ─┬─ Gaia Hammer ─┐
                                               └─ Last Judgement ┴─ Tall Hammer ★

Trial Hammer → Gaia Hammer → Tall Hammer ★

Big Bucks Hammer → Magical Hammer ─┬─ Gaia Hammer ─┐
                                    └─ Last Judgement ┴─ Tall Hammer ★

Turtle Shell ─┬─ Magical Hammer ─┬─ Gaia Hammer ─┐
              │                   └─ Last Judgement ┴─ Tall Hammer ★
              └─ Battle Axe → Satan's Axe → Inferno ★

Battle Axe → Satan's Axe → Inferno ★

Frozen Tuna ★
```

#### Ruby

> **Mod changes:** Platinum Ring builds up to Crystal Ring or Thorn Armlet (was Crystal Ring or Fairy Ring). Thorn Armlet builds up to Destruction Ring only (was Platinum Ring). Pocklekul builds up to Fairy Ring, Goddess Ring or Satan's Ring (was Fairy Ring).

```
Platinum Ring ─┬─ Crystal Ring ─┬─ Goddess Ring → Athena's Armlet → Secret Armlet ★
               │                └─ Satan's Ring → Mobius Ring ★
               └─ Thorn Armlet → Destruction Ring → Mobius Ring ★

Bandit's Ring ─┬─ Crystal Ring ─┬─ Goddess Ring → Athena's Armlet → Secret Armlet ★
               │                └─ Satan's Ring → Mobius Ring ★
               └─ Pocklekul ─┬─ Fairy Ring → Destruction Ring → Mobius Ring ★
                             ├─ Goddess Ring → Athena's Armlet → Secret Armlet ★
                             └─ Satan's Ring → Mobius Ring ★
```

#### Ungaga

```
Javelin ─┬─ Desanga ─┐
          └─ Partisan ┴─ Cactus → Terra Sword → Babel Spear ★

Halberd → Scorpion ─┬─ Mirage ─┬─ Terra Sword → Babel Spear ★
                    │           └─ Hercules' Wrath ★
                    └─ Cactus → Terra Sword → Babel Spear ★

5 Foot Nail ─┬─ Scorpion ─┬─ Mirage ─┬─ Terra Sword → Babel Spear ★
             │             │           └─ Hercules' Wrath ★
             │             └─ Cactus → Terra Sword → Babel Spear ★
             └─ Partisan → Cactus → Terra Sword → Babel Spear ★
```

#### Osmond

> **Mod changes:** Skunk is a terminal weapon (Hexa Blaster path removed). Jackal and Snail no longer build up to Blessing Gun; Blessing Gun is a standalone starting weapon.

```
Jackal → Swallow → G Crusher → Star Breaker ★

Snail → Hexa Blaster → Supernova ★

Blessing Gun → Skunk ★
```

## Enemies

- A monster met outside its home dungeon (or Demon Shaft band) fits in with the monsters there: one from earlier in the game has its life, ABS, defense and damage raised to their average, one from later has them lowered to the strongest met so far, and look-alikes such as Cave Bat and Evil Bat keep their order. Bosses and mimics keep theirs. [#149](https://github.com/Tororoi/Chronicle-Expanded/pull/149)
- Stronger Enemies option (Options, Expanded page): every monster has the life, defense and damage of a region deeper, past the last Demon Shaft floors too, and is never weakened; a mimic is at least as strong as the next one. ABS rewards stay as they were. [#150](https://github.com/Tororoi/Chronicle-Expanded/pull/150)
- Enemy poison lasts 30 seconds, and each tick's damage scales with the monster's resistance. [#7](https://github.com/Tororoi/Chronicle-Expanded/pull/7)
- New Faster Enemies option (Options, Expanded page): monsters move and act twice as fast. Each attack still slows to its normal speed for the moment it can hit, so its blows land as before. [#151](https://github.com/Tororoi/Chronicle-Expanded/pull/151)
- New Harder Enemy AI option (Options, Expanded page): a defeated monster that has a get-up animation, such as a Skeleton Soldier, the Days of the Week or the card soldiers, gets back up about 30% of the time with part of its life, and the undead that already could (Mummy, Master Jacket, Gacious, Horn Head, Silver Gear) do so about 45% of the time. [#152](https://github.com/Tororoi/Chronicle-Expanded/pull/152)
- Minibosses: most floors, back floors included, have a miniboss, more likely one of the kind of monster there are the most of; a second one is rare, and never of the same kind. A miniboss is half again as big, with three times the life and ABS and four times the money, half again the defense and damage (shots too), and a longer reach. It spots you and attacks from farther away, takes a tenth of the damage of thrown items, and always drops something. One you leave alive is still there when you come back from the back floor, or go back down to it; one you defeat stays gone. [#153](https://github.com/Tororoi/Chronicle-Expanded/pull/153)

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

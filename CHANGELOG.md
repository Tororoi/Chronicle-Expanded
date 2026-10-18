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
- Fruit of Eden can be sold and thrown away. [#161](https://github.com/Tororoi/Chronicle-Expanded/pull/161)

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
- New Randomize Enemies option (Options, Expanded page): each time you enter a dungeon, every floor but the boss and event floors gets its monster kinds drawn from the whole game, front and back floor alike: usually five to nine kinds, now and then only a few, often with the dungeon's own mimic and king mimic on top. A floor keeps its monsters until you leave the dungeon. [#155](https://github.com/Tororoi/Chronicle-Expanded/pull/155)
- With Randomize Enemies on, half the floors are a themed group instead of a random mix: cards, days of the week, dragons, pirates, undead, an element, a Demon Shaft band and more (a band most often in Demon Shaft itself). The group either fills the floor, or each of its monsters appears once alongside a varying number of other kinds: the dungeon's mimics alone, or its mimics and usually several of its own monsters. [#158](https://github.com/Tororoi/Chronicle-Expanded/pull/158)
- Monsters met outside their home dungeon on a randomized floor cry, attack and die with their own sounds instead of in silence. [#159](https://github.com/Tororoi/Chronicle-Expanded/pull/159)
- Minotaur Joe can appear on regular floors: there he fights where the floor puts him instead of on his arena, with his own sounds but no crowd cheering, and when beaten he roars, staggers, falls over and fades away, giving his ABS like any monster, instead of starting his defeat scene. [#157](https://github.com/Tororoi/Chronicle-Expanded/pull/157)
- Master Utan can appear on regular floors: there he fights on the floor's ground where it puts him, instead of up on his arena, with his own sounds, and when beaten he falls and fades away, giving his ABS like any monster, instead of starting his defeat scene. [#162](https://github.com/Tororoi/Chronicle-Expanded/pull/162)
- The Ice Queen can appear on regular floors once Ruby is in the party: there she fights where the floor puts her, with her barrier and her own sounds, and her ice arrows (now slower and freezing only sometimes; the Anti-Freeze Amulet protects you), ice prison, meteors, blasts and tornadoes no longer take up the floor's monster places. Breaking her barrier no longer counts as a kill there, and when beaten she falls and fades away, giving her ABS like any monster, instead of starting her defeat scene. [#163](https://github.com/Tororoi/Chronicle-Expanded/pull/163)
- King's Curse can appear on regular floors once Ungaga is in the party: there he fights where the floor puts him, in his black smoke and with his own sounds, and when his coffin is beaten it falls and fades away, giving its ABS like any monster, instead of starting his defeat scene. [#164](https://github.com/Tororoi/Chronicle-Expanded/pull/164)
- The Black Knight can appear on regular floors, on his mount or on foot: there the mount's fall still leaves him to fight on, and when beaten he bows his head and screams until a light comes down on him and he fades away, giving his ABS like any monster, instead of starting his defeat scene. [#165](https://github.com/Tororoi/Chronicle-Expanded/pull/165)
- Dran can appear on regular floors: there he flies the floor's largest room as his arena, with his own sounds, and when beaten he falls out of the air and fades away, giving his ABS like any monster, instead of starting his defeat scene. [#166](https://github.com/Tororoi/Chronicle-Expanded/pull/166)
- Bosses on regular floors find their way round walls to you instead of walking into them, keeping their large bodies clear of the walls. [#166](https://github.com/Tororoi/Chronicle-Expanded/pull/166)
- Bosses on regular floors are as strong as the dungeon they turn up in: their life, ABS and damage are set for each dungeon and each 20 floors of Demon Shaft, and the Black Knight gives 110 ABS. [#167](https://github.com/Tororoi/Chronicle-Expanded/pull/167)
- A Randomizer Bosses option (Off, Defeated or Any) lets randomized floors bring a boss with its own group of monsters and the dungeon's mimics, for the bosses you have beaten or for any. [#167](https://github.com/Tororoi/Chronicle-Expanded/pull/167)
- A new monster, the Bomb Gemron, can turn up on randomized floors: a black Gemron carrying bombs, weak to fire and immune to every other element. It throws bombs, explodes when it dies, and when nearly beaten with you close it lights its fuse and blows itself up. [#169](https://github.com/Tororoi/Chronicle-Expanded/pull/169)
- A new monster, the Crystal Gemron, can turn up on randomized floors: a pale blue Gemron with glowing eyes and crystal balls that shatter when it dies, weak to ice and immune to every other element. It fires the Ice Queen's homing ice arrow, which can freeze you, and encases you in her ice prison while you are frozen. [#170](https://github.com/Tororoi/Chronicle-Expanded/pull/170)
- Steve comments on every monster of a randomized floor, including ones from other dungeons, with his own lines about them. [#160](https://github.com/Tororoi/Chronicle-Expanded/pull/160)
- Enemy poison lasts 30 seconds, and each tick's damage scales with the monster's resistance. [#7](https://github.com/Tororoi/Chronicle-Expanded/pull/7)
- New Faster Enemies option (Options, Expanded page): monsters move and act twice as fast. Each attack still slows to its normal speed for the moment it can hit, so its blows land as before. [#151](https://github.com/Tororoi/Chronicle-Expanded/pull/151)
- New Harder Enemy AI option (Options, Expanded page): a defeated monster that has a get-up animation, such as a Skeleton Soldier, the Days of the Week or the card soldiers, gets back up about 30% of the time with part of its life, and the undead that already could (Mummy, Master Jacket, Gacious, Horn Head, Silver Gear) do so about 45% of the time. [#152](https://github.com/Tororoi/Chronicle-Expanded/pull/152)
- Minibosses: most floors, back floors included, have a miniboss, more likely one of the kind of monster there are the most of; a second one is rare, and never of the same kind. A miniboss is half again as big, with three times the life and ABS and four times the money, half again the defense and damage (shots too), and a longer reach. It spots you and attacks from farther away, takes a tenth of the damage of thrown items, and always drops something. One you leave alive is still there when you come back from the back floor, or go back down to it; one you defeat stays gone. [#153](https://github.com/Tororoi/Chronicle-Expanded/pull/153)
- Miniboss loot: instead of money, a miniboss drops an item tied to its kind of monster, from the table below: its rare drop 5% of the time and its common drop 30%. Stronger items come only from the dungeon they belong to onward (an item marked with a dungeon drops from there on), and some give way to a better one in a later dungeon (→). Otherwise, or when none of its items drops in this dungeon yet, it drops the back floor's key (35%), one of the dungeon's weapons, a gem or attachment, or a powder, Stamina Drink or Dran's Feather. Weapons marked ★ come boosted: once picked up, they have the stats in the second table. [#154](https://github.com/Tororoi/Chronicle-Expanded/pull/154)

### Miniboss loot
| Species | Rare (5%) | Common (30%) |
|---|---|---|
| Alexander |  | Drain Seeker (Gallery of Time+) or Garnet |
| Alexander (Enhanced) | Takes Alexander's loot | |
| Arthur | ★5 Foot Nail (Sun and Moon Temple+) | Steel Slingshot (Wise Owl Forest+) or Swallow (Moon Sea+) |
| Arthur (Enhanced) | Takes Arthur's loot | |
| Auntie Medu | Serpent Sword (Shipwreck+) | Turquoise, Garnet or Emerald |
| Auntie Medu (Enhanced) | Takes Auntie Medu's loot | |
| Billy |  | ★Trial Hammer (Gallery of Time+) |
| Bishop Q | Flamingo (Wise Owl Forest+) → ★Flamingo (Gallery of Time+) |  |
| Black Dragon | Dragon's Y (Sun and Moon Temple+) → ★Dragon's Y (Gallery of Time+) | Peridot |
| Blizzard | Crystal Ring (Shipwreck+) | Aquamarine or Turquoise |
| Blue Dragon | Dragon's Y (Sun and Moon Temple+) → ★Dragon's Y (Gallery of Time+) | Amethyst, Aquamarine or Turquoise |
| Bomber Head |  | Powerup Powder |
| Bomber Head (Enhanced) | Takes Bomber Head's loot | |
| Cannibal Plant and Cursed Rose | Thorn Armlet (Shipwreck+) → ★Thorn Armlet (Gallery of Time+) | Sun Dew (Wise Owl Forest+) |
| Cursed Rose (Enhanced) | Takes Cursed Rose's loot | |
| Captain | Flamingo (Shipwreck+) | Gold Bullion or Bravery Launch (Moon Sea+) |
| Captain (Enhanced) | Takes Captain's loot | |
| Cave Bat and Evil Bat | Drain Seeker (Gallery of Time+) | Poisonous Apple |
| Cave Bat (Enhanced) | Takes Cave Bat's loot | |
| Evil Bat (Enhanced) | Takes Evil Bat's loot | |
| Club | ★Cactus (Gallery of Time+) | Steel Hammer (Wise Owl Forest+) |
| Club (Enhanced) | Takes Club's loot | |
| Corcea | ★Dusack (Shipwreck+) | Chopper (Shipwreck+) or Bravery Launch (Moon Sea+) |
| Corcea (Enhanced) | Takes Corcea's loot | |
| Crabby Hermit | Snail (Moon Sea+) |  |
| Crabby Hermit (Enhanced) | Takes Crabby Hermit's loot | |
| Crescent Baron | Tsukikage (Wise Owl Forest+) or Mirage (Sun and Moon Temple+) | Pearl |
| Crescent Baron (Enhanced) | Takes Crescent Baron's loot | |
| Curse Dancer | Maneater (Shipwreck+) | Evilcise (Wise Owl Forest+) |
| Dark Flower | Thorn Armlet (Shipwreck+) → ★Thorn Armlet (Gallery of Time+) | 7 Branch Sword (Gallery of Time+) or Sun Dew (Wise Owl Forest+) |
| Dasher | Battle Ax (Wise Owl Forest+) | Topaz |
| Diamond | ★Big Bang (Gallery of Time+) | Diamond |
| Diamond (Enhanced) | Takes Diamond's loot | |
| Dragon | Dragon's Y (Sun and Moon Temple+) → ★Dragon's Y (Gallery of Time+) | Ruby or Garnet |
| Dune | Cactus (Sun and Moon Temple+) | Powerup Powder |
| Earth Digger and Moon Digger | Trial Hammer (Wise Owl Forest+) | Plate Hammer (Wise Owl Forest+) → Magical Hammer (Moon Sea+) |
| FliFli | Snail (Moon Sea+) |  |
| Gacious (Enhanced) | Satan's Ax (Gallery of Time+) | Bone Rapier |
| Fire Gemron | Satan's Ring (Demon Shaft+) | Ruby or Sun (Moon Sea+) |
| Holy Gemron | Goddess Ring (Demon Shaft+) | Peridot or Sun (Moon Sea+) |
| Ice Gemron | ★Crystal Ring (Demon Shaft+) | Aquamarine, Sapphire or Sun (Moon Sea+) |
| Thunder Gemron | ★Trial Hammer (Demon Shaft+) | Pearl or Sun (Moon Sea+) |
| Wind Gemron | ★Heaven's Cloud (Demon Shaft+) | Emerald or Sun (Moon Sea+) |
| Ghost and Lich | Cross Hinder (Sun and Moon Temple+) |  |
| Lich (Enhanced) | Takes Lich's loot | |
| Gol |  | Gold Bullion |
| Gol (Enhanced) | Takes Gol's loot | |
| Golem | Sun Sword (Sun and Moon Temple+) or Tsukikage (Sun and Moon Temple+) | Auto Repair Powder |
| Gunny |  | Pearl |
| Gyon | Frozen Tuna (Shipwreck+) → ★Frozen Tuna (Moon Sea+) | Aquamarine or Javelin (Sun and Moon Temple+) |
| Gyon (Enhanced) | Takes Gyon's loot | |
| Haley Holey | Flamingo (Wise Owl Forest+) | Sapphire |
| Halloween | Steve |  |
| Halloween (Enhanced) | Takes Halloween's loot | |
| Heart | ★Angel Shooter (Gallery of Time+) | Goddess Ring (Gallery of Time+) |
| Heart (Enhanced) | Takes Heart's loot | |
| Hell Pockle | Satan's Ring (Moon Sea+) or ★Bandit Slingshot (Wise Owl Forest+) → ★Pocklekul (Moon Sea+) |  |
| Horn Head | Satan's Ax (Gallery of Time+) | Matador (Moon Sea+) or Steel Hammer (Wise Owl Forest+) |
| Hornet and Phantom | Scorpion (Sun and Moon Temple+) |  |
| Joker | ★Super Steve (Gallery of Time+) | Steve |
| Joker (Enhanced) | Takes Joker's loot | |
| King Prickly | Cactus (Sun and Moon Temple+) | Steel Hammer (Wise Owl Forest+) |
| Mask of Prajna | ★Small Sword (Shipwreck+) | Amethyst |
| Mask of Prajna (Enhanced) | Takes Mask of Prajna's loot | |
| Master Jacket | ★Gladius | Gladius |
| Master Jacket (Enhanced) | Takes Master Jacket's loot | |
| Moon Bug |  | Powerup Powder |
| Mr. Blare |  | Blessing Gun (Sun and Moon Temple+) |
| Mummy | Claymore (Sun and Moon Temple+) | Revival Powder or Peridot |
| Mummy (Enhanced) | Takes Mummy's loot | |
| Nikapous | Macho Sword (Demon Shaft+) | Garnet |
| Opar |  | Opal |
| Pirate's Chariot |  | Steel Hammer (Shipwreck+) or Powerup Powder |
| Pirate's Chariot (Enhanced) | Takes Pirate's Chariot's loot | |
| Rash Dasher | Battle Ax (Wise Owl Forest+) |  |
| Rash Dasher (Enhanced) | Takes Rash Dasher's loot | |
| Rockanoff |  | Diamond |
| Rockanoff (Enhanced) | Takes Rockanoff's loot | |
| Sam |  | Crystal Ring (Shipwreck+) |
| Sil | ★Platinum Ring (Shipwreck+) |  |
| Sil (Enhanced) | Takes Sil's loot | |
| Silver Gear | Destruction Ring (Gallery of Time+) | Bone Slingshot |
| Skeleton Soldier |  | Bone Rapier or Bone Slingshot |
| Space Gyon | Frozen Tuna (Shipwreck+) → ★Frozen Tuna (Moon Sea+) | Javelin (Sun and Moon Temple+) |
| Space Gyon (Enhanced) | Takes Space Gyon's loot | |
| Spade | ★Dark Cloud (Gallery of Time+) | Brave Ark (Gallery of Time+) |
| Spade (Enhanced) | Takes Spade's loot | |
| Statue and Living Armor | ★Plate Hammer (Shipwreck+) | Steel Slingshot |
| Living Armor (Enhanced) | Takes Living Armor's loot | |
| Statue Dog | Steve |  |
| Statue Dog (Enhanced) | Takes Statue Dog's loot | |
| Steel Giant | ★Steel Slingshot (Wise Owl Forest+) or ★5 Foot Nail (Sun and Moon Temple+) | Opal or Diamond |
| Steel Giant (Enhanced) | Takes Steel Giant's loot | |
| Sunday to Saturday | ★Bandit Slingshot (Wise Owl Forest+) → ★Pocklekul (Moon Sea+) | Auto Repair Powder |
| Titan | Gaia Hammer (Moon Sea+) | Any gem |
| Titan (Enhanced) | Takes Titan's loot | |
| Vulcan | ★Blessing Gun (Moon Sea+) | Peridot, Garnet, Topaz, Diamond or Ruby |
| Vulcan (Enhanced) | Takes Vulcan's loot | |
| Werewolf | ★Lamb's Sword (Wise Owl Forest+) |  |
| White Fang | De Sanga (Moon Sea+) | Topaz |
| White Fang (Enhanced) | Takes White Fang's loot | |
| Witch Illza and Witch Hellza | Fruit of Eden (Shipwreck+) |  |
| Witch Hellza (Enhanced) | Takes Witch Hellza's loot | |
| Yammich | Evilcise |  |
| Yammich (Enhanced) | Takes Yammich's loot | |
| King Mimic | | Gold Bullion or any gem |
| Mimic | | Any gem |

| Boosted weapon | From | Picked up with |
|---|---|---|
| Gladius | Master Jacket | Attack 15, Holy 50, Anti-Undead 50 |
| Bandit Slingshot | Sunday to Saturday (Wise Owl Forest+), Hell Pockle (Wise Owl Forest+) | Attack 35, Wind 40, Anti-Mimic 40 |
| Lamb's Sword | Werewolf (Wise Owl Forest+) | Attack 60, Magic 30, WHP 99 |
| Dusack | Corcea (Shipwreck+) | Steal |
| Small Sword | Mask of Prajna (Shipwreck+) | Attack 40, Thunder 40, Anti-Mage 40 |
| Platinum Ring | Sil (Shipwreck+) | Attack 30, Holy 40, Anti-Stone 40, Durable |
| Frozen Tuna | Gyon (Moon Sea+), Space Gyon (Moon Sea+) | WHP 99, Endurance 99, Ice 99, Anti-Sea 99, Big Bucks, Fragile |
| Blessing Gun | Vulcan (Moon Sea+) | Attack 50, Magic 30, Fire 40, Anti-Dino 40, Anti-Plant 20 |
| Pocklekul | Sunday to Saturday (Moon Sea+), Hell Pockle (Moon Sea+) | Steal |
| Dragon's Y | Dragon (Gallery of Time+), Blue Dragon (Gallery of Time+), Black Dragon (Gallery of Time+) | Thunder 40, Anti-Beast 40, Anti-Sky 40 |
| Cactus | Club (Gallery of Time+) | WHP 99, Endurance 99, Wind 50, Anti-Plant 40, Anti-Sea 40, Critical |
| Thorn Armlet | Cannibal Plant and Cursed Rose (Gallery of Time+), Dark Flower (Gallery of Time+) | Critical |
| Big Bang | Diamond (Gallery of Time+) | WHP 99, Durable |
| Angel Shooter | Heart (Gallery of Time+) | WHP 99, Fire 40, Thunder 40, Holy 40 |
| Super Steve | Joker (Gallery of Time+) | WHP 99, Attack 80, Magic 50, Steal, Abs Up |
| Dark Cloud | Spade (Gallery of Time+) | WHP 99, Fire 40, Holy 40, Anti-Mage 30, Stop, Drain |
| Trial Hammer | Billy (Gallery of Time+), Thunder Gemron (Demon Shaft+) | Thunder 50 |
| Crystal Ring | Ice Gemron (Demon Shaft+) | Ice 50 |
| Heaven's Cloud | Wind Gemron (Demon Shaft+) | Wind 50 |
| Flamingo | Bishop Q (Gallery of Time+) | Abs Up |
| Steel Slingshot | Steel Giant (Wise Owl Forest+) | Durable |
| 5 Foot Nail | Steel Giant (Sun and Moon Temple+), Arthur (Sun and Moon Temple+) | Durable |
| Plate Hammer | Statue and Living Armor (Shipwreck+) | Durable |

## Towns

- Play as any ally in town: commit them in the party menu with the dungeon's change sequence, and they walk with full animations. [#9](https://github.com/Tororoi/Chronicle-Expanded/pull/9), [#10](https://github.com/Tororoi/Chronicle-Expanded/pull/10), [#11](https://github.com/Tororoi/Chronicle-Expanded/pull/11), [#12](https://github.com/Tororoi/Chronicle-Expanded/pull/12), [#13](https://github.com/Tororoi/Chronicle-Expanded/pull/13)
- Each ally takes ladders their own way: Xiao leaps, Goro hops, Ruby floats, Osmond flies and Ungaga refuses. [#14](https://github.com/Tororoi/Chronicle-Expanded/pull/14)

## Fishing

### From Chronicle

- Fish sizes taper smoothly into each species' maximum instead of piling up at it. [Chronicle#141](https://github.com/TheMoonPeople/Chronicle/pull/141)

## Developer tools

- Enemy roster: the developer menu's "enemy roster" row (typed in) or `--roster 20!,60` lays out these monster kinds on every dungeon floor from the next dungeon visit, in debug mode. [#156](https://github.com/Tororoi/Chronicle-Expanded/pull/156)

### From Chronicle

- Developer/boot menu, debug mode from `config.json` or a pad chord, and debug shortcuts under Select+L2. [Chronicle#119](https://github.com/TheMoonPeople/Chronicle/pull/119), [dda4b7f0](https://github.com/TheMoonPeople/Chronicle/commit/dda4b7f0)
- Items and weapons given at start with `--give`. [Chronicle#117](https://github.com/TheMoonPeople/Chronicle/pull/117)
- Text wipe (F9) compares the original picture text with the new runtime text. [Chronicle#174](https://github.com/TheMoonPeople/Chronicle/pull/174)

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
- Once your disc is extracted, the window shows the memory card's lamp icon from it, and Windows gets a Dark Cloud shortcut with that icon. [Chronicle#190](https://github.com/TheMoonPeople/Chronicle/pull/190), [Chronicle#208](https://github.com/TheMoonPeople/Chronicle/pull/208)
- Nintendo Switch port (homebrew, Vulkan). [9b3ff321](https://github.com/TheMoonPeople/Chronicle/commit/9b3ff321), [0c9c0ee6](https://github.com/TheMoonPeople/Chronicle/commit/0c9c0ee6)
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
- The title screen has an Exit row, which asks before closing the game, its four rows drawn in every language in a font made from the title's painted capitals; Quit in the pause menu now asks and returns to the title screen instead of the desktop. [Chronicle#211](https://github.com/TheMoonPeople/Chronicle/pull/211)

## Fixes

- With Heaven's Cloud's SynthSphere on Super Steve, an ordinary pellet no longer bursts like a charged one when a charged pellet was still flying as you left the last floor. [#240](https://github.com/Tororoi/Chronicle-Expanded/pull/240)
- A monster confused by Babel's Spear can't be blocked by a blinded monster's guard, and a shot it fires that bursts on another monster bursts with its full blast damage and size, as it does on you. [#239](https://github.com/Tororoi/Chronicle-Expanded/pull/239)
- A blinding (the Sun Sword's Solar Flash and the attacks that share it) no longer wears off during an event or while the monsters are held, such as a chest opening: like poison and confusion, it waits. [#238](https://github.com/Tororoi/Chronicle-Expanded/pull/238)
- The Bomb Gemron throws its bombs again, and Expanded's other shot effects load again: since the last upstream update the game could not look inside the effect files Expanded reads for itself. [#202](https://github.com/Tororoi/Chronicle-Expanded/pull/202)

### From Chronicle

- Treasure chests weight items by the dungeon's own rate list, so the Flapping Duster, Goddess Ring and Aga's Sword can drop. [Chronicle#76](https://github.com/TheMoonPeople/Chronicle/pull/76)
- Sixteen regular monsters that never dropped anything (Hornet, Ghost, Lich, Cave Bat, Gol, Sil and others) now drop money and items. [Chronicle#142](https://github.com/TheMoonPeople/Chronicle/pull/142)
- Treant's chests are no longer lost by finishing Matataki's river before forest floor 8. [Chronicle#146](https://github.com/TheMoonPeople/Chronicle/pull/146), [Chronicle#153](https://github.com/TheMoonPeople/Chronicle/pull/153)
- The Broken Dagger glitch in the item shop is gone. [Chronicle#158](https://github.com/TheMoonPeople/Chronicle/pull/158)
- A bomb's blast throws the player away from it, not always the same way. [Chronicle#181](https://github.com/TheMoonPeople/Chronicle/pull/181)
- The HUD's weapon icon keeps its colours after closing a menu on the PAL disc in English. [Chronicle#91](https://github.com/TheMoonPeople/Chronicle/pull/91)
- On the PAL disc, Carrot, Potato Cake, Minon, Battan and Petite Fish go on the hook as fishing bait; choosing one used it up and left the hook empty. [Chronicle#212](https://github.com/TheMoonPeople/Chronicle/pull/212)

## Game mechanics

- A hit on a frozen player breaks the freeze. [#4](https://github.com/Tororoi/Chronicle-Expanded/pull/4)
- Fruit of Eden can be sold and thrown away. [#161](https://github.com/Tororoi/Chronicle-Expanded/pull/161)

### From Chronicle

- Element Quick Select (off by default): D-pad Up in a dungeon picks the equipped weapon's element. [Chronicle#111](https://github.com/TheMoonPeople/Chronicle/pull/111)

## Weapons

- Weapon Abilities option (Options, Expanded page), on by default: off, every weapon's special ability below is off, Super Steve's SynthSpheres lend nothing, and the weapons work as in the original game; what a weapon was given when it was made or built up stays with it. [#237](https://github.com/Tororoi/Chronicle-Expanded/pull/237)
- The Heal ability heals every 3 seconds instead of 4. [#3](https://github.com/Tororoi/Chronicle-Expanded/pull/3)
- Ungaga's charge shots wear his weapon only when fired, not again for every hit. [#5](https://github.com/Tororoi/Chronicle-Expanded/pull/5)
- New Confuse ability: 5% of hits confuse a non-boss monster for 20 seconds, turning it on other monsters. A weapon's SynthSphere passes it on. [#6](https://github.com/Tororoi/Chronicle-Expanded/pull/6)
- Super Steve shows the weapon icon of the SynthSphere attached to it on the dungeon screen, and a slingshot's sphere makes it fire that slingshot's pellets. [#145](https://github.com/Tororoi/Chronicle-Expanded/pull/145)
- Baselard "Heavy Hand": every hit throws the monster back about 17 units, and so does every pellet of Super Steve carrying the Baselard's SynthSphere. [#146](https://github.com/Tororoi/Chronicle-Expanded/pull/146)
- Gladius "Jacket Hunter": a Master Jacket killed with the Gladius in hand gives four times its ABS, and so does one killed by Super Steve carrying the Gladius's SynthSphere. [#184](https://github.com/Tororoi/Chronicle-Expanded/pull/184)
- Crysknife "Crystal Affinity": thrown elemental gems deal twice their damage while the Crysknife is in hand, or Super Steve carries its SynthSphere. [#185](https://github.com/Tororoi/Chronicle-Expanded/pull/185)
- Macho Sword "Overtraining": while you own a Macho Sword (in hand, a bag or storage), every weapon's ABS keeps filling past its max, up to twice it, and the surplus carries into the next level (a weapon at 116/100 starts its new level at 16). [#186](https://github.com/Tororoi/Chronicle-Expanded/pull/186)
- Buster Sword "Buster Boost": anti-monster attachments (Dinoslayer to Mage Slayer) are worth +4 instead of +3 on a Buster Sword, in its stats, its damage, its build-ups and what a level-up absorbs. [#187](https://github.com/Tororoi/Chronicle-Expanded/pull/187)
- Sun Sword "Solar Harvest": while the Sun Sword, Big Bang or Sword of Zeus is in hand, or Super Steve carries one's SynthSphere, each monster you defeat has a 1% chance to drop a Sun attachment. A monster carrying a key or another set drop still drops that. [#199](https://github.com/Tororoi/Chronicle-Expanded/pull/199)
- Sun Sword "Solar Flash": hold guard for 1.5 seconds and the blade whitens as the room darkens, a gold glow growing on its hilt until the charge is ready (let go early, the glow shrinks away over half a second). The next swing flashes the room white: every monster within 300 takes a light hit (a quarter of the attack, no element) and is thrown away from the flash, then the floor's monsters (bosses aside) stand blinded for 5 seconds, holding up a guard that blocks nothing. The flash costs 5 WHP, worn like a swing (Endurance, Durable and Fragile apply). A charge unused for 10 seconds fades. On Super Steve, the Sun Sword's SynthSphere charges in 2 seconds and the next pellet carries the flash to wherever it lands, the monster it struck taking the pellet instead of the light hit. [#200](https://github.com/Tororoi/Chronicle-Expanded/pull/200)
- Wise Owl Sword "Wise Owl Always Knows": while you own a Wise Owl Sword, a line at the bottom of the screen names the key (Shiny Stone, Red Berry or Pointy Chestnut) carried by the nearest monster within 500 that has one, as it comes in range. [#188](https://github.com/Tororoi/Chronicle-Expanded/pull/188)
- Crysknife / Magical Hammer "Circle Amplifier": owning one of them (in hand, a bag or storage) doubles every magic circle's effect, owning both triples it: attack up 60 or 90 s, more money gained or lost, WHP and stat changes twice or three times as large, monsters enraged 10 or 15 s, and the WHP-loss circle taking WHP to 1. The ABS-max and WHP-cure circles reach every party member's equipped weapon when doubled and every carried weapon when tripled. An ABS-max circle never lowers a weapon the Macho Sword overtrained past its max. [#189](https://github.com/Tororoi/Chronicle-Expanded/pull/189)
- Bone Rapier / Bone Slingshot "Skeleton Key" and "Gravedigger": in hand, Divine Beast Cave's bone doors open without the Bone Key, and every monster that would get back up stays down: the undead that revive by themselves (Mummies, Master Jackets, Gacious, Silver Gears, Horn Heads) and those Harder Enemy AI raises. On Super Steve, either bone sphere gives both; a Cross Hinder sphere gives Gravedigger. [#190](https://github.com/Tororoi/Chronicle-Expanded/pull/190)
- Cross Hinder "Sanctifier": with the Cross Hinder in hand, or Super Steve carrying its SynthSphere, hits on undead deal double damage, an undead you kill gives double ABS, and an undead that would get back up stays down. You also lock on from twice as far, as you do with Big Bang and the Sword of Zeus (in hand or as Super Steve's sphere). [#201](https://github.com/Tororoi/Chronicle-Expanded/pull/201)
- Big Bang "Detonate": the whirlwind is an explosion at Toan's feet in place of the tornado: every monster within 50 takes 4×, 3×, 2× or 1× the attack by how close it stands (within 10, 25, 40, 50), with no element and no guard stopping it, and is thrown about 50 away; it costs 20 WHP. Its guard charge is the Sun Sword's Solar Flash at half the attack, a cool white light; locked on, a giant copy of the blade hangs over the target with a blue glow and drops on the first swing, its landing that explosion, the flash and the blinding (20 WHP; not locked on, the flash alone for 5). Explosions cannot hurt whoever carries it. On Super Steve, its SynthSphere: guard 3 seconds and, locked on, a bomb four times its size hangs glowing over the target and drops when the guard is let go, its blast passing any guard (20 WHP; not locked on, the flash alone for 5); a shot held drawn a second flies as a bomb that explodes where it lands at half the drop's strength, which a guard can stop (10 WHP); every other shot flies as a small bomb that bursts where it lands. Each explosion rumbles the controller as a knockdown blow does, a small bomb's burst as a lighter hit. [#203](https://github.com/Tororoi/Chronicle-Expanded/pull/203)
- Sword of Zeus "Lightning": its guard charge is the Sun Sword's Solar Flash with no hit of its own and an electric-white light. Locked on, a giant copy of the blade hangs over the target with a red ring and drives into the ground to the hilt on the first swing, calling a lightning bolt down on the target; every later swing of the combo calls another as it comes forward. Not locked on, the swing calls a bolt on each of the six nearest monsters within 300. A bolt is a Big Bang explosion's damage over a smaller reach (4×, 3×, 2× or 1× the attack within 8, 20, 32 or 40), through any guard, as thunder whatever element the sword has selected (a monster immune to thunder, like Billy, takes nothing), with a clap of the Dark Heaven Castle storm's thunder (the whole storm sample for a volley) and the controller's knockdown rumble (10 WHP; a volley costs 10 in all). Its charge attack is always the lunge; held 3 seconds past the lunge, the lunge jumps half again as high with the blade hanging over where it will land, and a bolt comes down as Toan does. On Super Steve, its SynthSphere: guard 3 seconds, then, not locked on, the next shot calls a bolt where it lands; locked on, for 5 seconds every shot that reaches a monster calls a bolt on it; a shot held drawn a second calls a bolt where it lands. A shot that calls a bolt does no damage itself. [#204](https://github.com/Tororoi/Chronicle-Expanded/pull/204)
- Small Sword "Quick Draw": the opening combo swing skips its wind-up and lands about 0.2 s sooner; Tsukikage and Heaven's Cloud have it too. On Super Steve, any of their spheres fires the shot the instant the button is let go and refills the shot gauge almost at once. [#191](https://github.com/Tororoi/Chronicle-Expanded/pull/191)
- Tsukikage "Moonlit Focus": charge attacks build twice as fast (lunge about 0.25 s, whirlwind about 0.75 s), on top of Quick Draw; Heaven's Cloud has both. On Super Steve, a Tsukikage or Heaven's Cloud sphere makes pellets fly twice as fast, plus Quick Draw. [#192](https://github.com/Tororoi/Chronicle-Expanded/pull/192)
- Shamshir "Swift Strikes": the five combo swings play 4/3 as fast (charge attacks unchanged); the Dusack, 7 Branch Sword, Atlamillia Sword and Chronicle Sword have it too. On Super Steve, any of their spheres or a Partisan's makes the draw play 1.6 times as fast and the shot fire sooner. [#193](https://github.com/Tororoi/Chronicle-Expanded/pull/193)
- Sax "Fine Fare": while it is out, item chests give Premium Water for water and Premium Chicken for Bread and Cheese. The Dusack also turns Treasure Keys into Gold Bullion; the 7 Branch Sword, Atlamillia Sword and Chronicle Sword also turn one Repair Powder chest in four into Auto Repair Powder. On Super Steve, any of the five swords' spheres gives that sword's fare. [#194](https://github.com/Tororoi/Chronicle-Expanded/pull/194)
- Dusack / Brave Ark "No Fool's Gold": mimics and king mimics can be hit through the guard they wake with; their own guard and the short pause after a hit still hold. The Dusack also has Fine Fare at its tier and Swift Strikes. On Super Steve, a Dusack or Brave Ark sphere gives the same. [#195](https://github.com/Tororoi/Chronicle-Expanded/pull/195)
- Kitchen Knife "Spring's Blessing": stepping into a healing spring blesses the knife for 60 seconds, tripling its blade (and reach) and doubling its attack; standing in the spring keeps the blessing fresh, and each step into it mends the knife's WHP to full. [#196](https://github.com/Tororoi/Chronicle-Expanded/pull/196)
- Evilcise "Jealous Soul": Toan is cursed while it is equipped (from the pause menu too) and takes no other harmful ailment; lifting the curse with holy water poisons him and leaves him 1 HP. The curse returns on every floor and after a revival, and lifts when it is unequipped or he leaves the dungeon. On Super Steve, its sphere curses Xiao the same way. [#197](https://github.com/Tororoi/Chronicle-Expanded/pull/197)
- Aga's Sword "Defensive Legacy": Toan has 15 more defense while it is equipped, on top of what food has raised it to, and the menus show it. Super Steve carrying its SynthSphere gives Xiao the same. [#206](https://github.com/Tororoi/Chronicle-Expanded/pull/206)
- Dark Cloud "Guard Crush": with the Dark Cloud or the 7th Heaven in hand, no monster's guard stops Toan's hits; the monster still raises its guard, but the hit lands. Super Steve carrying either's SynthSphere does the same for Xiao's shots. [#207](https://github.com/Tororoi/Chronicle-Expanded/pull/207)
- 7th Heaven "Divine Guard": while it is in hand, Toan's guard blocks every monster attack and shot, even the heavy blows that normally break through a guard and knock him down, and his guard stays up through each block instead of being raised again; unguarded, those blows now throw him back instead of knocking him down. It also has the Dark Cloud's Guard Crush. Super Steve carrying its SynthSphere gives Xiao the same guard. [#209](https://github.com/Tororoi/Chronicle-Expanded/pull/209)
- Brave Ark "Hero's Courage": while it is equipped, Toan cannot be frozen, poisoned, cursed or gooed. A monster's hit carrying one does nothing (and uses up no amulet), and one he already has, or gets from an item, is gone at once. Super Steve carrying its SynthSphere does the same for Xiao. [#210](https://github.com/Tororoi/Chronicle-Expanded/pull/210)
- Claymore "Greatsword": in Toan's hand the Claymore is 1.8 times its size and reaches that much farther, still hitting what the normal blade would, and every hit throws the monster back as the Baselard's do. Super Steve carrying its SynthSphere throws monsters back too. [#211](https://github.com/Tororoi/Chronicle-Expanded/pull/211)
- Maneater "Blood Price": Toan is cursed while it is equipped, again on every floor, but holy water lifts the curse with no penalty. While its WHP is low enough for the gauge to blink, it feeds on him: 1 HP a second to mend 1 WHP, never leaving him below 1 HP. Super Steve carrying its SynthSphere curses Xiao the same way and mends itself from her. [#212](https://github.com/Tororoi/Chronicle-Expanded/pull/212)
- 7 Branch Sword "Sevenfold Rite": it refuses a Status Break below +7, and from +7 its SynthSphere keeps 77% of its stats, elements and anti-monster values instead of 60%; the weapon menu's Status Break hint and confirmation say so. [#213](https://github.com/Tororoi/Chronicle-Expanded/pull/213)
- Atlamillia Sword "Atlamillia Insurance": while you own one, a weapon that breaks in a dungeon leaves an Atla on a random floor of that dungeon holding its SynthSphere, with 10% of its stats for each level from +0, up to 50% at +4; its attachments are lost. The Atla comes back each time you visit that floor until you pick it up, and is kept with your save. [#214](https://github.com/Tororoi/Chronicle-Expanded/pull/214)
- Heaven's Cloud "Typhoon": holding the charge past the whirlwind's threshold grows the sword up to three times its size over four seconds, with the charge flash once it is full grown, and the whirlwind it lets go is as much bigger: as it starts it hits every monster within its reach and throws each out past it. On Super Steve, its SynthSphere: a shot held drawn charges in two stages, each with the charge flash; a charged shot's pellet grows (up to eight times its size) and hits harder (up to 1.5 times), and where it lands among the monsters it bursts in a wind blast (60 to 160 across with the charge) that hits every monster in it and throws them out to its edge. [#215](https://github.com/Tororoi/Chronicle-Expanded/pull/215)
- Chronicle 2: while you own it, SynthSpheres attached to a weapon can take its attack and magic past their usual max. Only while attached: a level-up that absorbs them still stops at the max. [#216](https://github.com/Tororoi/Chronicle-Expanded/pull/216)
- Steel Slingshot "Endurance Up": while its WHP is low enough for the gauge to blink, each shot wears it half as much (Durable and Fragile still apply on top), and each level-up adds 2 endurance instead of 1 and twice the usual roll to its max WHP. Super Steve carrying its SynthSphere has the half-wear too. [#217](https://github.com/Tororoi/Chronicle-Expanded/pull/217)
- Bandit Slingshot "Steal Shot": a steal that lands on a monster with a projectile takes the projectile too. Until Xiao steals another or the floor ends, every pellet she fires is that monster's shot, flags and element and all, turned on the monsters at twice the pellet's attack, and the stolen item's "acquired" notice adds "[monster]'s projectile is now yours". Self-detonations are never taken: a monster with only that gives the item alone. Super Steve carrying either Bandit's SynthSphere does the same. [#218](https://github.com/Tororoi/Chronicle-Expanded/pull/218)
- Flamingo "Lock-On Distance": monsters can be locked on from twice as far with the Flamingo in hand, or a slingshot built from it (Dragon's Y, Divine Beast Title, Angel Shooter, Angel Gear), or Super Steve carrying any of their SynthSpheres. [#219](https://github.com/Tororoi/Chronicle-Expanded/pull/219)
- Flamingo "Bait Flamboyance": while you own a Flamingo (in any bag or the storage), every bait is noticed by fish from 10 units further, for each Flamingo owned up to three. [#219](https://github.com/Tororoi/Chronicle-Expanded/pull/219)
- Hardshooter "Ricochet": a pellet that lands on a monster sends a second pellet from it at the nearest other monster (within 120), aimed at its largest hurt sphere taking hits at the time, or in a random direction when none is that near. The second pellet has the first's damage, never comes back at the monster struck and does not ricochet again; a wall hit sends none. Super Steve carrying its SynthSphere does the same. [#220](https://github.com/Tororoi/Chronicle-Expanded/pull/220)
- Double Impact: every shot is two pellets side by side, each at three quarters of the attack and each rolling the weapon's abilities on its own, both drawn as the Steel Slingshot's stone. Both ricochet as the Hardshooter's do, at different monsters. Super Steve carrying its SynthSphere does the same. [#221](https://github.com/Tororoi/Chronicle-Expanded/pull/221)
- Matador "Charging Bull": hold the shot drawn for a second, until the charge flash, and the pellet let go is charged: one and a half times the damage, through a guard, and a shove along its flight like Goro's hammer swing. It flies as an orange copy of the slingshot in hand, upright in a fire glow, its elastic drawn back and the pellet in its pouch. Super Steve carrying its SynthSphere does the same, sending a copy of Super Steve. [#222](https://github.com/Tororoi/Chronicle-Expanded/pull/222)
- Dragon's Y: locked on, Xiao moves 1.3 times as fast with the Dragon's Y or a slingshot built from it (Divine Beast Title, Angel Shooter, Angel Gear), or Super Steve carrying any of their SynthSpheres. With the Dragon's Y itself, a shot held for a second (the charge flash) fires the Gemron ball of the selected element in place of the pellet (fire, ice, thunder, wind or holy; the Black Dragon's with none), at one and a half times the damage. The weapon's abilities ride on it, the ball's own ailment is left off, and a monster it hits is thrown back along its flight. Super Steve carrying its SynthSphere fires the ball too. [#223](https://github.com/Tororoi/Chronicle-Expanded/pull/223)
- Divine Beast Title "Spirit Beast": hold the shot through the charge flash (half a second) and Xiao's cat comes out of the pellet, stepping as the mod's does. It grows on the pellet, breaks away along its flight and lands; it walks straight to the monster she is locked on to (or would lock on to, else the nearest), standing where a wall stops it, crouches and pounces, its crouch, wind-up and landing timed by its clips. The first monster that takes its touch takes one hit of the pellet's damage plus the weapon's attack, in the weapon's element and through a guard, staggering it and shoving it back like a sword blow, and the cat fades away; a touch the monster refuses leaves it to touch again. A mimic still in its chest draws it too: it waits crouched by the chest and pounces as the mimic comes out. With nothing to chase it sits; it fades after 20 seconds, and another character or weapon in hand, or an event, takes it away at once. It glows blue. Super Steve carrying its SynthSphere lets the same cat go. `dcdata` builds the cat from the disc's two town cats when it extracts, and rebuilds monster scripts as the mod does: every mimic wakes guarding rather than untouchable, Blizzard, Sam and Billy take Titan's and Mr. Blare's hurt spheres, and every hurt sphere of Minotaur Joe's takes the cat's hit in full. [#224](https://github.com/Tororoi/Chronicle-Expanded/pull/224)
- Angel Shooter "Guardian Grace": while Xiao guards, the slingshot's Heal ability ticks every second, the guard starting with the healing spring's sparkle, a soft white flash and the heal chime, her colour swelling with each tick. Its Spirit Beast cat wears Dran's wings, is 1.1 times the size, crouches from 50 rather than 30 (30 still by a mimic in its chest) and steers its pounce on past the apex. Super Steve carrying its SynthSphere has the heal, and the cat at that size and range wearing a cape and mask in the selected element's colour instead of the wings. [#225](https://github.com/Tororoi/Chronicle-Expanded/pull/225)
- Xiao's charged shots, as the mod's: the charge counts from her drawing the slingshot back, not only once it is drawn (Matador, Dragon's Y, the Divine Beast Title's and Angel Shooter's cat, Big Bang and the Sword of Zeus on Super Steve), and a charged shot costs twice the weapon HP of an ordinary one (Matador, Dragon's Y, the cat, and an empowered Typhoon); the Steel Slingshot's halved cost at low WHP is doubled with it. The Spirit Beast cat keeps its tint as it fades. [#226](https://github.com/Tororoi/Chronicle-Expanded/pull/226)
- Spirit Beast cat: as it sits down it meows, with the cat sound of the game's own scenes with Xiao's cat. [#227](https://github.com/Tororoi/Chronicle-Expanded/pull/227)
- Angel Gear "Guardian Reflector": while Xiao guards, a giant copy of the slingshot in her hand, glowing gold, stands in front of her, turning about her to face the nearest shot coming at her, else the nearest monster. A monster's shot that reaches its pouch is caught there, or vanishes into her with a dull flash, and once nothing more is coming the slingshot draws its pouch back and fires the shot from its fork at the nearest monster. That hit is the slingshot's attack scaled by the dungeon (a fourteenth in Divine Beast Cave up to half in Demon Shaft), in the shot's element, and gives the freeze, poison or slow the shot carries. The slingshot is also a shield: monsters walk up to it and swing at it rather than at her, and five swings break it, with the weapon-break sound. While it stands, the action gauge shows its health; after a break the gauge refills over four seconds, and the slingshot only stands on a full gauge. The Angel Gear also has Guardian Grace, its heal coming every half second while she guards, beginning with a golden burst, a warm flash and the character-change jingle, her colour swelling gold once a second, and the Spirit Beast, whose cat wears Dran's wings, glows gold and is 1.2 times the size. The Angel Gear always has the Heal ability, and with it in Xiao's hand each tick of that ability heals the rest of the party 1 HP as well as her. Super Steve carrying its SynthSphere stands its own model as the shield, has Guardian Grace, heals the party while its sphere carries the Heal ability, and lets its cat go at that size wearing the cape and mask. [#228](https://github.com/Tororoi/Chronicle-Expanded/pull/228)
- Partisan "Quick Combo": with the Partisan in Ungaga's hand, his three combo swings play 4/3 as fast; his charge keeps its pace. Super Steve carrying its SynthSphere has Swift Strikes' faster draw and shot. [#229](https://github.com/Tororoi/Chronicle-Expanded/pull/229)
- Halberd "Tornado Charge Buff": the shot Ungaga's held charge fires flies faster, and so farther, and is larger, hitting as much wider: 1.4 times the speed and 1.1 times the size with the Halberd, 1.7 and 1.3 with the Scorpion, 2 and 1.5 with the Mirage and the Cactus, and 2.5 and 1.7 with Hercules' Wrath, the Terra Sword and Babel's Spear. Super Steve carrying a SynthSphere of any of them charges a shot held drawn for half a second (the charge flash): it costs twice the weapon HP, and its pellet flies faster and is drawn larger by that weapon's amounts, for one and a half times the damage. [#230](https://github.com/Tororoi/Chronicle-Expanded/pull/230)
- Scorpion "Venom": each time a monster is poisoned with the Scorpion in Ungaga's hand, his own poison is cured and the weapon takes half that monster's ABS, up to its maximum. A monster already poisoned does not count again. Its charge shot flies at the Scorpion's Tornado Charge, 1.7 times the speed and 1.3 times the size. Super Steve carrying its SynthSphere cures Xiao and fills itself the same way, and its charged shot flies at that tier. [#233](https://github.com/Tororoi/Chronicle-Expanded/pull/233)
- Javelin "Marine Hunter": with the Javelin in hand, or Super Steve carrying its SynthSphere, marine monsters (the ones Sea Killer works on) lose all their defense: none of it comes off a hit's damage, and their hardness no longer adds wear to the weapon that hits them. A marine monster you kill gives twice its ABS. [#231](https://github.com/Tororoi/Chronicle-Expanded/pull/231)
- DeSanga "Vampire": every monster Ungaga kills with the DeSanga in hand restores 5 of its weapon HP, up to its max. Super Steve carrying its SynthSphere does the same for Xiao's kills. [#232](https://github.com/Tororoi/Chronicle-Expanded/pull/232)
- Mirage "Decoy": with the Mirage or Hercules' Wrath in hand, hold guard for a quarter of a second and Ungaga flashes and leaves a copy of himself where he stands, holding his guard under a heat shimmer; the shimmer rises first and the copy fades in through it. For 12 seconds (18 with Hercules' Wrath) every monster goes for the copy and attacks it instead of him, and a monster he hits turns back on him. Holding guard again plants a new decoy where he stands: the old copy dissolves, the monsters keep to its spot until the new copy has fully appeared, then all of them go for the new one, those he had hit too. At the end the copy fades out before its shimmer. While a decoy stands, a confused monster goes for the copy rather than the other monsters. The Mirage, Terra Sword, Hercules' Wrath and Babel's Spear also lock on from twice as far. Super Steve carrying a Mirage or Hercules' Wrath SynthSphere plants Xiao's own decoy, and any of the four spheres gives the lock-on reach. [#234](https://github.com/Tororoi/Chronicle-Expanded/pull/234)
- Cactus "Absorb" and "Desert Bloom": each hit Ungaga lands with the Cactus restores his thirst by a tenth of the damage it does (100 damage is one drop), up to a full gauge; rock, metal and undead monsters give none. Holding the guard for a quarter of a second raises a giant cactus out of the floor just ahead of him, in a poof of smoke, with the charge flash and the sound of an item box opening: it shoots up, swells past its size and settles. For ten seconds it stands as a solid pillar: monsters walk round it, Ungaga cannot walk through it, and monsters' shots stop against it. Every quarter of a second it pricks each monster at it for a sixth of the weapon's attack with a short shove, costing the weapon no WHP. It casts a round shadow, then fades away; holding the guard again while it stands moves it. Super Steve carrying its SynthSphere restores Xiao's thirst with her pellets, and raises Queens' two trees in its place: a wall that hurts nothing. [#235](https://github.com/Tororoi/Chronicle-Expanded/pull/235)
- Hercules' Wrath "Air Strike": once the decoy is down, keep holding guard five more seconds and the room darkens as the spear turns gold, the decoy's spear with it; letting go early eases the light and the gold back over half a second. Charged, the strike waits as long as a decoy stands, and fades away as the last one dissolves. Ungaga's next swing calls it on the decoy: a sparkle twinkles at the tip of the decoy's spear and falls to the ground there with the sound of the light that ends the Black Knight, the room plunges to black, and a blast lands on the spot through any guard, 4, 3, 2 and 1 times the attack within 20, 50, 80 and 100 units, throwing the monsters back. The decoy vanishes in a white flash that fades over two seconds, the strike costs 20 WHP (the blast's hits wear nothing more), and the sparkle and the gold fade out together. Super Steve carrying its SynthSphere charges the same way, the slingshot turning gold, and the first pellet Xiao fires after brings the strike down where it ends, on the floor there. [#236](https://github.com/Tororoi/Chronicle-Expanded/pull/236)
- Snail "Slime Trail": each hit Osmond lands with the Snail has a 1 in 20 chance to goo the monster, turning it blue and slowing it to half speed as a gooey hit does; a monster that ailments cannot touch, or one already frozen, poisoned or angry, is left as it is. Super Steve carrying its SynthSphere gives Xiao's pellets the same chance. [#250](https://github.com/Tororoi/Chronicle-Expanded/pull/250)
- Secret Armlet "Favoured Circles": while the Secret Armlet is owned (in hand, a bag or storage), every bad magic circle turns out well. The money-loss circle gives money, the max-WHP-loss circle raises the max WHP, the WHP-loss circle restores the WHP, and the stat-loss circle raises the stat, element and anti it would have lowered by the same amounts, up to 99. The circle that enrages the monsters slows every monster on the floor to half speed for 5 seconds instead, saying "Monsters slowed down." The Circle Amplifier makes these circles stronger too, the slow lasting 10 or 15 seconds. [#253](https://github.com/Tororoi/Chronicle-Expanded/pull/253)
- Pocklekul "Pockle Call": with the Pocklekul in hand, hold guard for a second and Ruby flashes as a pockle pops out of a poof of smoke in front of her, one of the seven day pockles or Hell Pockle at random, with a notice naming it. It is a real pockle with its own life: it grows in from tiny, glowing as it appears, then fights beside her, going for the monster she is locked on to (else the one nearest her); with none about it walks back to her right side and follows her. Its kills and their ABS are Ruby's, and its stealing swings take a monster's gold, which it drops when it falls. Monsters it hits or bumps turn on it, and its swings never hit Ruby. Only one comes at a time: it leaves when it falls, when Ruby gets far from it, when she changes weapon or member, or with the floor. Super Steve carrying its SynthSphere summons the same pockle from Xiao's guard. [#255](https://github.com/Tororoi/Chronicle-Expanded/pull/255)

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
| Angel Gear | Heal (always) |  |  |  |  |  |  |  |  | [#228](https://github.com/Tororoi/Chronicle-Expanded/pull/228) |
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
- A new monster, the Atla Gemron, can turn up on randomized floors: a deep blue Gemron holding an atla and wearing the Atlamillia on its forehead, weak to wind and immune to every other element. It fires the Atlamillia's burst straight ahead, which grows as it flies. When it dies the atla bounces free and stays on the floor: pick it up for an Atlamillia Sword (from Sun & Moon Temple on), an Atlamillia SynthSphere, the dungeon's featured weapon's SynthSphere or another weapon's. [#171](https://github.com/Tororoi/Chronicle-Expanded/pull/171)
- The Bomb, Crystal and Atla Gemrons have miniboss loot of their own: Big Bang (from Gallery of Time) or Auto Repair Powder, the Crystal Ring (boosted from Shipwreck) or Powerup Powder, and the Atlamillia Sword (from Shipwreck) or a Turquoise. [#183](https://github.com/Tororoi/Chronicle-Expanded/pull/183)
- Steve comments on every monster of a randomized floor, including ones from other dungeons, with his own lines about them. [#160](https://github.com/Tororoi/Chronicle-Expanded/pull/160)
- Enemy poison lasts 30 seconds, and each tick's damage scales with the monster's resistance. [#7](https://github.com/Tororoi/Chronicle-Expanded/pull/7)
- New Faster Enemies option (Options, Expanded page): monsters move and act twice as fast. Each attack still slows to its normal speed for the moment it can hit, so its blows land as before. [#151](https://github.com/Tororoi/Chronicle-Expanded/pull/151)
- New Harder Enemy AI option (Options, Expanded page): a defeated monster that has a get-up animation, such as a Skeleton Soldier, the Days of the Week or the card soldiers, gets back up about 30% of the time with part of its life, and the undead that already could (Mummy, Master Jacket, Gacious, Horn Head, Silver Gear) do so about 45% of the time. [#152](https://github.com/Tororoi/Chronicle-Expanded/pull/152)
- Minibosses: most floors, back floors included, have a miniboss, more likely one of the kind of monster there are the most of; a second one is rare, and never of the same kind. A miniboss is half again as big, with three times the life and ABS and four times the money, half again the defense and damage (shots too), and a longer reach. It spots you and attacks from farther away, takes a tenth of the damage of thrown items, and always drops something. One you leave alive is still there when you come back from the back floor, or go back down to it; one you defeat stays gone. [#153](https://github.com/Tororoi/Chronicle-Expanded/pull/153)
- Miniboss loot: instead of money, a miniboss drops an item tied to its kind of monster, from [its kind's loot](expanded/docs/miniboss-loot.md): its rare drop 5% of the time and its common drop 30%. Stronger items come only from the dungeon they belong to onward (an item marked with a dungeon drops from there on), and some give way to a better one in a later dungeon (→). Otherwise, or when none of its items drops in this dungeon yet, it drops the back floor's key (35%), one of the dungeon's weapons, a gem or attachment, or a powder, Stamina Drink or Dran's Feather. Weapons marked ★ come boosted: once picked up, they have the stats listed there. [#154](https://github.com/Tororoi/Chronicle-Expanded/pull/154)

## Towns

- Play as any ally in town: commit them in the party menu with the dungeon's change sequence, and they walk with full animations. [#9](https://github.com/Tororoi/Chronicle-Expanded/pull/9), [#10](https://github.com/Tororoi/Chronicle-Expanded/pull/10), [#11](https://github.com/Tororoi/Chronicle-Expanded/pull/11), [#12](https://github.com/Tororoi/Chronicle-Expanded/pull/12), [#13](https://github.com/Tororoi/Chronicle-Expanded/pull/13)
- Each ally takes ladders their own way: Xiao leaps, Goro hops, Ruby floats, Osmond flies and Ungaga refuses. [#14](https://github.com/Tororoi/Chronicle-Expanded/pull/14)

## Fishing

### From Chronicle

- Fish sizes taper smoothly into each species' maximum instead of piling up at it. [Chronicle#141](https://github.com/TheMoonPeople/Chronicle/pull/141)

## Developer tools

- Enemy roster: the developer menu's "enemy roster" row (typed in) or `--roster 20!,60` lays out these monster kinds on every dungeon floor from the next dungeon visit, in debug mode. [#156](https://github.com/Tororoi/Chronicle-Expanded/pull/156)
- The enemy roster refuses a roster it cannot lay out and says why, naming the kinds to add or drop: the Ice Queen (80) and King's Curse (81, 82) need another kind beside them, since they come only with Ruby or Ungaga in the party. [#182](https://github.com/Tororoi/Chronicle-Expanded/pull/182)
- Staged recordings: `--stage` starts a save's party on a dungeon floor with the member, weapon and monsters a clip needs, `--event-log` writes damage, kills, life, weapon HP and sounds as JSON lines, and `scripts/port/clip.py` records a clip and the same run with Weapon Abilities off side by side ([recording.md](expanded/docs/recording.md)). [#237](https://github.com/Tororoi/Chronicle-Expanded/pull/237)

### From Chronicle

- Developer/boot menu, debug mode from `config.json` or a pad chord, and debug shortcuts under Select+L2. [Chronicle#119](https://github.com/TheMoonPeople/Chronicle/pull/119), [dda4b7f0](https://github.com/TheMoonPeople/Chronicle/commit/dda4b7f0)
- Items and weapons given at start with `--give`. [Chronicle#117](https://github.com/TheMoonPeople/Chronicle/pull/117)
- Text wipe (F9) compares the original picture text with the new runtime text. [Chronicle#174](https://github.com/TheMoonPeople/Chronicle/pull/174)

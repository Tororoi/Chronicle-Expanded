# Expanded's Gemrons

Monsters of Expanded's own, placed by the randomizer only (no floor lays them out): each a monster kind past retail's
167 (`expanded/enemies/species.hpp`) with a record built on a retail one, its stand-in, and a model `dcdata` builds from
the disc at extraction (`expanded/src/data/gemron_models.cpp`). They are the PS2 mod's (DarkCloud-Expanded,
`docs/custom-enemies.md`), whose models the builds here reproduce byte for byte.

## Monster kinds past retail's

`SetupBaseModel` reads a kind's record through `SpeciesRecord` and loads its model from `SpeciesPackFile`. An Expanded
kind is placed only once its model is built (`SpeciesBuilt`): the randomizer's random mix draws past one that is not,
and a staged floor drops it. Its stand-in is where it counts as native for the stat normalization and whose sounds it
makes.

## Bomb Gemron

Kind 167, on Holy Gemron (its stats, script and sounds): weak to fire (150 %) and immune to the rest, 40 ABS, steals a
Bomb, rarely drops Repair Powder. Its name is message 3320 of the dungeon's name bank, added by the language files.

| Part | Where | What |
|---|---|---|
| Model `expanded/monstor/e167a.chr` | `BuildBombGemronModel` | Ice Gemron's pack (the five Gemrons share one rig and sheet layout). Its three gems are the thrown bomb (`dun/item/main_data/bakudan`), scaled to each gem and turned (the big one wick down, forward and to its right; the wing ones wick up, toward it), the bomb's sheet in the bank; the glow overlays dropped. The body sheet takes the look made on the PS2 mod's preview page (`bomb_gemron_look.cpp`: 16 sections and their sliders over a texel map, merged to 256 colours; `SheetLook`), the second sheet the first section's sliders. |
| Self-destruct | same | KEY 14 (a repeat of the attack the script never plays) over 200-218 at 0.15 in both motion files: the guard's return (185-195) reversed in pieces (195-191 over 4 frames, 191-188 over 1.5, 188-185 over 0.75), then the guard loop (170-180) once, a key on every frame, held one frame past its final pose (217). |
| Glow disc | same | The torch glow (`lightling` in Gallery of Time's `fire.img`) in the Fire ramp as an 8-bit picture (`bombglow`, 128 x 128), on a frame of its own under the big bomb (`bombglow__czappba`: a glow sprite's quad, 30 across, 9.4 toward the camera), its scale track growing it from nothing over the death's 105-122 and the self-destruct's 200-217, gone the frame after. |
| Thrown bomb `expanded/effect/bomb_throw.chr` | `BuildBombThrowPack` | Witch Illza's apple shot (`ringo_ex`) drawing the bomb. |
| Shots | `bomb_gemron.cpp` | Entered into its model slot as it is set up. The throw: the apple's flight (0.2, then 1.5 for up to 160 frames), aimed 14 over the player's feet where its contact test looks (Holy's script aims at 7.4), ending in the item bomb's blast at 0.5. The death and self-destruct blast: no flight or life, at 2. Neither hits on its own. A blast's hit radius and shock ring are 25 per unit of its scale (12.5, and 50 for the big one; the engine's 20 and 30) and it throws the player away from it (the engine's along +X), through `ShotEffectBomb`. Their damage is the Gemron's shot on the floor (its normalized stats). |
| Death | same | The death clip (motion 11, 105-125) blows up at 122, once seen running below it (the first steps can still read the last clip's frame), 11 over its feet. |
| Self-destruct | same | Under a quarter of its life with the player within 22, it stops and plays KEY 14; on its final pose it blows up and fades out (no kill, no ABS). It stays hittable: a hit's reaction takes it over, and it may light again 60 steps later. |
| Fuse | same, `frame_tint.cpp` | A spark kept alight at the big bomb's wick (the machine-gun hit's flash, 3 across, its three cells 4 frames each), placed from the unit's own posed bone as it draws (every unit shares the species' frames), walking down the wick as the death or the self-destruct burns it, with a guard spark burst (20 marks at twice the size, scattered 0.75) every 15 frames; out with the blast, the fade or the death loop. The big bomb reddens as it burns, up to 150 more red on the ambient for its own draw (`SetFrameTint`). |
| Randomizer | `randomize.cpp`, `themes.cpp`, `normalize.cpp` | In the random mix's pool, the Bombs Away theme, and the Gemrons' look-alikes after Holy Gemron. |

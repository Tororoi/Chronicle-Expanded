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

## Crystal Gemron

Kind 168, on Holy Gemron (its stats and sounds): weak to ice (150 %) and immune to the rest, 80 ABS, steals Powerup
Powder, rarely drops a Crystal Eyeball. Its name is message 3321.

| Part | Where | What |
|---|---|---|
| Model `expanded/monstor/e168a.chr` | `BuildCrystalGemronModel` | Holy Gemron's pack. The e209 event's crystal ball (`gedit/s34/chara/e209ball_b.chr`: eight shell pieces, ten inner shards, camera-facing glints, beams and rings) grafted under each gem bone, scaled to the gem and turned upright in the pose the death reaches at frame 121; only the big one keeps the glints, beams and rings (`null14`). The gems and their glow shells dropped. The sheets in the look made on the PS2 mod's preview page from a painted sheet (`crystal_gemron_look.cpp`, 36 sections). The eyes: the head's triangles whose texture centroid lies in (0.21-0.35, 0.50-0.61) move to a frame of their own, `eyes`. |
| Death clip | same | The vanilla death (105-125) copied to 200-220 in both motion files, held to 263 (KEY 11 at 0.35, KEY 12 its loop on 263), the ball's shatter (its frames 1-21) from 216, stretched by 0.35 / 0.1, its effects 1.5 times that and its pieces twice. Every grafted track runs from frame 0 to 264 (the engine plays a track only between two keys); the material fades are keyed from the shatter only, so only a dying Gemron writes the species' one material, and the engine skips a track before its first key (`MotionProc`: retail read the track's header as a key there). The effects and shards carry subtree-visibility tracks, out of the draw but while they show. |
| Script `expanded/monstor/e168a.stb` | `BuildCrystalGemronScript` | Holy Gemron's: its death sounds the life sphere's shatter (312, from the Ice Queen's arena sounds, bound wherever it is) as the ball breaks at 216 instead of a cry at 214 the vanilla clip never reached; its AI loop shoots within 140 (was 70) and walks closer to 100 (was 50). The record keeps Holy's script name, so its damage is Holy's; the built script loads in its place (`SpeciesScriptFile`). |
| Shot | `crystal_gemron.cpp`, `ice_shots.cpp` | Holy Gemron's shot on the Ice Queen's arrow (`korinoya`): flying from the moment it is fired at 1.2, steered as the korinoya flies (within 60 degrees of its heading it turns toward the player's chest by a 0.12 chord a step), Freeze (the hit's 65 % roll; the Anti-Freeze Amulet blocks it) and the Ice element, a guardable knockback, for 360 frames. |
| Prison | same | An arrow spent within 20 of the player's chest while he is frozen encases him in the Ice Queen's prison (`kori`), held formed while his ailment lasts, then shattering. A hit on the frozen player ends the freeze (`BreakFreeze`). |
| Eyes | same, `frame_tint.cpp` | 45, 70 and 50 more on the ambient for their own draw. |

## Atla Gemron

Kind 169, on Holy Gemron (its stats and sounds): weak to wind (150 %) and immune to the rest, 40 ABS, steals a Dran's
Feather, rarely drops a Potato Cake. Its name is message 3322.

| Part | Where | What |
|---|---|---|
| Model `expanded/monstor/e169a.chr` | `BuildAtlaGemronModel` | Ice Gemron's pack in the look chosen on the PS2 mod's preview page (`atla_gemron_look.cpp`: every section more saturated and a little darker). A dungeon atla (`atr.mds` in `dun/pack/maindat.pac`, its picture from `dun/etc/atrtx.img`) grafted under the big gem at the gem's size, turned as a real atla is; Toan's Atlamillia lens (`renzu__m` in `c01d.chr`, on its patch of his sheet) on the forehead; the Atlamillia's light (`e503ex_atall`'s glow pair, from the s88 event) on the lens, playing on the Gemron's own frames; fly_light's glows, half desaturated, on the wing gems; and a spawn bone for the shot (`shot0`). The gems and their glow shells dropped. The numbers the preview page fitted to the rig are `atla_gemron_recipe.hpp`. |
| Death clip | same | The vanilla death (105-125) copied to 200-220 in both motion files and held to 229 (KEY 11 at 0.35, KEY 12 its loop). The held atla leaves the draw on 217, the copy of the gem landing (a subtree-visibility track). |
| Shot pack `expanded/effect/atla_s.chr` | `BuildAtlaShotPack` | `e503ex_atall`'s burst (its key 9 tail, frames 221-250 at 0.35) under a root of the shot's own, its flight backed off so the burst stands where it was fired while the shot flies; each mesh out of the draw while it is clear. |
| Script `expanded/monstor/e169a.stb` | `BuildAtlaGemronScript` | Holy Gemron's: the shot fires from `shot0` along the Gemron's facing (the fire frame's hold-fire check jumps to aim cells appended past the end: its own place + its facing x 100, 7.4 up), within 50 (walking closer beyond), every 40-109 frames; close in it backs off 60 % of the time, else shoots (the bite before). |
| Shot | `atla_gemron.cpp` | Holy Gemron's shot (its damage, reaction, element, target and sounds) on the burst, one phase, turned to its aim. It flies 0.65 a frame whatever it meets (the engine's flight is only its aim), its hit growing from 1 to 18 between the burst's frames 224.5 and 233.25 (attack frames 140 and 145 of the script's fire). |
| Atlamillia | same, `frame_tint.cpp` | 50 more on each of the ambient's channels for the lens's own draw. |
| Its atla | same, `atla_drops.cpp` | As its death reaches 200 it places a real atla carrying its drop (below) where the held one will rest, out of the draw. From 217 to 228 the atla moves from the held one's pose, as the unit last drew it, to a real atla's: turned, grown to its own size and eased into place, rising 6 at the middle. It can be picked up once it settles. With every place on the floor holding an atla, the one farthest from the player comes to it instead (its own content); with no atla model on the floor (or no event free), the drop is handed over at once. |
| Drop | `atla_gemron.cpp` | 5 % the Atlamillia Sword (from Sun & Moon Temple on; before it, a weapon's sphere), 15 % its SynthSphere, 10 % the dungeon's featured weapon's (Bone Rapier, Wise Owl Sword, Dusack, Maneater, 7 Branch Sword, Dark Cloud; Demon Shaft has none), else the SynthSphere of a weapon of the tiers the dungeon draws from (its own and every one below; Demon Shaft all), one whose owner has joined. |
| Randomizer | `randomize.cpp` | In the random mix's pool. |

## Atlas carrying a drop

`expanded/dungeon/atla_drops.hpp` places atlas that carry a drop of their own in place of a georama part: a weapon, or
a weapon's SynthSphere. `PlaceAtlaDrop` lays one as `CDungeonMap::SetAtraBoll` does (part -1), or in a collected one's
place once the floor's eight are used; `MoveAtla` takes over its draw (`DrawAtraBoll`) while it moves into place and
keeps its pickup radius at 0 until then. Toan picks it up as any atla: `BtAtraGetShort_Loop` reads no part and saves
nothing for it, and the drop is handed over (`AtlaDropTaken`) and named in place of the part (`AtlaDropNamed`). A weapon
goes to its owner as a chest's does, or its SynthSphere when the owner has no room. A SynthSphere is a sixth of the
weapon's base values (attack, endurance, speed, magic, elements and antis), each rounded up, with all its abilities;
its line, `expanded.atla_drop.sphere` in the language files, is a system message of its own (`SetSystemMessage`). A
new floor forgets them: an atla left behind is lost.

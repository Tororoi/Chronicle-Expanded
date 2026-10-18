# Town models

An ally played in town needs every motion the town plays on the player, but the allies' town
models carry only a few. `dcdata` builds each ally a full set from the disc's own models when it
extracts (README, "Game data built at extraction"), into `<data>/expanded/town/<ally>.chr`. The game
loads one as it loads Toan's `chara/c01d.chr`: `EdLoadMainChara("expanded/town/xiao.chr", "info.cfg", ...)`.
The town ally swap (`kTownAllyBodies` in `town/ally.hpp`) loads an ally's built model when the data has
it, and otherwise the ally's disc model, which lacks most motions, logging once which models are missing.

The code is `expanded/src/data/`: `chr_pack.cpp` reads and rebuilds `.chr` packs and their `.mot`
motions, `town_model.cpp` assembles a model, and `town_allies.cpp` is the table of allies. It is the
Dark Cloud Expanded mod's ISO bake (`IsoPatch/TownModelBakes.cs`) moved to C++, and builds the same
bytes from the same disc files: every ally's model matched the mod's, on the PAL and NTSC discs.

## Motion slots

A model's motions are `KEY start, end, speed` lines in its cfg, windows on the one timeline of its
`.mot`; the town plays them by their order:

| Slot | Motion | Slot | Motion |
|---|---|---|---|
| 0 | idle | 6 | item get, held |
| 1 | run | 7 | damage (never played in town) |
| 2 | walk | 8 | fall |
| 3 | push door | 9 | land |
| 4 | pull door | 10 | double door |
| 5 | item get | 11 on | the ally's own |

## Building a model

An ally (`TownAlly`) names its base `.chr` and one `TownSlot` a KEY. A slot without `src` keeps the
base's frames `[lo, hi]`; one with `src` grafts that model's frames `win` into them. A graft maps the
source's joints to the base's by name (a track's joint is an index into the model's `.mds` frame table,
and rigs of one character number their joints differently), replaces the base's keys in the window in
the body motion and, when both models have one, the shadow motion, and keys every grafted track at the
window's edges so a sparse track does not drift into the next clip. Then, in this order, a slot can:

- `hold`: hold the pose of one frame across the window;
- `root_offset`: move the root's translation, such as back off a door;
- `reverse`: play the window backwards;
- `root_freeze`: set every translation to its value at one frame, removing travel a clip has baked in;
- `pin_nodes`: hold some nodes, such as a held prop, at one frame's pose (body only).

Last, the cfg's KEY lines are replaced with one per slot. An ally can also:

- `root_motion`: graft root and hip channels the base's motion lacks, from the base's bind pose
  outside the grafted windows, and take a source root of another name as the base's;
- `shadow_inject`: copy a shadow model, motion and weights from another model into a base without one,
  and declare them in the cfg;
- `mesh_graft`: append frame nodes the base lacks, with their meshes and bind rows, held at one pose
  outside their source's windows, and with `spin` turn a hub of them.

A pack's records follow one another by their strides, which stay multiples of 16: a `.mds` after a
record grown to another length would otherwise be read out of alignment.

## Xiao

Base `gedit/e01/chara/c04pcat.chr`, the cat model no cutscene animates.

| Slot | Frames | Speed | Motion | From |
|---|---|---|---|---|
| 0 | 10-20 | 0.10 | idle | base |
| 1 | 120-136 | 0.50 | run | `gedit/s86/chara/c04cat.chr` 120-136 |
| 2 | 60-80 | 0.30 | walk | base |
| 3, 4, 10 | 150-159 | 0.20 | doors | `gedit/e01/chara/e04c04cat.chr` 160-169, 2 back |
| 5, 6 | 30-40 | 0.10 | item get: a sit | `gedit/s86/chara/c04cat.chr` 30-40 |
| 7 | 228-278 | 0.30 | ladder refusal: a head shake | `dun/d01/event/e613c04cat.chr` 115-165 |
| 8 | 205-214 | 0.50 | fall: a leap | `gedit/s86/chara/c04cat.chr` 205-214 |
| 9 | 215-227 | 0.36 | land | `gedit/s86/chara/c04cat.chr` 215-227 |
| 11 | 95-105 | 0.40 | ladder jump: ready | `gedit/s86/chara/c04cat.chr` 95-105 |
| 12 | 285-294 | 0.60 | ladder jump: up | `gedit/e01/chara/e04c04cat.chr` 160-169 |
| 13 | 300-320 | 0.30 | walk backwards | the base's 60-80, reversed |

## Goro

Base `gedit/s01/chara/c06p.chr`, with root motion. `e101` and `e102` hold Toan too, so their slots name
the cfgs `e101c06a.cfg` and `e102c06a.cfg`.

| Slot | Frames | Speed | Motion | From |
|---|---|---|---|---|
| 0 | 10-20 | 0.15 | idle | base |
| 1 | 60-80 | 0.40 | run | base |
| 2 | 30-50 | 0.25 | walk | base |
| 3, 4, 10 | 10-20 | 0.15 | doors: the idle | base |
| 5 | 90-100 | 0.30 | item get | `dun/mainchara/c06b.chr` 515-525 |
| 6 | 105-115 | 0.30 | item get, held | `dun/mainchara/c06b.chr` 530-540 |
| 7 | 120-149 | 0.50 | ladder refusal | `dun/mainchara/c06b.chr` 421-450 |
| 8 | 155-175 | 0.30 | fall | `gedit/s01/chara/e101.chr` 75-95 |
| 9 | 180-190 | 0.25 | land | `gedit/s01/chara/e101.chr` 95-105 |
| 11, 12 | 195-215, 220-240 | 0.30 | ladder hops | `gedit/s01/chara/e102.chr` 90-110, 110-130 |
| 13 | 245-255 | 0.25 | the landing reversed | `gedit/s01/chara/e101.chr` 95-105 |

## Ruby

Base `gedit/e03/chara/c05a.chr`, with root motion and the shadow of `gedit/e03/chara/e223c05a.chr`
(`e223c05s.*`), as the base has none.

| Slot | Frames | Speed | Motion | From |
|---|---|---|---|---|
| 0 | 10-20 | 0.15 | idle | `dun/mainchara/c05a.chr` 10-20 |
| 1 | 60-80 | 0.55 | run | `dun/mainchara/c05a.chr` 60-80 |
| 2 | 30-50 | 0.25 | walk | `dun/mainchara/c05a.chr` 30-50 |
| 3, 4, 10 | 90-102 | 0.15 | doors: a pat | `gedit/e03/chara/e223c05a.chr` 100-112, 3 back |
| 5 | 120-130 | 0.30 | item get | `dun/mainchara/c05a.chr` 575-585 |
| 6 | 135-145 | 0.30 | item get, held | `dun/mainchara/c05a.chr` 590-600 |
| 7 | 150-170 | 0.20 | ladder refusal | `dun/mainchara/c05a.chr` 605-625 |
| 8 | 175-195 | 0.10 | fall: a float | `gedit/e03/chara/e228c05a.chr` 10-30 |
| 9 | 10-20 | 0.15 | land: the idle | base |
| 11 | 200-230 | 0.20 | ladder jump | `gedit/e03/chara/e228c05a.chr` 285-315, travel frozen at 200 |
| 12 | 235-245 | 0.10 | ladder jump, held | `gedit/e03/chara/e228c05a.chr` 320-330, travel frozen at 175 |

## Ungaga

Base `gedit/e04/chara/e323_2c10a.chr`, with root motion; its cfg is `e323_2c10a.cfg`.

| Slot | Frames | Speed | Motion | From |
|---|---|---|---|---|
| 0 | 10-20 | 0.10 | idle | base |
| 1 | 60-80 | 0.55 | run | `dun/mainchara/c10b.chr` 60-80 |
| 2 | 30-50 | 0.27 | walk | base |
| 3, 4, 10 | 115-131 | 0.20 | doors: a hand to the chest | base, slot 3 2.5 back |
| 5 | 240-250 | 0.30 | item get | `dun/mainchara/c10b.chr` 630-640 |
| 6 | 255-265 | 0.30 | item get, held | `dun/mainchara/c10b.chr` 643-653 |
| 7 | 270-310 | 0.22 | ladder refusal | `dun/mainchara/c10b.chr` 490-530, `kon_1` and `r_handa` held at 15 |
| 8 | 315 | 0 | fall: one pose | `dun/mainchara/c10b.chr` 296 |
| 9 | 357-358 | 0.20 | land | `dun/mainchara/c10b.chr` 295-296, reversed |

## Osmond

Base `gedit/e05/chara/c18p.chr`, with the propeller of `gedit/s13/chara/e402c18a.chr` (13 nodes) grafted
on at its frame 15, and its hub `tukene1` spun 72 degrees a frame through slots 13 to 15.

| Slot | Frames | Speed | Motion | From |
|---|---|---|---|---|
| 0 | 10-20 | 0.10 | idle | base |
| 1 | 130-150 | 0.75 | run: the walk, sped up | base 30-50 |
| 2 | 30-50 | 0.20 | walk | base |
| 3, 4, 10 | 60-80 | 0.20, 0.30 | doors: the talk | base |
| 5, 6 | 90-92 | 0.10 | item get: the talk's frame 69, held | base |
| 7 | 10-20 | 0.10 | damage: the idle | base |
| 8 | 100-108 | 0.30 | fall | `gedit/e05/chara/e403c18a.chr` 210-218 |
| 9 | 110-125 | 0.30 | land | `gedit/e05/chara/e403c18a.chr` 220-235 |
| 11 | 155-175 | 0.30 | ladder down: a dive | `gedit/e05/chara/e403c18a.chr` 185-205 |
| 12 | 180-235 | 0.25 | ladder up: the propeller unfolds | `gedit/s13/chara/e402c18a.chr` 280-335 |
| 13, 14 | 240-255, 260-270 | 0.25, 0.20 | take off, fly | `gedit/s13/chara/e402c18a.chr` 345-360, 360-370 |
| 15, 16 | 275-290, 295-350 | 0.25, 0.30 | land, fold the propeller | slots 13 and 12, reversed |

## Ladders

Toan's climb is rigged to his body, so an ally in its built model takes a ladder its own way
(`expanded/src/town/ladder.cpp`, the mod's `Town/TownLadder.cs`): the press builds a frame-by-frame
sequence of positions, facings and motions, which `EdMoveChara` plays in place of its own step, with the
camera following at the ladder's angle as Toan's climb has it.

| Ally | Up | Down |
|---|---|---|
| Xiao | walk to the ladder, step back (13), crouch (11), leap (12, then 8), the town lands her | crouch, leap (8) |
| Goro | four hops (11, 12 by turns), the last onto the ledge, crouch (9) | stand and crouch (13, 9), jump (8), crouch (9) |
| Ruby | float (8) up, over the ledge and down onto it | float out and down |
| Ungaga | refuses (7) | refuses (7) |
| Osmond | propeller out (12), take off (13), fly up and over (14), land (15), fold it away (16) | dive (11), fall (8), land (9) |

A ladder whose ends can't be read gets the refusal (none for Osmond), and an ally in its disc model,
which has neither, can't take a ladder.

## Adding an ally

Add a `TownAlly` to `town_allies.cpp` with its output file, base and slots; the mod's
`docs/town-swap-animation-map.md` says why each clip was chosen. `ExpandedData.BuildsWhatTheModsToolsBuild`
(`expanded/tests/data/built_files_test.cpp`) checks every built file, the allies' among them, against the mod's
bytes when `DC_TEST_DATA` names an extracted data directory; add the new ally's file and hash to its table.

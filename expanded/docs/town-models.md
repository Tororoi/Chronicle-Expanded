# Town models

An ally played in town needs every motion the town plays on the player, but the allies' town
models carry only a few. `dcdata` builds each ally a full set from the disc's own models when it
extracts (README, "Game data built at extraction"), into `<data>/expanded/town/<ally>.chr`. The game
loads one as it loads Toan's `chara/c01d.chr`: `EdLoadMainChara("expanded/town/xiao.chr", "info.cfg", ...)`.

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

## Adding an ally

Add a `TownAlly` to `town_allies.cpp` with its output file, base and slots; the mod's
`TownModelBakes.cs` has the configs of all five allies and `docs/town-swap-animation-map.md` why each
clip was chosen. `TownModel.XiaoMatchesTheModsBake` checks Xiao against the mod's bytes when
`DC_TEST_DATA` names an extracted data directory.

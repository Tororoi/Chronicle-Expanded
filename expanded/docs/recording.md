# Recording a feature

A feature's clip is a headless run of the release build: a **stage** puts a save's party on a dungeon floor with
the member, weapon and monsters it needs (or at a town's fishing sign with a bait ready: see [Fishing](#fishing)), an
**input script** plays the feature, an **event log** gives the numbers
for its caption, and the same run with the **Weapon Abilities** option off is the before to its after.
`scripts/port/clip.py` does all of it in one command. A clip's stage and input script belong to that clip, not to the
repository: keep them beside its recordings in `port/run/clips/` (which git ignores) or outside the repository. For
Hercules' Wrath's Air Strike, `air-strike.stage`:

```
# Ungaga on Sun & Moon Temple floor 4, five Mummies set out 100 ahead, walking in.
roster     = 44
dungeon    = 3
floor      = 3
character  = ungaga
weapon     = 356
attack     = 25
seed       = 1
place      = 5
distance   = 100
keep_alive = yes
```

and `air-strike.in`, timed from a dry run's `stage` events (Ungaga out at frame 469, the Mummies placed at 568): lock
on, guard (the decoy, then the five-second charge), step back and aside, let go once primed, and swing.

```
600 128 128 128 255
650 circle 128 128 128 255
656 128 128 128 255
670 r1 128 128 128 255
695 r1 128 255 128 255
820 r1 255 128 128 255
920 r1 128 128 128 255
1020 128 128 128 255
1045 cross 128 128 128 255
1050 128 128 128 255
1520
```

Then, with a release build:

```
cmake --preset macos-arm64-release -DCHRONICLE_EXPANDED=ON
cmake --build port/build/macos-arm64-release --target darkcloud
python3 scripts/port/clip.py air-strike --compare --stage air-strike.stage --input air-strike.in \
    --from 600 --length 920 --data <game data> --save <a save folder with a late-game save in slot 1>
```

That writes `port/run/clips/air-strike.mp4` (the run on the left, the one with Weapon Abilities off on the right, each
one's sound in its own ear), `air-strike.events.jsonl`, `air-strike.off.events.jsonl` and `air-strike.sheet.png` (a
frame a second), in about half a minute. A debug build is about ten times slower, most of it in writing the frames.
Recordings go in `port/run/clips/` or outside the repository: `clip.py` and `record_clip.py` refuse to write a video,
a kept folder or a log anywhere in the repository that git does not ignore, so a recording is never committed.

## Stages

`--stage FILE` (or `DC_STAGE`) reads `key = value` lines (`#` starts a comment). With a `dungeon` the run starts in
it at once, as `--jump dungeon:<n>` does, with the game of the `--save` folder's save `slot`: no title, town or
menus. With a `town` it starts in that town with the save's game, as the title's Continue goes on there, with debug
mode off. Every key is optional; `roster` to `keep_alive` act only in a dungeon, and `fishing_spot` and `bait` need a
`town`.

| Key | Value |
|---|---|
| `roster` | the floor's monster kinds, as `--roster` takes them (`44`, `20!,60`) |
| `slot` | the save loaded, 1 for `saves/1` (default 1) |
| `dungeon` | 0-6: start in this dungeon |
| `floor` | the floor its entrance takes, by the floor menu's index (0 is floor 1) |
| `character` | the member brought out once the floor is in play: `toan`, `xiao`, `goro`, `ruby`, `ungaga`, `osmond` or 0-5 |
| `weapon` | that member's weapon (item number, `ps2/include/itemdata.hpp`), at its maximum attack |
| `attack` | that weapon's attack instead |
| `sphere` | Xiao's Super Steve, carrying a SynthSphere of this weapon |
| `seed` | Expanded's random numbers (the game's own are the same run to run already) |
| `place` | how many monsters in play are set out in front of the member; the rest are sent 2000 away |
| `distance`, `spread`, `angle` | how far in front (default 60), the radians between them (0.35) and turned from the member's facing (0) |
| `place_kind` | only monsters of this kind |
| `place_after` | monster steps after the member is out (default 100) |
| `big_room` | `yes`: walk the member to the middle of the nearest big room first |
| `circle` | 0-9: the floor's first magic circle set to this kind (`ps2/include/dungeonmap.hpp`) and moved 40 in front of the member, as the monsters are set out, for the member to walk onto |
| `keep_alive` | `yes`: the party's life kept full |
| `town` | start in this map: `norune`, `matataki`, `queens`, `muska_lacka`, `yellow_drops` (4, the georama; the pond's west bank is the dome, 23), `brownboo` (14), `east_harbor` (19), or its MapNo: 0-4 the towns (`gedit/e01`-`e05`), 11-52 the other maps (`s01`-`s42`) |
| `fishing_spot` | stand at the town's fishing sign of this number (0 its first), ready to read it |
| `bait` | this bait (item number) given when the bag lacks it, and put on the hook as fishing starts |
| `time` | the time of day the save starts at: `morning`, `day` (or `afternoon`), `evening` (or `dusk`), `night`, or the game's clock, 0 to 12 |
| `give` | item and weapon numbers given as the town or dungeon starts, as `--give` takes them (which takes their place) |

A run is the same every time for the same stage, inputs and build. Time the input script from the stage's own
events: a dry run (`darkcloud --headless --frames 700 --data D --save S --fast-load --stage F --event-log dry.jsonl`)
logs `stage` events for when the member is out, has walked and the monsters and the circle are placed, or, in a
town, for when it is in play, the player stands at the sign and the bait is on. The game's frame numbers count the
main loop, as `--input`, `--record` and `--from` do.

## Fishing

A town stage runs the game's fishing as it is, debug mode off: fish notice a bait from its own radius, and a hooked
fish not struck in time gets away. With a `fishing_spot` the player stands on the sign's event point, facing the way
they came from the town's entrance, the sign's mark showing: cross reads the sign, whose menu starts on Fish. A town's
fishing signs are its event points whose script loads the fishing data (`_LOAD_FISHING_DATA`), counted in the order
the town lists them, so a sign a feature adds is found the same way. The vanilla ones:

| Spot | `town` | `fishing_spot` |
|---|---|---|
| Norune's pond | `norune` | 0 |
| Matataki's Peanut Pond | `matataki` | 0 |
| Matataki's waterfall | `matataki` | 1 |
| Muska Lacka's oasis | `muska_lacka` | 0 |
| East Harbor's dock | `east_harbor` | 0 |

Fishing starts with no bait, as it does after the sign. A stage's `bait` is put on the way the square button and the
bait picker would: the stage asks for the bait change as fishing starts and picks its bait, out of the bag. A fish
takes a bait it likes ([fish.cpp](../../port/src/game/fish.cpp)'s `food_affinity`): Norune's Gummies and Nonkies
like Mimi (197), not Evy. Morning, day, evening and night by name start an hour into that part of the day (each
lasts about 5,400 frames of walking at normal time speed). For Norune's pond, `norune-catch.stage`:

```
# Toan at Norune's fishing sign on a morning, with a Mimi to put on.
town         = norune
fishing_spot = 0
bait         = 197
time         = morning
```

and `norune-catch.in`, timed from a dry run's events (at the sign at frame 6, fishing at 164 with the Mimi on at
169, a Nonky hooked at 1035, caught at 1260): read the sign, choose Fish, cast, strike once it is hooked and turn the
catch's message's two pages.

```
60 cross
65
130 cross
135
200 cross
205
1045 cross
1050
1360 cross
1365
1560 cross
1565
1800
```

```
python3 scripts/port/clip.py norune-catch --stage norune-catch.stage --input norune-catch.in --from 30 --length 1770 \
    --data <game data> --save <a save folder with a late-game save in slot 1>
```

The Nonky (125 cm, 15 FP) is caught at frame 1260 and the rod is ready again at 1755. A run is the same every time,
so a strike that comes too late in one comes too late in all: time it from the dry run's `hooked`.

## Event log

`--event-log FILE` (or `DC_EVENT_LOG`) writes one JSON object a line, the frame and the event first:

```
{"frame":1194,"event":"damage","monster":0,"kind":44,"amount":136,"life":14,"owner":1,"attack":0}
```

| Event | Fields |
|---|---|
| `floor` | `dungeon`, `floor` as a floor's monsters load |
| `character` | `chara`, the member out, as it changes |
| `stage` | `step`: `out`, `walked`, `placed`, `circle`; in a town `town` (in play), `at_sign` and `bait` (`item`, on the hook) |
| `fishing` | `step`: `start` (fishing, with its `bait`, -1 for none), `ready` (the rod ready to cast again), `cast`, `float` (in the water), `reeled` (in again), each with the `bait`; `bite` (a nibble), `let_go`, `hooked`, `escaped` (unstruck), `missed` (struck in a nibble), `fight` (struck when hooked) and `landed`, each with the `fish` (0-5) and its `kind`; `caught` (`kind`, `size` in cm, `points`, the `total` FP); `bait` (`bait`, put on), `bait_lost` (`bait`); `quit` |
| `damage` | `monster`, `kind`, `amount`, `life` left, for a weapon's hit, and the hit's `owner` (the member) and `attack` (which of theirs) |
| `kill` | `monster`, `kind`, `by` (the member the kill is credited to; the ABS goes to their weapon when they are out) |
| `life` | `chara`, `from`, `to`, the member's life as it changes |
| `whp` | `chara`, `weapon`, `from`, `to`, its weapon HP as it changes |
| `sound` | `program`, `note`, an Expanded sound played |
| `confuse` | `step`: `confused`, `target` (`target` the monster it goes after, -1 the player), `senses` (its confusion over), each with its `monster`; `hit` (`by`, `monster`, `amount`, `life` left), a confused monster's hit on another |
| `flame_gun` | `step`: Super Steve's flame `lit`, `out` (let go), `empty` (the gauge run out) and the run-out gauge `refilled`, each with the action `gauge` |
| `babel` | `step`: Babel's Spear's `summoned` (`target`, `x`, `z`), `struck`, `spike` or `hand` (`monster`, `damage` before defense), `risen`, `gone` |
| `tide` | `step`: `level` (`level`, the Queens canal's water as it shows, on entering Queens and when it changes), `evict` (a player caught down in the canal by the rising tide, sent to East Harbor's dock) |

A feature can add its own with `expanded::LogEvent` (`expanded/debug/event_log.hpp`).

## The before: Weapon Abilities off

`--compare` records a second run at the same time with the Expanded page's Weapon Abilities option off
(`"expanded": {"weapon_abilities": false}` in its save copy's `config.json`). Off, `WeaponWielded`, `WeaponActive`,
`HeldWeapon`, `CarriesWeapon`, `SphereWeapon` and the ownership tests answer no weapon, so every Expanded weapon effect stays off, the
Mirage's decoy and Super Steve's spheres included, and the run plays as the game's own weapons do. What a weapon was
given when it was made or built up (an ability Expanded adds, the Steel Slingshot's endurance, the Seven Branch
Sword's build-ups) stays with it.

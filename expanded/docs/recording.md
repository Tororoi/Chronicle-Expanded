# Recording a feature

A feature's clip is a headless run of the release build: a **stage** puts a save's party on a dungeon floor with
the member, weapon and monsters it needs, an **input script** plays the feature, an **event log** gives the numbers
for its caption, and **plain weapons** record the same run without Expanded's weapon effects for a before and after.
`scripts/port/clip.py` does all of it in one command:

```
cmake --preset macos-arm64-release -DCHRONICLE_EXPANDED=ON
cmake --build port/build/macos-arm64-release --target darkcloud
python3 scripts/port/clip.py air-strike --compare \
    --stage scripts/port/stages/hercules-air-strike.stage --input scripts/port/stages/hercules-air-strike.in \
    --from 600 --length 920 --data <game data> --save <a save folder with a late-game save in slot 1>
```

That writes `port/run/clips/air-strike.mp4` (the run on the left, the plain run on the right, each one's sound in its
own ear), `air-strike.events.jsonl`, `air-strike.plain.events.jsonl` and `air-strike.sheet.png` (a frame a second),
in about half a minute. Record with a release build: a debug build is about ten times slower, most of it in writing
the frames. `clip.py` refuses to write anything git would not ignore, and `.gitignore` ignores videos, sounds,
frames, contact sheets and event logs wherever they are, so a recording is never committed.

## Stages

`--stage FILE` (or `DC_STAGE`) reads `key = value` lines (`#` starts a comment). With a `dungeon` the run starts in
it at once, as `--jump dungeon:<n>` does, with the game of the `--save` folder's save `slot`: no title, town or
menus. Every key is optional.

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
| `keep_alive` | `yes`: the party's life kept full |

A run is the same every time for the same stage, inputs and build. Time the input script from the stage's own
events: a dry run (`darkcloud --headless --frames 700 --data D --save S --fast-load --stage F --event-log dry.jsonl`)
logs `stage` events for when the member is out, has walked and the monsters are placed. The game's frame numbers
count the main loop, as `--input`, `--record` and `--from` do.

## Event log

`--event-log FILE` (or `DC_EVENT_LOG`) writes one JSON object a line, the frame and the event first:

```
{"frame":1194,"event":"damage","monster":0,"kind":44,"amount":136,"life":14}
```

| Event | Fields |
|---|---|
| `floor` | `dungeon`, `floor` as a floor's monsters load |
| `character` | `chara`, the member out, as it changes |
| `stage` | `step`: `out`, `walked`, `placed` |
| `damage` | `monster`, `kind`, `amount`, `life` left, for a weapon's hit |
| `kill` | `monster`, `kind` |
| `life` | `chara`, `from`, `to`, the member's life as it changes |
| `whp` | `chara`, `weapon`, `from`, `to`, its weapon HP as it changes |
| `sound` | `program`, `note`, an Expanded sound played |

A feature can add its own with `expanded::LogEvent` (`expanded/debug/event_log.hpp`).

## Plain weapons

`--plain-weapons` (or `DC_PLAIN_WEAPONS=1`) has `WeaponWielded`, `WeaponActive`, `HeldWeapon`, `SphereWeapon` and
the ownership tests answer no weapon, so every Expanded weapon effect that asks them stays off, the Mirage's decoy
and Super Steve's spheres included, and the run plays as the game's own weapons do. A weapon's upgrades in the menus
(the Steel Slingshot's endurance, the Seven Branch Sword's build-ups) are untouched.

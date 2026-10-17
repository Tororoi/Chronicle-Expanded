# Bosses on regular floors

A boss a staged roster (such as the debug `--roster`) places on a regular floor plays there
only when it is supported (`BossSupported`, `expanded/src/enemies/bosses.cpp`): Minotaur Joe for now. Any
other boss plays as on its arena.

A supported boss on a regular floor:

- keeps the spot the floor gives it: `_SET_POSITION` and `_SET_MONSTOR_POS` are ignored while it sets up
  (state 1) and once beaten;
- raises no event: `_RUN_SCRIPT` and `_STATUS_SET_EVENT` are dropped, so no arena intro or victory scene
  starts;
- dies natively instead of through its death label.

## Minotaur Joe's death

His death label (120 in `dun/monstor/c16a.stb`) stops him, makes him invincible for 100 steps, cuts sound
1798 and runs the defeat scene (510). On a regular floor `BossDeathStep` does the first three, then:

1. plays KEY 11, the stagger and fall, once, stopping on its last frame;
2. switches to KEY 12, the death loop, and fades him out over 96 steps;
3. sets his state to -1 and lowers the live count, as `_STATUS_SET_DEAD` does, so his ABS is granted.

Joe is a no-lock-on kind, so like on his arena he drops no item and never carries the floor's key.

## The built model

`dcdata extract` and `dcdata expanded` build `expanded/monstor/c16a.chr`
(`expanded/src/data/boss_model.cpp`) from:

- `dun/monstor/c16a.chr`, the battle body, whose KEYs 0-10, mesh, weights and textures are kept as they are;
- `gedit/s90/chara/e406.chr`, the Moon Sea defeat scene: its `e406c16a.cfg` KEY 3 (ふらふら～倒れる,
  frames 90-140, speed 0.2) is grafted by joint name onto the body and the shadow, past their last frame
  (370-420), keyed at the window's edges.

| KEY | Frames | Speed | Motion |
|---|---|---|---|
| 11 | 370-420 | 0.2 | stagger and fall |
| 12 | 420-420 | 0.0 | death loop, the fall's last frame |

`SetupBaseModel` loads it for Joe wherever he appears, his arena included: the extra KEYs are played only
by the regular-floor death. Without it (data extracted before), Joe loads his disc pack, falls with his
disc death (KEY 9) and fades, and the game logs once that `dcdata expanded <data>` adds the new one.

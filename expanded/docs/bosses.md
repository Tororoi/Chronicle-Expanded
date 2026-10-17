# Bosses on regular floors

A boss a staged roster (such as the debug `--roster`) places on a regular floor plays there
only when it is supported (`BossSupported`, `expanded/src/enemies/bosses.cpp`): Minotaur Joe and Master
Utan for now. Any other boss plays as on its arena.

A supported boss on a regular floor:

- keeps the spot the floor gives it: `_SET_POSITION` and `_SET_MONSTOR_POS` are ignored while it sets up
  (state 1) and once beaten;
- raises no event: `_RUN_SCRIPT` and `_STATUS_SET_EVENT` are dropped, so no arena intro or victory scene
  starts;
- has its sounds: the floor binds the sound set its arena's event loads (`expanded/src/enemies/sounds.cpp`);
- dies natively instead of through its death label.

## Minotaur Joe's death

His death label (120 in `dun/monstor/c16a.stb`) stops him, makes him invincible for 100 steps, cuts sound
1798 and runs the defeat scene (510). On a regular floor `BossDeathStep` does the first two, then:

1. plays KEY 11, the stagger and fall, once, stopping on its last frame;
2. switches to KEY 12, the death loop, and fades him out over 96 steps;
3. sets his state to -1 and lowers the live count, as `_STATUS_SET_DEAD` does, so his ABS is granted.

## Joe's sounds

His ids 1798-1813 are program 116 (note id - 1778) in `se_info`, whose bank only `sound/set/snd204.snd`
carries: event 405 loads that set (`_LOAD_SOUND_SET(204)`) for his arena, and no dungeon set has it. On a
regular floor the set is bound as an extra monster bank, so he sounds as on his arena, except for 1798, the
crowd cheering that his script plays when he is hit, which `_SET_SND_NOW` drops there.

Joe is a no-lock-on kind, so like on his arena he drops no item and never carries the floor's key.

## Master Utan

His setup label (1 in `dun/monstor/c14a.stb`) puts him at `_SET_POSITION(0, 60, 100)`: his arena (the `gedit/s96`
scene) stands him on raised ground 60 up near its origin, which on a regular floor is the map's corner, in the
air. Kept where the floor places him, he stands on its ground; the rest of his AI moves relative to the player,
and the floor's other monsters keep their own spots. The boulder he throws on his arena is the arena scene's
(`iwa.chr`), not his, so he fights with his swipes, charge and seed spray.

His death label (120) stops him, makes him invincible for 100 steps, holds motion 13 (死亡, frames 360-385) on
its last frame and runs the defeat scene. On a regular floor `BossDeathStep` plays motion 13 once, fades him
out on its last frame (he has no death loop) and lets him die as Joe does.

His ids 1741-1759 are program 113, only in `sound/set/snd201.snd`, the set his arena's event loads
(`_LOAD_SOUND_SET(201)`); it is bound like Joe's.

## The built model

`dcdata extract` and `dcdata expanded` build `expanded/monstor/c16a.chr`
(`expanded/src/data/boss_model.cpp`) from:

- `dun/monstor/c16a.chr`, the battle body, whose KEYs 0-10, mesh, weights and textures are kept as they are;
- `gedit/s90/chara/e406.chr`, the Moon Sea defeat scene: its `e406c16a.cfg` KEY 3 (ふらふら～倒れる,
  frames 90-140, speed 0.2) is grafted by joint name onto the body and the shadow, past their last frame
  (370-420), keyed at the window's edges. The cfg's sound EVENT lines in the window, his stomps (1800 at
  105, 1799 and 1800 at 121), come along shifted to 385 and 401, with 1809, the roar and collapse, at 385:
  event 406 plays it 75 ticks after starting the fall, frame 105 at its speed.

| KEY | Frames | Speed | Motion |
|---|---|---|---|
| 11 | 370-420 | 0.2 | stagger and fall |
| 12 | 420-420 | 0.0 | death loop, the fall's last frame |

`SetupBaseModel` loads it for Joe wherever he appears, his arena included: the extra KEYs are played only
by the regular-floor death. Without it (data extracted before), Joe loads his disc pack, falls with his
disc death (KEY 9) and fades, and the game logs once that `dcdata expanded <data>` adds the new one.

# Bosses on regular floors

A boss a staged roster (such as the debug `--roster`) places on a regular floor plays there
only when it is supported (`BossSupported`, `expanded/src/enemies/bosses.cpp`): Minotaur Joe, Master Utan,
the Ice Queen, King's Curse and the Black Knight for now. Any other boss plays as on its arena.

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

## Casts

A boss fought as several monsters whose scripts talk to each other is a cast (`expanded/src/enemies/boss_casts.cpp`):
the Ice Queen and her barrier, King's Curse's coffin and king, the Black Knight's mount and the Knight. On their
floors they talk through globals 0-9 and find each other by monster number (the cast's first is monster 0, its
second monster 1). Off them:

- a roster with any of a cast keeps all of it, spawned once, while the party member the fight needs is in
  the party (the one who can hurt the boss: Ruby for the Queen, Ungaga for the king); otherwise, or with no
  room for the rest, none of it. A member that may come alone (the Black Knight) stays alone in a roster
  without the rest, starting where the rest would leave it;
- the cast's scripts use a mailbox of their own instead of the globals (`_SET_GLOBAL_INT`, `_GET_GLOBAL_INT`),
  and a script's monster 0 or 1 is the cast's in every opcode taking a monster's number (`_GET_MONSTOR_*`,
  `_SET_MONSTOR_*`, `_SET_REFERENCE`, `_DEL_REFERENCE`); a partner that did not come is the member itself;
- a member attached to its partner (`_SET_REFERENCE`) sits on the anchor (its own spot held its place on the
  floor), is attached once the partner spawns when it is still to come, and wakes by where the partner is
  instead of its offset on it;
- setting up, its members keep the floor's spots (`_SET_POSITION`);
- one member follows the other: while dormant it waits by it, to wake with it, and its script's hiding place
  is taken by it too;
- the boss's partner is no kill (no kill count, the live count kept), and leaves with the boss when it falls.

## The Ice Queen

On her floor (Shipwreck's boss floor) the Queen (`c13a`) fights with eight companions, each a monster with its
own script: her barrier (`c13_baria`), the ice arrow (`c13_korinoya`), the prison (`c13_kori`), the meteor
(`c13_i_meteo`), the aura (`c13_reiki`) and three tornadoes (`c13_i_tatumaki`), the companions finding her as
monster 0. Off her floor her barrier is her cast's other member, following her at chest height; breaking it
keeps it down until she raises it. The other companions are shot effects of their own packs
(`expanded/src/enemies/ice_queen.cpp`), loaded with her, fired by `IceQueenStep` for her script's requests and
answering in the mailbox as their scripts did:

| Companion | Shot | As the script did |
|---|---|---|
| Ice arrow (`korinoya.chr`) | 15 ahead of her toward the player; 69, freeze 0x100 (65%, the Anti-Freeze Amulet blocks it), 360 steps | turns toward the player's chest within 60 degrees ahead, 0.12 a step at 1.2 (the Crystal Gemron's tuning; retail 1.6 and always freezing) |
| Prison (`kori.chr`) | at the frozen player's feet, no hit | forms, holds its last frame while the player is afflicted, shatters at 0.5 |
| Meteor (`i_meteo.chr`) | forms 50 above the player (0.1), falls 1.6; 69, radius 30 | lands on the ground or the player |
| Aura (`b3_reiki.chr`) | at her feet (0.2); 74, radius 60, throws | one blast |
| Tornadoes (`i_tatumaki.chr`, three) | 30-110 from the player (0.2); 74, radius 20, throws | gone once they hit, or when the player is more than 100 away |

The arrow's freeze is checked two steps after it lands, once its hit has reached the player. She dies with
her death (motion 11) and its loop (12) like Joe; her shots end and her barrier leaves with her. Her sounds
(program 114) are in `sound/set/snd202.snd`, bound like Joe's.

## King's Curse

On his floor (the Sun and Moon Temple's last) King's Curse is two monsters: the coffin (`c15a`, 2000 life,
monster 0) and the king (`c15b`, monster 1), who fights. The king takes no damage; Ungaga's hit (damage id 42)
blows him away, and the coffin comes onto him and opens to be hit until it closes 360 steps later and he
forms again. The coffin's defeat is his. Off his floor, as a cast with the coffin following the king:

- the coffin's hiding place (`_SET_POSITION(0, 0, -500)`, out of his arena) is taken 500 under the king, and a
  dormant coffin waits there, to wake with him;
- the coffin falls with its death (motion 5) and its loop (6) and fades; the king leaves with it;
- the king is drawn as his smoke (`InitBee`, `DrawBee`: `c15a03` cells along his frames, which are hidden),
  which retail ties to his floor and monster 1 (`BossSmokes`);
- their sounds (program 115) are in `sound/set/snd203.snd`, bound like Joe's.

## The Black Knight

On his floor (the Demon Shaft's last) the Black Knight (`c21a`, 40000 life, monster 1) rides his mount (`c22a`,
50000 life, monster 0): attached to its `g_center_1_2_1` frame, his riding motions played by it, firing when
it sets g1, hurt by nothing. When the mount's life runs out its death label is the change of phase: both fall
(motion 7), he gets up (14), the mount fades, lets him go where it stood and leaves 500 away, and g0 1 wakes him
on foot. His armour (`dcol00`, `dcol01`) guards; his body is hurt by anyone, so he needs no one in the party.

Off his floor the mount is his cast's partner: its raised spot (`_SET_POSITION(0, 60, 100)`) is dropped, its
fall still hands the fight to him (no kill), and it leaves when he falls. A roster with the mount brings him;
one with him alone keeps him alone, starting dismounted (g0 1) and standing (motion 15), his monster 0 himself.
He dies with his death (motion 7) and its loop (8). Their sounds (program 121) are in `sound/set/snd207.snd`.

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

# Quests

Expanded's quests are the port's quests (`docs/QUEST_FRAMEWORK.md`): each a `quest::Definition` registered after the
port's own (`expanded::RegisterQuests`, called from `RegisterAllQuests`), with its progress in the save's `quests.json`.
Their code is `expanded/src/quests/`, their text `expanded.quest.*` in `expanded/lang/` (the four PAL languages; the
English is in the code), checked by `QuestText.EveryLanguageHasEveryLine`.

## Talk menus

A giver's quest is a choice on their talk menu (the dialog framework's: Talk, the added choices, Never mind), there
only while it is theirs to give or under way (`Offers`, `quests/givers.hpp`):

| Choice | While |
|---|---|
| Got anything for me? (`expanded.quest.offer`) | the giver has a quest to give: its first word, or the next one |
| About the quest. (`expanded.quest.about`) | the giver's quest is under way, or done and waiting on its reward |

A villager with neither has no choice, and with nothing else added keeps retail's conversation as it was.

## Givers

A giver is a villager by town (`MapNo`) and number: the dialog framework's `npc_id`, which is the villager's `PEOPLE2`
line in the town's `gedit/<town>/mapinfo.cfg`, from 0 (each line's comment names them in Japanese). The mod named
them by model (`p01a.chr`, two ASCII digits read from memory); `PEOPLE2 "p01a"` is Norune's line 3, so its Macho is
3. A villager's name tag is `gedit/system/editsys` message 1000 + 100 × town + number (Yellow Drops' 3300 + number).

| Giver | Town (`MapNo`) | Number | Model | Quests |
|---|---|---|---|---|
| Macho | Norune (0) | 3 | p01a | Monster Slayer |
| Gob | Matataki (1) | 5 | p25a | Monster Slayer |
| Jake | Queens (2) | 8 | p43a | Monster Slayer |
| Chief Bonka | Muska Lacka (3) | 0 | p48a | Monster Slayer |

## Monster Slayer

`quests/monster_slayer.cpp`, one quest a giver (`slayer_macho`, `slayer_gob`, `slayer_jake`, `slayer_bonka`):

    unheard  -- asked -->  active  -- the last kill -->  complete  -- the reward -->  idle  -- asked -->  active

- Asking sets a target: one of `kSlayerTargets`' four kinds of a dungeon the player has been into (the Divine Beast
  Cave to the Gallery of Time; the cave when none), 8 to 18 of them. The first ask tells what the quests are first.
  Macho's first is Dashers in the Divine Beast Cave. A kind another giver's quest under way wants is rolled again
  (up to 50 times).
- Vars: `dungeon`, `monster` (the `Monster` number), `left`, `total`, `given` (quests set so far).
- A kill counts in `MonsterKilled` (`MonsterSlayerKill`): of the target or its stronger copy (`BaseOf`), in its
  dungeon (`selectMapNo`), while it is active. The last one shows a notice at the bottom of the screen.
- The reward is a Powerup Powder (`IGame::GiveItem`), shown as the town's events show an item received; a pack with
  no room keeps the quest complete until there is.

## Trying them

A town stage with `villager` stands the player in front of a giver (`expanded/docs/recording.md`): Macho with
`town = norune` and `villager = 3`. A save folder's `saves/<slot>/quests.json` sets a quest's stage and vars to start
from, for instance two Dashers from done:

```json
{"version": 1, "quests": {"slayer_macho": {"stage": "active", "vars": {"dungeon": 0, "monster": 3, "left": 2, "total": 12, "given": 1}}}}
```

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
| Pike (PAL: Dike) | Norune (0) | 5 | p06a | Fishing |
| Pao | Matataki (1) | 7 | p24a | Fishing |
| Sam | Queens (2) | 11 | p34a | Fishing (at East Harbor) |
| Devia | Muska Lacka (3) | 5 | p53a | Fishing |
| Laura | Norune (0) | 10 | p04a | Item hunt |
| Ro (French: Ed) | Matataki (1) | 10 | p21a | Item hunt |
| Phil | Queens (2) | 12 | p42a | Item hunt |
| Zabo | Muska Lacka (3) | 2 | p50a | Item hunt |
| Ledan | Yellow Drops (23) | 0 | p75a | The puzzle |
| Samba | Yellow Drops (23) | 1 | p74a | The challenge |

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

## Fishing

`quests/fishing_quests.cpp`, one quest a giver (`fishing_pike`, `fishing_pao`, `fishing_sam`, `fishing_devia`), with
Monster Slayer's stages. Half the time a count quest, half a size quest:

| Giver | Waters (spots) | Count quest's kinds | Size window (least rolled; width) | Count points per fish | Size bonus |
|---|---|---|---|---|---|
| Pike | Norune pond (0) | Niler, Gummy, Nonky, Gobbler | 80–140; 5 | 35–65 | 0 |
| Pao | Peanut Pond (1) or the waterfall (2) | pond: Baku Baku, Gobbler, Tarton, Umadakara; waterfall: Baku Baku, Nonky, Niler, Mardan and Baron Garayan | 80–140; 5 | 43–73, a Garayan ×2 | 10 |
| Sam | East Harbor (3) | Bobo, Kaji, Gobbler, Bon, Hamahama | 90–160; 10 | 51–81 | 20 |
| Devia | the oasis (4) | Negie, Den, Heela, Mardan and Baron Garayan | 100–180; 5 | 60–90 | 30 |

- The kinds are the spots' pools as Expanded has them (`PoolFish`: the waterfall's Niler, East Harbor's Gobbler). A
  Garayan is rolled again once and asked for alone; the rest two at a time. Pao's count quests are at one of his two
  waters, half the time each; the Baku Baku, in both, and his size quests are anywhere in Matataki.
- Vars: `size` (0 a count quest), `fish`, `left`, `total`, `least`, `most`, `place` (`FishPlace`), and Sam's `done`
  (quests completed) and `shown`.
- A catch counts at `ED_FISHING_MES_EVALUATE` (`FishingQuestCatch`), at the quest's waters (`FishingSpotNow`).
- A count quest pays its points per fish × the count; a size quest a roll from its window's most − 30 to its most − 1,
  and the waters' bonus (`IGame::EarnFishingPoints`).
- Sam's third completed quest also sets `shown`: from then on `FishingLoadFish` draws East Harbor's fish and the line
  under the water (`EastHarborFishShown`), where the original game hides them. The original game draws East Harbor's
  sea with the ground, before the fish; Expanded draws it with the water, after them, as every other fishing water is
  drawn (`expanded/town/sea.hpp`).

## Item hunts

`quests/item_hunts.cpp`, one quest a giver (`hunt_laura`, `hunt_ro`, `hunt_phil`, `hunt_zabo`):

    unheard  -- the rumour -->  heard  -- the item had -->  found  -- the giver told -->  done

| Giver | Dungeon | Item |
|---|---|---|
| Laura | Divine Beast Cave | Medusa Powder (171) |
| Ro | Wise Owl Forest | Warp Powder (173) |
| Phil | Shipwreck | Shell Ring (243) |
| Zabo | Sun & Moon Temple | Hardening Powder (172) |

- While a hunt is `heard` and the player has not the item (carried or stored: `HasItemAnywhere`), each back floor of
  its dungeon has a one-in-three chance of it in its first small chest (`ItemHuntChests`, from `FloorChestsPlaced`
  at the end of `CDungeonMap::buildEventData`; the back floor is the one built without atla, `place_atla` 0).
- `heard` moves to `found` by a rule once the item is had, so the giver's next word is the found one; the hunt is
  then done and the giver's choice goes. The item is the reward.

## Yellow Drops

`quests/yellow_drops.cpp`. Both wait on the whole party (six; the Yellow Drops villagers' Boss, Osmond, last):
`hidden` moves to `offered` by a rule.

- **The puzzle** (`yd_crystal`: `hidden`, `offered`, `asked`, `done`): Ledan's hint is what Blue Terra's people
  needed; the answer is the Gooey Peach, Escape Powder, Stamina Drink and Bomb in the bag's first four slots, in that
  order (`IGame::PackItem`). It pays the Magical Crystal.
- **The challenge** (`yd_map`: `hidden`, `offered`, `taken`, `cleared`, `done`): clear the Moon Sea's floor 7 back
  floor (`selectMapNo` 4, `cur_floor` 6, `BtUraDongeon`) as Toan with only the Dagger. `MoonSeaChallengeStep`
  (each dungeon step) begins it `kChallengeSettle` frames onto the floor if Toan has the Dagger in hand; another
  member or weapon, or an item thrown (`CMainItemModel::SetThrowModel`), ends it until the floor is come onto again.
  Once a monster has been seen alive there, none living clears it. `HealingWater` heals nothing there while the
  challenge is taken (`SpringsDry`). Each turn is told at the bottom of the screen. It pays the Map.
- **The rewards**: carried in the pack, the Map and Magical Crystal act on every floor as the floor's own do once
  picked up (`BtEquipMap`, `BtEquipMasuisyou`, set after `BtCleatRandomMap`'s and `BtCleatFreeMap`'s reset:
  `DungeonFloorBuilt`), and each floor's own (its first two small chests) holds another of the floor's items in
  their place (`PresetSmallItemNo_Get`: `CarriedMapChests`).

## The quest tracker

`quests/tracker.cpp`: the journal's Quests category (the sixth, after Miracle Chests: `menu_manual.cpp`, mode 101) opens
a page like the Chest Tracker's. `TrackedQuests` lists the quests under way in the order they are registered (Monster
Slayer, fishing, item hunts, Yellow Drops), each with its giver, their town (the Chest Tracker's names,
`tracker.town.<n>`), its task, its progress (kills, fish, 0 or 1 for the rest) and the item drawn beside it (the
reward, or Ledan's and Samba's prizes). A quest complete says to go back to its giver in place of its progress. Up and
Down choose among them, six shown at a time with a scroll bar past that; the pointer chooses a row. Samba's task names
Toan as the player named him (`CSaveData::GetCharaName`, through the name entry's codes). Expanded's language files
also give the dungeon menu's Journal help (`battlemenu.journal.help`) a word of the quests.

## Trying them

A quest moved on in play is logged as a `quest` event (its id, stage and `left`) with `--event-log`, and a hunt's
item put in a back floor's chest as `quest_item`. A town stage
with `villager` stands the player in front of a giver (`expanded/docs/recording.md`): Macho with
`town = norune` and `villager = 3`. A save folder's `saves/<slot>/quests.json` sets a quest's stage and vars to start
from, for instance two Dashers from done (a dungeon stage's `back_floor` reaches Samba's floor: `dungeon = 4`,
`floor = 6`):

```json
{"version": 1, "quests": {"slayer_macho": {"stage": "active", "vars": {"dungeon": 0, "monster": 3, "left": 2, "total": 12, "given": 1}}}}
```

# mod-moa — My Own Azeroth

A deliberately small progression + challenge module for a **personal** AzerothCore 3.3.5a server.

It does two things:

1. **Stage-based progression** — every stage defines a level cap and a set of instances. Clearing all end bosses of the current stage unlocks the next one automatically.
2. **Two opt-in challenges** — Semi-Hardcore and Slow XP.

It does **not** touch spells, talents, items, creature stats, quests, loot or world phasing. That is the whole point: no retroactive content rewriting means it stays fully compatible with `mod-playerbots`.

> Reference projects: [mod-individual-progression](https://github.com/ZhengPeiRu21/mod-individual-progression) and [mod-challenge-modes](https://github.com/ZhengPeiRu21/mod-challenge-modes). This module keeps only the instance/level gating and two challenge modes from them.

---

## Requirements

- AzerothCore 3.3.5a (`Playerbot` branch or current master — only stable `PlayerScript` hooks are used)
- `MaxPlayerLevel = 80` in `worldserver.conf`
  The module caps players per stage. If the core itself is capped at 60, nobody can ever level past 60 when the stage advances.
- `EnablePlayerSettings = 1` in `worldserver.conf`
  Challenge choices are stored in the native `character_settings` table; without this they are not persisted.

The module logs an `ERROR` on startup if either one is wrong.

---

## Install

```bash
cd /path/to/azerothcore-wotlk/modules
git clone https://github.com/alexchenhq/mod-MOA.git mod-moa
```

Import the SQL into the **world** database:

```bash
mysql -u USER -p acore_world < mod-moa/data/sql/db-world/base/moa_01_tables.sql
mysql -u USER -p acore_world < mod-moa/data/sql/db-world/base/moa_02_data.sql
```

Rebuild:

```bash
cmake -S . -B ../build
cmake --build ../build --config RelWithDebInfo --target worldserver
```

Copy `conf/mod_moa.conf.dist` to `configs/modules/mod_moa.conf` (CMake also copies it automatically on configure).

---

## Stages

The default layout has 12 stages:

| Stage | Level cap | Content |
|---|---|---|
| 1 | 60 | Classic dungeons, Molten Core, Onyxia |
| 2 | 60 | Blackwing Lair, Zul'Gurub |
| 3 | 60 | Ruins of Ahn'Qiraj, Temple of Ahn'Qiraj |
| 4 | 70 | TBC dungeons, Karazhan, Gruul, Magtheridon |
| 5 | 70 | Serpentshrine Cavern, Tempest Keep |
| 6 | 70 | Mount Hyjal, Black Temple |
| 7 | 70 | Sunwell Plateau, Magisters' Terrace |
| 8 | 80 | WotLK dungeons, Naxxramas, Sartharion, Malygos, Archavon |
| 9 | 80 | Ulduar |
| 10 | 80 | Trial of the Crusader, Forge of Souls, Pit of Saron, Halls of Reflection |
| 11 | 80 | Icecrown Citadel |
| 12 | 80 | Ruby Sanctum |

Naxxramas 40 does not exist on a 3.3.5 client, so it is not modelled — Naxxramas is a stage 8 (level 80) raid.

### Advancing

A stage completes when **every boss listed for it in `moa_stage_boss`** has been killed. Kill credit is recorded in `moa_boss_kill`, the new stage is written to `moa_state`, every online player's level cap is refreshed, and the server gets a broadcast.

Only **raid end bosses** are listed by default. If you also want dungeon end bosses to count, insert them into `moa_stage_boss` — the table is the definition, no code change needed.

Difficulty variants are expanded automatically from `creature_template.difficulty_entry_1..3`, so killing 10-man or 25-man Kel'Thuzad both count.

---

## Commands

| Command | Access | Description |
|---|---|---|
| `.moa` | player | Current stage, level cap, instance/boss progress, own challenges |
| `.moa stage` | player | Same as above (kept for discoverability) |
| `.moa stage set <n>` | GM | Jump to a stage |
| `.moa stage next` | GM | Advance one stage (also used to skip a stuck stage) |
| `.moa challenge` | player | Alias of `.moa` for challenge status |
| `.moa challenge on semi` | player | Enable Semi-Hardcore (level 1, DK 55) |
| `.moa challenge on slow` | player | Enable Slow XP (level 1, DK 55) |
| `.moa reload` | GM | Reload config + stage tables |

Challenges are permanent once enabled and are stored per character.

---

## Challenges

### Semi-Hardcore

Dying to a **creature** in the **open world** destroys all equipped items and removes all carried gold.

Not triggered by:

- deaths inside dungeons and raids (`MOA.SemiHardcore.ExcludeDungeon`)
- deaths inside battlegrounds and arenas (`MOA.SemiHardcore.ExcludeBattleground`)
- **any PvP death** — those fire `OnPlayerPVPKill`, a different hook, so the penalty code is never reached

Bank and bags are untouched; only equipped slots are destroyed.

### Slow XP

`MOA.SlowXP.Multiplier` (default `0.5`). Applied after the level-cap check, so it stacks correctly with the stage cap.

---

## How the gating works

| Gate | Hook | Notes |
|---|---|---|
| Instance entry | `PlayerScript::OnPlayerCanEnterMap` | Called from `MapMgr::PlayerCannotEnter`, the single entry path for instance maps. GMs bypass it. |
| Level up | `PlayerScript::OnPlayerCanGiveLevel` | Hard stop: returning `false` aborts `Player::GiveLevel`. |
| XP gain | `PlayerScript::OnPlayerGiveXP` | Zeroed at the cap so experience does not pile up; also applies the slow-XP multiplier. |
| Client XP bar | `PLAYER_FIELD_MAX_LEVEL` via `OnPlayerSetMaxLevel` + `OnPlayerLogin` | `InitStatsForLevel` is not called on login, so the field is refreshed on login and on every stage change. |
| Boss kill | `PlayerScript::OnPlayerCreatureKillCredit` | Includes pet/totem kills and tapped mobs. |

`MOA.DisableRDF = 1` blocks the dungeon finder on purpose: a random queue can roll an instance that is not unlocked yet and leave the group stuck on a failed teleport. Players travel to the entrance manually.

---

## Customising

- **Level caps / stage count** — `MOA.MaxLevels` in the conf. The number of entries is the number of stages.
- **Which instance unlocks when** — `moa_instance_unlock`. Maps not listed are never gated.
- **Which bosses gate a stage** — `moa_stage_boss`.
- **Reset a stage's progress** — `DELETE FROM moa_boss_kill WHERE stage = N;`
- **Re-apply default data** — re-run `moa_02_data.sql` (it deletes and re-inserts both tables).

---

## playerbots

Nothing in this module inspects bot state or alters combat, spells or AI, so bots are unaffected by design. The two known boundaries:

- Bots do not opt into challenges (default player setting is 0).
- A bot teleported by a command skips `MapMgr::PlayerCannotEnter`, so it is not gated. Bots that follow a player into an unlocked instance behave normally.

---

## License

GNU AGPL v3, same as AzerothCore.

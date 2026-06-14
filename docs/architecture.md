# Architecture

Developer overview of how the Stat Booster module is put together. For config
keys see [Configuration](configuration.md); for the data tables see
[Database](database.md).

## What it does

When a player obtains a piece of equipment (loot, quest reward, crafting, or a
group roll), the module rolls a chance to add an extra stat enchant chosen to
*complement* the item. It inspects the item's existing stats to guess its role
(tank / physical DPS / hybrid / spell), then picks an item-level-appropriate
enchant for that role from a database pool. Players can re-roll the extra stat
with the **Attribute Recalibrator** item (entry `41605`).

## Source layout (`src/`)

| File | Responsibility |
|------|----------------|
| `MP_loader.cpp` | Module entry point. `Addmod_stat_boosterScripts()` registers the scripts. The name must match the module folder (`-` → `_`). |
| `StatBoost.{h,cpp}` | The AzerothCore script hooks. Thin layer: enable-check → call `StatBoostMgr` → feedback (sound / announce / soulbind). Also the `.sb` GM command. |
| `StatBoostMgr.{h,cpp}` | Core boosting logic. Static and stateless. |
| `StatBoostCfgMgr.{h,cpp}` | Config singleton (`sBoostConfigMgr`) plus the two DB-backed pools. |
| `StatBoostCommon.h` | Shared includes. |

There is no `CMakeLists.txt`; AzerothCore auto-globs `src/`.

## Hooks (`StatBoost.cpp`)

All on `ScriptObject` subclasses. PlayerScript hooks use the core's `OnPlayer*`
names.

- **`StatBoosterPlayer : PlayerScript`**
  - `OnPlayerLogin` — optional login message.
  - `OnPlayerLootItem`, `OnPlayerQuestRewardItem`, `OnPlayerCreateItem`,
    `OnPlayerGroupRollRewardItem` — the four item sources. Each: bail if
    disabled → `StatBoostMgr::BoostItem(player, item, chance)` → on success play
    sound / announce / optionally soulbind, per per-source config.
  - `OnPlayerCanCastItemUseSpell` — the reroll item (`41605`). Validates target
    ownership / boosted-only rules, re-boosts at 100% chance, consumes the item,
    and plays the configured visual.
- **`StatBoosterWorld : WorldScript`** — `OnAfterConfigLoad(reload)` loads all
  config and (re)loads the DB pools. Runs on startup **and** `.reload config`,
  so it is reload-safe (pools are cleared before refill).
- **`StatBoosterCommands : CommandScript`** — `.sb additem <itemId> <count>
  [suffixId]` (SEC_ADMINISTRATOR): stores an item on the target player and
  boosts it.

## The boost flow (`StatBoostMgr::BoostItem`)

```
BoostItem(player, item, chance)
  ├─ IsEquipment(item)                  weapon or armor only
  ├─ quality in [MinQuality, MaxQuality]
  ├─ urand(0,100) <= chance             the roll
  ├─ statType = AnalyzeItem(item)       determine the role (see below)
  │     └─ fallback GetStatTypeFromSubClass() if unscored
  ├─ enchant = EnchantPool.Get(role, classMask, subClassMask, typeMask, iLvl)
  └─ EnchantItem(...)                   apply + mark boosted
```

Masks passed to the pool are built as `1 << value` from the item's class,
subclass, and inventory type. See [Database](database.md) for how the pool
matches them.

### Determining the role (`AnalyzeItem` → `ScoreItem`)

`AnalyzeItem` decides whether the item has anything to score:

- No stats, no item spells, no random property → `GetStatTypeFromSubClass`
  picks a role randomly from those sensible for the weapon/armor subclass.
- Otherwise `ScoreItem` tallies a score per role by running each signal through
  the `statbooster_enchant_scores` table:
  - each `ItemStat` on the template,
  - stats delivered via item `Spells` (matched by aura type),
  - stats from the item's random property / suffix enchantments.

  The highest-scoring role wins; a zero tie returns `STAT_TYPE_NONE`, which
  falls back to `GetStatTypeFromSubClass`.

`StatType` is a bitmask: `NONE=0, TANK=1, PHYS=2, HYBRID=4, SPELL=8`.

### Applying the enchant (`EnchantItem`)

- **Armor** → the enchant goes in `TEMP_ENCHANTMENT_SLOT`.
- **Weapons** → the enchant goes in the first free socket slot, plus a dummy
  scaling enchant (`ENCHANT_DUMMY = 2814`) in `PRISMATIC_ENCHANTMENT_SLOT` so
  the slot renders correctly.

Enchant application unapplies/reapplies the item's enchantments around the
change so stats take effect immediately, then sets the boost marker flag
`ITEM_FIELD_FLAG_UNK26`. `IsBoosted()` checks this flag (used by the reroll
item and `OverwriteEnchantEnable`).

## Persistence

The boost lives entirely in the item's enchantment slot + flag, which the core
already saves with the item — the module stores no per-item data of its own.
Its only tables are the two read-only config pools described in
[Database](database.md).

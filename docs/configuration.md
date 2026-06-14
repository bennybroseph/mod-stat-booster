# Configuration

All options live under the `[worldserver]` section of
`mod-stat-booster.conf` (copy `mod-stat-booster.conf.dist` into your server's
`configs/modules/`). They are read in `StatBoosterWorld::OnAfterConfigLoad`, so
`.reload config` re-applies them and re-loads the [Database](database.md) pools.

The module is enabled by default; set `StatBooster.Enable = 0` to turn it off
(nothing else is loaded while it is disabled).

## Module

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.Enable` | `1` | Master on/off. |
| `StatBooster.VerboseEnable` | `0` | Debug logging to chat/console. Has a real performance cost in hot hooks — debugging only. |

## Sources & chances

Each item source can be toggled and given an independent proc chance (0–100).

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.OnLootItemEnable` | `1` | Boost looted items (also covers group-roll rewards). |
| `StatBooster.OnQuestRewardItemEnable` | `1` | Boost quest reward items. |
| `StatBooster.OnCraftItemEnable` | `1` | Boost crafted items. |
| `StatBooster.LootItemChance` | `55` | Percent chance for loot / rolls. |
| `StatBooster.QuestRewardChance` | `30` | Percent chance for quest rewards. |
| `StatBooster.CraftItemChance` | `85` | Percent chance for crafting. |

## Item eligibility

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.MinQuality` | `2` (Uncommon) | Lowest item quality eligible. |
| `StatBooster.MaxQuality` | `4` (Epic) | Highest item quality eligible. |

Quality scale: `0` Junk, `1` Common, `2` Uncommon, `3` Rare, `4` Epic,
`5` Legendary. Only weapons and armor are ever considered.

## Enchant slot handling

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.OverwriteEnchantEnable` | `1` | Overwrite the bonus enchant slot if already populated. Note: gemming an item afterward can drop the boosted stat. |

## Feedback

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.OnLoginEnable` | `1` | Show a login message to the player. |
| `StatBooster.OnLoginMessage` | "This server is running the StatBooster module." | The login message. |
| `StatBooster.AnnounceBoostEnable` | `1` | Whisper the player when an item is boosted. |
| `StatBooster.AnnounceLoot` | "You looted a boosted item." | Loot/roll message. |
| `StatBooster.AnnounceQuest` | "You received a boosted item." | Quest message. |
| `StatBooster.AnnounceCraft` | "You crafted a boosted item." | Craft message. |
| `StatBooster.PlaySoundEnable` | `1` | Play a sound on boost. |
| `StatBooster.SoundId` | `120` | Sound id (120 = loot-coin). Ids: https://wotlkdb.com/?sounds |

## Soulbinding

Bind the item on boost, per source. Only affects items that are
Bind-on-Equip (`BIND_WHEN_EQUIPPED`).

| Key | Default |
|-----|---------|
| `StatBooster.SoulbindOnEnchantLoot` | `0` |
| `StatBooster.SoulbindOnEnchantQuest` | `0` |
| `StatBooster.SoulbindOnEnchantCraft` | `0` |
| `StatBooster.SoulbindOnEnchantRoll` | `0` |

## Reroll item (Attribute Recalibrator, entry `41605`)

| Key | Default | Description |
|-----|---------|-------------|
| `StatBooster.Reroll.VisualId` | `62015` | Spell visual played on a successful re-roll. |
| `StatBooster.Reroll.AllowOwnedItemsOnly` | `1` | Block re-rolling items the caster doesn't own. |
| `StatBooster.Reroll.AllowBoostedItemsOnly` | `0` | Only allow re-rolling items that already have a boosted stat. |

# Database

The module is driven by two read-only tables in the **world** database, loaded
into memory on startup and on `.reload config`. See [Architecture](architecture.md)
for where they sit in the boost flow.

SQL lives in `data/sql/db-world/`: `base/` holds the initial schema + seed data,
`updates/` holds dated incremental changes (`statbooster_world_YYYY_MM_DD_NN.sql`).
Never edit an already-shipped file — add a new dated update instead.

## `statbooster_enchant_template` — the enchant pool

Every row is a candidate enchant the booster can apply. On boost, the role and
the item's properties are matched against these rows and one matching row is
picked at random.

| Column | Meaning |
|--------|---------|
| `Id` | Enchantment id (`SpellItemEnchantment.dbc`) — the stat actually applied. |
| `iLvlMin`, `iLvlMax` | Inclusive item-level range this enchant is allowed on. |
| `RoleMask` | Which roles this enchant suits (bitmask, see below). `0` = any. |
| `ClassMask` | Which item classes. `0` = any. |
| `SubClassMask` | Which item subclasses. `0` = any. |
| `ItemTypeMask` | Which inventory types (slots). `0` = any. |
| `Description`, `Note` | Human-readable; ignored by code. |

### How masks work

- **RoleMask** uses the `StatRole` bits directly: `TANK=1`, `PHYS=2`,
  `HYBRID=4`, `SPELL=8`. Combine by adding, e.g. `3` = TANK+PHYS,
  `7` = TANK+PHYS+HYBRID, `12` = HYBRID+SPELL.
- **ClassMask / SubClassMask / ItemTypeMask** are `1 << value`, where the value
  is the item's `ItemClass` / subclass / `InventoryType` enum (see
  `ItemTemplate.h`). For example armor (`ITEM_CLASS_ARMOR = 4`) → `1 << 4 = 16`.
  Combine slots by OR-ing the bits (e.g. the seed data uses `351535086` for
  "all equip slots except shield/tabard").

### Matching rule

A row matches when, for each mask, the item's bit is contained in the row's mask
**or** the row's mask is `0` (wildcard), **and** the item level is within
`[iLvlMin, iLvlMax]`. The role check uses the single role the booster picked for
the item, so a `RoleMask` of `3` (TANK+PHYS) will match a PHYS item. Matching
rows are shuffled and the first is used, giving an even random pick.

### Adding an enchant

Insert a row with the enchant `Id`, an item-level band, and whatever
role/class/subclass/type restrictions you want (use `0` to leave a dimension
unrestricted). Add it as a new dated `updates/` file.

## `statbooster_enchant_scores` — role scoring

This table teaches the booster which role an existing stat implies. `ScoreItem`
runs every stat it finds on an item through this table and sums the per-role
scores; the highest total wins.

| Column | Meaning |
|--------|---------|
| `mod_type` | `0` = item stat (`ITEM_MOD_*`), `1` = spell aura (`SPELL_AURA_*`). |
| `mod_id` | The stat/aura enum value. |
| `subclass` | Item subclass this scoring applies to; `0` = any. |
| `tank_score`, `phys_score`, `spell_score`, `hybrid_score` | Points added to each role. |
| `note` | Human-readable; ignored by code. |

The `subclass` column lets the same stat score differently by armor type — e.g.
Intellect scores pure spell on cloth but adds tank/hybrid weight on plate.

### Tuning role detection

Adjust the scores here rather than touching C++. A stat with no matching row
contributes nothing. If an item ends up unscored (all roles zero), the booster
falls back to a sensible random role based on the item's subclass.

## Other SQL

The base install also registers the `.sb` / `.sb additem` commands in the
`command` table, and an update repurposes item `41605` into the **Attribute
Recalibrator** reroll item.

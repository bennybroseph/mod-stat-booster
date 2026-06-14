# Stat Booster

An AzerothCore (WotLK 3.3.5a) module that randomly enchants equipment players
obtain — from loot, quest rewards, crafting, and group rolls — with an extra
stat chosen to complement the item.

## Documentation

- **[Architecture](architecture.md)** — how the module works: the hooks, the
  boost flow, role scoring, and how enchants get applied.
- **[Configuration](configuration.md)** — every `StatBooster.*` option and its
  default.
- **[Database](database.md)** — the two driver tables and how to add or tune
  enchants.

## Quick start

1. Clone into `azerothcore-wotlk/modules/mod-stat-booster` and rebuild the core.
2. Copy `mod-stat-booster.conf.dist` into your server's `configs/modules/`.
3. Start the worldserver — the module is enabled by default.

See [Configuration](configuration.md) for the full option list (including how to
disable it or tune the boost chances).

# Testing

The module ships Google Test unit tests that run inside AzerothCore's existing
`unit_tests` target — no core edits, and they only build when you ask for them.

## What's covered

Most of the boost pipeline is glued to the game runtime (`Player`, `Item`,
`ItemTemplate`, the enchantment APIs, the DBC stores, the world database), which
the unit harness can't construct. The pure, regression-prone decision logic is
extracted and tested:

- **Enchant selection** — `StatBoosterConfig::EnchantPool::Get(...)`
  (`tests/PoolMatchTest.cpp`): the role/class/subclass/type bitmask matching
  (a `0` mask is "any") and the inclusive item-level range.
- **Role scoring** — `StatBoosterConfig::EnchantScorePool::Evaluate(...)`
  (`tests/PoolMatchTest.cpp`): the `(modType, modId, subclass)` lookup with the
  `subclass == 0` wildcard, and the additive accumulation into the four role
  scores.
- **Role detection** — `StatBoostMgr::RoleFromItemType(...)`
  (`tests/RoleDetectionTest.cpp`): the weapon/armor -> role mapping. The random
  source is injected (`RandFn`) so the multi-option branches are deterministic.

`StatBoostMgr::GetStatRoleFromSubClass(Item*)` is a thin wrapper over
`RoleFromItemType`, so the tests exercise the production mapping.

### What's *not* unit-tested (and why)

The orchestration — `BoostItem`, `AnalyzeItem` / `ScoreItem`, `EnchantItem`,
`MakeSoulbound` — needs a live `Player` / `Item`, the DBC stores, and the world
database, none of which exist in the unit harness. Those stay covered by in-game
verification.

## How it's wired

- `mod-stat-booster.cmake` (included inline by `modules/CMakeLists.txt`) appends
  the test sources and the `src/` include dir to the global
  `ACORE_MODULE_TEST_SOURCES` / `ACORE_MODULE_TEST_INCLUDES` properties — only
  when `BUILD_TESTING` is on.
- The core's `src/test/CMakeLists.txt` adds those sources to `unit_tests` and
  links the `modules` library, so the tests compile and link against the module.
- Test sources live in `tests/` (a sibling of `src/`), so they are **not**
  compiled into the `modules` library itself — only into `unit_tests`.

## Building and running

If you run the server via the AzerothCore Docker stack, the easiest path is the
`ac-dev-server` container (compose profile `dev`), which builds in dedicated
Docker volumes — isolated from your live `ac-worldserver` / `ac-database` and
leaving no build files in your project folder:

```bash
# from the repo root
docker compose --profile dev run --rm ac-dev-server \
  bash modules/mod-stat-booster/tests/run-in-docker.sh        # all cores
docker compose --profile dev run --rm ac-dev-server \
  bash modules/mod-stat-booster/tests/run-in-docker.sh 4      # -j4, host headroom
```

Or build directly with a local toolchain:

```bash
cmake -S . -B build -DBUILD_TESTING=ON -DSCRIPTS=static -DMODULES=static
cmake --build build --target unit_tests -j"$(nproc)"
./build/src/test/unit_tests --gtest_filter='StatBooster*'
```

Notes:

- Building `unit_tests` compiles the full `game` + `modules` libraries (it links
  them), so the **first** run is a heavy compile; ccache + the persisted build
  volume make re-runs fast.
- `--gtest_filter='StatBooster*'` runs just this module's cases; drop it to run
  the whole suite. When configured you'll see
  `mod-stat-booster: registered unit tests`.

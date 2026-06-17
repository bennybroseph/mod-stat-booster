/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// Unit tests for the stat-booster config pools — the enchant mask + item-level
// matching (EnchantPool::Get) and the role scoring with its subclass wildcard
// (EnchantScorePool::Evaluate). Pure logic over plain data; no Player/Item/DB.

#include "StatBoostCfgMgr.h"
#include "gtest/gtest.h"

namespace
{
    // Mirror the StatRole bits / DB RoleMask (StatBoostMgr::STAT_ROLE_*), kept
    // local so this file needn't depend on StatBoostMgr.h. A 0 mask is "any".
    constexpr uint32 kRoleTank = 1;
    constexpr uint32 kRolePhys = 2;
    constexpr uint32 kRoleSpell = 8;
    constexpr uint32 kAny = 0;

    // Arbitrary single-bit class masks (the field uses generic bitmask logic;
    // production passes 1 << itemClass).
    constexpr uint32 kClassBitA = 1u << 2;
    constexpr uint32 kClassBitB = 1u << 4;

    EnchantDefinition Def(uint32 id, uint32 lo, uint32 hi, uint32 role,
        uint32 cls = 0, uint32 sub = 0, uint32 type = 0)
    {
        return EnchantDefinition{id, lo, hi, role, cls, sub, type};
    }

    StatBoosterConfig::EnchantScore Score(uint32 modType, uint32 modId,
        uint32 subclass, uint32 tank, uint32 phys, uint32 spell, uint32 hybrid)
    {
        return StatBoosterConfig::EnchantScore{modType, modId, subclass,
            tank, phys, spell, hybrid};
    }
}

// --- EnchantPool::Get — match rule is, per field:
//     (data.Mask & query) == query  OR  data.Mask == 0 (wildcard),
//     AND ILvlMin <= itemLevel <= ILvlMax.
// Each test uses a pool with exactly one matching entry, so the internal
// shuffle can't change the outcome. ---

TEST(StatBoosterEnchantPool, ExactRoleBitMatches)
{
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(100, 1, 80, kRoleTank));

    EnchantDefinition* result = pool.Get(kRoleTank, kAny, kAny, kAny, 50);

    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->Id, 100u);
}

TEST(StatBoosterEnchantPool, EnchantValidForSupersetOfRolesMatches)
{
    // Enchant is valid for TANK or PHYS; an item whose role is PHYS matches.
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(101, 1, 80, kRoleTank | kRolePhys));

    EXPECT_NE(pool.Get(kRolePhys, kAny, kAny, kAny, 50), nullptr);
}

TEST(StatBoosterEnchantPool, RoleMismatchReturnsNull)
{
    // Enchant is TANK-only; a SPELL item must not match.
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(102, 1, 80, kRoleTank));

    EXPECT_EQ(pool.Get(kRoleSpell, kAny, kAny, kAny, 50), nullptr);
}

TEST(StatBoosterEnchantPool, WildcardRoleMaskMatchesAnyRole)
{
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(103, 1, 80, kAny));

    EXPECT_NE(pool.Get(kRoleSpell, kAny, kAny, kAny, 50), nullptr);
    EXPECT_NE(pool.Get(kRoleTank, kAny, kAny, kAny, 50), nullptr);
}

TEST(StatBoosterEnchantPool, ClassMaskIsEnforced)
{
    // RoleMask wildcard so only the class field decides the match.
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(104, 1, 80, kAny, kClassBitA));

    EXPECT_NE(pool.Get(kRoleTank, kClassBitA, kAny, kAny, 50), nullptr);
    EXPECT_EQ(pool.Get(kRoleTank, kClassBitB, kAny, kAny, 50), nullptr);
}

TEST(StatBoosterEnchantPool, ItemLevelMustBeWithinRange)
{
    StatBoosterConfig::EnchantPool pool;
    pool.Add(Def(105, 20, 40, kAny));

    EXPECT_NE(pool.Get(kRoleTank, kAny, kAny, kAny, 20), nullptr); // low bound
    EXPECT_NE(pool.Get(kRoleTank, kAny, kAny, kAny, 40), nullptr); // high bound
    EXPECT_EQ(pool.Get(kRoleTank, kAny, kAny, kAny, 19), nullptr); // below
    EXPECT_EQ(pool.Get(kRoleTank, kAny, kAny, kAny, 41), nullptr); // above
}

TEST(StatBoosterEnchantPool, EmptyPoolReturnsNull)
{
    StatBoosterConfig::EnchantPool pool;
    EXPECT_EQ(pool.Get(kRoleTank, kAny, kAny, kAny, 50), nullptr);
}

// --- EnchantScorePool::Evaluate — finds the first score row matching
//     (modType, modId) with (subclass == query || subclass == 0), then ADDS its
//     four role scores into the accumulators. ---

class StatBoosterEnchantScore : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Evaluate reads VerboseEnable off the global config singleton for its
        // logging branch; keep it quiet and deterministic.
        sBoostConfigMgr->VerboseEnable = false;
    }
};

TEST_F(StatBoosterEnchantScore, ExactMatchAddsScores)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 4, 2, 1, 2, 0, 1));

    uint32 tank = 0, phys = 0, spell = 0, hybrid = 0;
    scores.Evaluate(0, 4, 2, tank, phys, spell, hybrid);

    EXPECT_EQ(tank, 1u);
    EXPECT_EQ(phys, 2u);
    EXPECT_EQ(spell, 0u);
    EXPECT_EQ(hybrid, 1u);
}

TEST_F(StatBoosterEnchantScore, SubclassZeroIsWildcard)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 5, 0, 1, 0, 3, 2));

    uint32 tank = 0, phys = 0, spell = 0, hybrid = 0;
    scores.Evaluate(0, 5, 99, tank, phys, spell, hybrid); // any subclass

    EXPECT_EQ(tank, 1u);
    EXPECT_EQ(spell, 3u);
    EXPECT_EQ(hybrid, 2u);
}

TEST_F(StatBoosterEnchantScore, NoMatchLeavesAccumulatorsUntouched)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 4, 2, 1, 2, 0, 1));

    uint32 tank = 5, phys = 5, spell = 5, hybrid = 5;
    scores.Evaluate(0, 999, 2, tank, phys, spell, hybrid); // modId absent

    EXPECT_EQ(tank, 5u);
    EXPECT_EQ(phys, 5u);
    EXPECT_EQ(spell, 5u);
    EXPECT_EQ(hybrid, 5u);
}

TEST_F(StatBoosterEnchantScore, ModTypeMustMatch)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 5, 0, 1, 1, 1, 1));

    uint32 tank = 0, phys = 0, spell = 0, hybrid = 0;
    scores.Evaluate(1, 5, 0, tank, phys, spell, hybrid); // id ok, modType not

    EXPECT_EQ(tank, 0u);
    EXPECT_EQ(phys, 0u);
}

TEST_F(StatBoosterEnchantScore, ScoresAreAdditive)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 4, 0, 3, 0, 0, 0));

    uint32 tank = 10, phys = 0, spell = 0, hybrid = 0;
    scores.Evaluate(0, 4, 1, tank, phys, spell, hybrid);

    EXPECT_EQ(tank, 13u); // 10 + 3, not replaced
}

TEST_F(StatBoosterEnchantScore, FirstMatchingRowWins)
{
    StatBoosterConfig::EnchantScorePool scores;
    scores.Add(Score(0, 4, 2, 1, 0, 0, 0));
    scores.Add(Score(0, 4, 2, 99, 0, 0, 0));

    uint32 tank = 0, phys = 0, spell = 0, hybrid = 0;
    scores.Evaluate(0, 4, 2, tank, phys, spell, hybrid);

    EXPECT_EQ(tank, 1u); // first row, not the second
}

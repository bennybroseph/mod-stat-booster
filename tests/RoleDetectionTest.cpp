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

// Unit tests for StatBoostMgr::RoleFromItemType — the weapon/armor -> role
// mapping used when an item has no scorable stats. The random source is
// injected so the multi-option branches are deterministic; single-option
// branches ignore it. Pins the current mapping table.

#include "StatBoostMgr.h"
#include "ItemTemplate.h"
#include "gtest/gtest.h"

using SBM = StatBoostMgr;

namespace
{
    constexpr uint32 kWeapon = ITEM_CLASS_WEAPON;
    constexpr uint32 kArmor = ITEM_CLASS_ARMOR;
    constexpr uint32 kConsumable = ITEM_CLASS_CONSUMABLE;

    // Fixed-value random stubs (-> StatBoostMgr::RandFn). Each maps to the
    // index the role table would otherwise roll via urand.
    uint32 rng0(uint32, uint32) { return 0; }
    uint32 rng1(uint32, uint32) { return 1; }
    uint32 rng2(uint32, uint32) { return 2; }
    uint32 rng3(uint32, uint32) { return 3; }

    SBM::StatRole Role(uint32 cls, uint32 sub, uint32 inv, SBM::RandFn rng)
    {
        return SBM::RoleFromItemType(cls, sub, inv, rng);
    }
}

// --- Single-option mappings: the result is fixed regardless of the RNG. ---

TEST(StatBoosterRole, ThrownIsAlwaysPhys)
{
    EXPECT_EQ(Role(kWeapon, ITEM_SUBCLASS_WEAPON_THROWN, 0, &rng0),
        SBM::STAT_ROLE_PHYS);
}

TEST(StatBoosterRole, WandIsAlwaysSpell)
{
    EXPECT_EQ(Role(kWeapon, ITEM_SUBCLASS_WEAPON_WAND, 0, &rng0),
        SBM::STAT_ROLE_SPELL);
}

TEST(StatBoosterRole, NonCloakClothIsSpell)
{
    EXPECT_EQ(Role(kArmor, ITEM_SUBCLASS_ARMOR_CLOTH, INVTYPE_CHEST, &rng0),
        SBM::STAT_ROLE_SPELL);
}

// --- Multi-option weapon mappings (RNG-driven). ---

TEST(StatBoosterRole, TwoHandMaceTable)
{
    constexpr uint32 s = ITEM_SUBCLASS_WEAPON_MACE2;
    EXPECT_EQ(Role(kWeapon, s, 0, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng1), SBM::STAT_ROLE_PHYS);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng2), SBM::STAT_ROLE_HYBRID);
}

TEST(StatBoosterRole, DaggerTable)
{
    constexpr uint32 s = ITEM_SUBCLASS_WEAPON_DAGGER;
    EXPECT_EQ(Role(kWeapon, s, 0, &rng0), SBM::STAT_ROLE_PHYS);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng1), SBM::STAT_ROLE_HYBRID);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng2), SBM::STAT_ROLE_SPELL);
}

TEST(StatBoosterRole, StaffCoversAllFourRoles)
{
    constexpr uint32 s = ITEM_SUBCLASS_WEAPON_STAFF;
    EXPECT_EQ(Role(kWeapon, s, 0, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng1), SBM::STAT_ROLE_PHYS);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng2), SBM::STAT_ROLE_HYBRID);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng3), SBM::STAT_ROLE_SPELL);
}

TEST(StatBoosterRole, OneHandSwordIsTankOrPhys)
{
    constexpr uint32 s = ITEM_SUBCLASS_WEAPON_SWORD;
    EXPECT_EQ(Role(kWeapon, s, 0, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kWeapon, s, 0, &rng1), SBM::STAT_ROLE_PHYS);
}

// --- Multi-option armor mappings (RNG-driven). ---

TEST(StatBoosterRole, ClothCloakCoversAllFourRoles)
{
    constexpr uint32 s = ITEM_SUBCLASS_ARMOR_CLOTH;
    EXPECT_EQ(Role(kArmor, s, INVTYPE_CLOAK, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kArmor, s, INVTYPE_CLOAK, &rng3), SBM::STAT_ROLE_SPELL);
}

TEST(StatBoosterRole, LeatherCoversAllFourRoles)
{
    constexpr uint32 s = ITEM_SUBCLASS_ARMOR_LEATHER;
    EXPECT_EQ(Role(kArmor, s, 0, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kArmor, s, 0, &rng3), SBM::STAT_ROLE_SPELL);
}

TEST(StatBoosterRole, ShieldIsTankOrSpell)
{
    constexpr uint32 s = ITEM_SUBCLASS_ARMOR_SHIELD;
    EXPECT_EQ(Role(kArmor, s, 0, &rng0), SBM::STAT_ROLE_TANK);
    EXPECT_EQ(Role(kArmor, s, 0, &rng1), SBM::STAT_ROLE_SPELL);
}

// --- Edge cases -> no role. ---

TEST(StatBoosterRole, NonEquipClassIsNone)
{
    EXPECT_EQ(Role(kConsumable, 0, 0, &rng0), SBM::STAT_ROLE_NONE);
}

TEST(StatBoosterRole, UnknownWeaponSubclassIsNone)
{
    EXPECT_EQ(Role(kWeapon, 99, 0, &rng0), SBM::STAT_ROLE_NONE);
}

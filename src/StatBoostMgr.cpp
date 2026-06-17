#include "StatBoostMgr.h"

StatBoostMgr::StatRole StatBoostMgr::RoleFromItemType(uint32 itemClass, uint32 itemSubClass, uint32 itemType, RandFn rng)
{
    if (itemClass == ITEM_CLASS_WEAPON)
    {
        switch (itemSubClass)
        {
        case ITEM_SUBCLASS_WEAPON_MACE2:
        case ITEM_SUBCLASS_WEAPON_POLEARM:
        case ITEM_SUBCLASS_WEAPON_SPEAR:
        case ITEM_SUBCLASS_WEAPON_GUN:
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
            switch (rng(0, 2))
            {
            case 0:
                return STAT_ROLE_TANK;

            case 1:
                return STAT_ROLE_PHYS;

            case 2:
                return STAT_ROLE_HYBRID;
            }

        case ITEM_SUBCLASS_WEAPON_THROWN:
            return STAT_ROLE_PHYS;

        case ITEM_SUBCLASS_WEAPON_DAGGER:
            switch (rng(0, 2))
            {
            case 0:
                return STAT_ROLE_PHYS;

            case 1:
                return STAT_ROLE_HYBRID;

            case 2:
                return STAT_ROLE_SPELL;
            }

        case ITEM_SUBCLASS_WEAPON_STAFF:
            switch (rng(0, 3))
            {
            case 0:
                return STAT_ROLE_TANK;

            case 1:
                return STAT_ROLE_PHYS;

            case 2:
                return STAT_ROLE_HYBRID;

            case 3:
                return STAT_ROLE_SPELL;
            }

        case ITEM_SUBCLASS_WEAPON_AXE:
        case ITEM_SUBCLASS_WEAPON_AXE2:
        case ITEM_SUBCLASS_WEAPON_MACE:
        case ITEM_SUBCLASS_WEAPON_SWORD:
        case ITEM_SUBCLASS_WEAPON_SWORD2:
        case ITEM_SUBCLASS_WEAPON_FIST:
            switch (rng(0, 1))
            {
            case 0:
                return STAT_ROLE_TANK;

            case 1:
                return STAT_ROLE_PHYS;
            }

        case ITEM_SUBCLASS_WEAPON_WAND:
            return STAT_ROLE_SPELL;
        }
    }
    else if (itemClass == ITEM_CLASS_ARMOR)
    {
        switch (itemSubClass)
        {
        case ITEM_SUBCLASS_ARMOR_CLOTH:
            switch (itemType)
            {
            case INVTYPE_CLOAK:
                switch (rng(0, 3))
                {
                case 0:
                    return STAT_ROLE_TANK;

                case 1:
                    return STAT_ROLE_PHYS;

                case 2:
                    return STAT_ROLE_HYBRID;

                case 3:
                    return STAT_ROLE_SPELL;
                }

            default:
                return STAT_ROLE_SPELL;
            }
            break;

        case ITEM_SUBCLASS_ARMOR_LEATHER:
        case ITEM_SUBCLASS_ARMOR_MAIL:
        case ITEM_SUBCLASS_ARMOR_PLATE:
            switch (rng(0, 3))
            {
            case 0:
                return STAT_ROLE_TANK;

            case 1:
                return STAT_ROLE_PHYS;

            case 2:
                return STAT_ROLE_HYBRID;

            case 3:
                return STAT_ROLE_SPELL;
            }

        case ITEM_SUBCLASS_ARMOR_BUCKLER:
        case ITEM_SUBCLASS_ARMOR_SHIELD:
            switch (rng(0, 1))
            {
            case 0:
                return STAT_ROLE_TANK;
            case 1:
                return STAT_ROLE_SPELL;
            }
        }
    }

    return STAT_ROLE_NONE;
}   

StatBoostMgr::StatRole StatBoostMgr::GetStatRoleFromSubClass(Item* item)
{
    auto itemTemplate = item->GetTemplate();
    return RoleFromItemType(itemTemplate->Class, itemTemplate->SubClass, itemTemplate->InventoryType);
}

StatBoostMgr::StatRole StatBoostMgr::ScoreItem(Item* item, bool hasAdditionalSpells)
{
    ScoreData tankScore { STAT_ROLE_TANK, 0 },
        physScore { STAT_ROLE_PHYS, 0 },
        hybridScore { STAT_ROLE_HYBRID, 0 },
        spellScore { STAT_ROLE_SPELL, 0 };

    //Store the scores in a vector so I can order by highest for a winner.
    std::vector<ScoreData*> roleScores;
    roleScores.push_back(&tankScore);
    roleScores.push_back(&physScore);
    roleScores.push_back(&hybridScore);
    roleScores.push_back(&spellScore);

    //TODO: IMPLEMENT SCORING
    auto itemTemplate = item->GetTemplate();
    auto subClass = itemTemplate->SubClass;

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Found {} stats on item.", itemTemplate->StatsCount);
    }

    for (uint32 i = 0; i < itemTemplate->StatsCount; i++)
    {
        auto stat = itemTemplate->ItemStat[i];
        uint32 statType = stat.ItemStatType;

        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "StatType: {}, StatValue: {}", stat.ItemStatType, stat.ItemStatValue);
        }

        sBoostConfigMgr->EnchantScores.Evaluate(0, statType, subClass, tankScore.Score, physScore.Score, spellScore.Score, hybridScore.Score);
    }

    //Sometimes stats are stored as additional spell effects and also need to be checked.
    if (hasAdditionalSpells)
    {
        auto scores = sBoostConfigMgr->EnchantScores.Get();

        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "Found {} spells on item.", itemTemplate->StatsCount);
        }

        for (_Spell const &spell : itemTemplate->Spells)
        {
            if (spell.SpellId)
            {
                auto spellInfo = sSpellMgr->GetSpellInfo(spell.SpellId);
                
                if (!spellInfo || !scores)
                {
                    continue;
                }

                if (sBoostConfigMgr->VerboseEnable)
                {
                    LOG_INFO("module", "SpellId: {}", spell.SpellId);
                }

                for (auto const &score : *scores)
                {
                    if (score.modType == 1)
                    {
                        if (spellInfo->HasAura(static_cast<AuraType>(score.modId)))
                        {
                            sBoostConfigMgr->EnchantScores.Evaluate(1, score.modId, subClass, tankScore.Score, physScore.Score, spellScore.Score, hybridScore.Score);
                        }
                    }
                }
            }
        }
    }

    if (item->GetItemRandomPropertyId() != 0)
    {
        auto propId = item->GetItemRandomPropertyId();

        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "Found random property {}.", propId);
        }

        uint32 enchantIds[5];

        if (propId > 0)
        {
            auto propStoreEntry = sItemRandomPropertiesStore.LookupEntry(propId);
            if (propStoreEntry)
            {
                enchantIds[0] = propStoreEntry->Enchantment[0];
                enchantIds[1] = propStoreEntry->Enchantment[1];
                enchantIds[2] = propStoreEntry->Enchantment[2];
                enchantIds[3] = propStoreEntry->Enchantment[3];
                enchantIds[4] = propStoreEntry->Enchantment[4];
            }
        }
        else if (propId < 0)
        {
            auto suffixStoreEntry = sItemRandomSuffixStore.LookupEntry(-propId);
            if (suffixStoreEntry)
            {
                enchantIds[0] = suffixStoreEntry->Enchantment[0];
                enchantIds[1] = suffixStoreEntry->Enchantment[1];
                enchantIds[2] = suffixStoreEntry->Enchantment[2];
                enchantIds[3] = suffixStoreEntry->Enchantment[3];
                enchantIds[4] = suffixStoreEntry->Enchantment[4];
            }
        }

        for (uint32 i = 0; i < 5; i++)
        {
            auto enchantmentId = enchantIds[i];
            if (enchantmentId == 0)
            {
                continue;
            }

            auto spellItemEntry = sSpellItemEnchantmentStore.LookupEntry(enchantmentId);
            if (!spellItemEntry)
            {
                continue;
            }

            if (spellItemEntry->type[0] != ITEM_ENCHANTMENT_TYPE_STAT)
            {
                continue;
            }

            auto statType = spellItemEntry->spellid[0];
            if (statType == 0)
            {
                continue;
            }

            if (sBoostConfigMgr->VerboseEnable)
            {
                LOG_INFO("module", "Found random enchant {} with statType {}", spellItemEntry->description[0], statType);
            }

            sBoostConfigMgr->EnchantScores.Evaluate(0, statType, subClass, tankScore.Score, physScore.Score, spellScore.Score, hybridScore.Score);
        }
    }
    else
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "No random enchants found.");
        }
    }

    //Tally up the results, the highest score is picked.
    auto winningScore = roleScores[0];

    if (!winningScore)
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "No winning score found.");
        }

        return STAT_ROLE_NONE;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Finding winning score.");
    }
    for (auto score : roleScores)
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "Score: {}, Type: {}", score->Score, score->Role);
        }

        if (score->Score > winningScore->Score)
        {
            winningScore = score;
        }
    }

    //No stats on the items could be scored.
    if (winningScore->Score < 1)
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "No stats were scored.");
        }

        return STAT_ROLE_NONE;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Scoring with scores: Tank({}), Phys({}), Spell({}), Hybrid({})", tankScore.Score, physScore.Score, spellScore.Score, hybridScore.Score);
    }

    return winningScore->Role;
}

void StatBoostMgr::MakeSoulbound(Item* item, Player* player)
{
    auto itemTemplate = item->GetTemplate();

    if (itemTemplate->Bonding == BIND_WHEN_EQUIPPED)
    {
        item->SetState(ITEM_CHANGED, player);
        item->SetBinding(true);
    }
}

StatBoostMgr::StatRole StatBoostMgr::AnalyzeItem(Item* item)
{
    auto itemTemplate = item->GetTemplate();

    //The spellids need to be checked because the Spells array is always allocated to a fixed size.
    //Thus we need to count how many VALID spells are in the array.
    uint32 spellsCount = 0;
    for (const auto& spell : itemTemplate->Spells)
    {
        if (spell.SpellId)
        {
            spellsCount++;
        }
    }

    if (itemTemplate->StatsCount < 1 && spellsCount < 1 && item->GetItemRandomPropertyId() == 0)
    {
        return GetStatRoleFromSubClass(item);
    }

    return ScoreItem(item, spellsCount);
}

bool StatBoostMgr::IsBoosted(Item* item)
{
    return item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_UNK26);
}

bool StatBoostMgr::BoostItem(Player* player, Item* item, uint32 chance)
{
    if (!item)
    {
        return false;
    }

    //Is not weapon or armor.
    if (!IsEquipment(item))
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Equipment Check");
    }

    if (item->GetTemplate()->Quality < sBoostConfigMgr->MinQuality ||
        item->GetTemplate()->Quality > sBoostConfigMgr->MaxQuality)
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Quality Check. Quality({})", item->GetTemplate()->Quality);
    }

    //Roll for the chance to upgrade.
    uint32 roll = urand(0, 100);

    if (roll > chance)
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Roll Check. Roll({})", roll);
    }

    //Fetch the role of stats that should be applied to the piece.
    StatRole role = AnalyzeItem(item);

    //Failed to find a role.
    if (role == STAT_ROLE_NONE)
    {
        role = GetStatRoleFromSubClass(item);

        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "No role found, got from subclass.");
        }
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Analyze Check. Role({})", role);
    }

    uint32 itemClass = item->GetTemplate()->Class;
    uint32 itemSubClass = item->GetTemplate()->SubClass;
    uint32 itemType = item->GetTemplate()->InventoryType;
    uint32 itemLevel = item->GetTemplate()->ItemLevel;

    uint32 itemClassMask = 1 << itemClass;

    if (!itemClassMask)
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed ItemClass Check");
    }

    uint32 itemSubClassMask = 1 << itemSubClass;

    if (!itemSubClassMask)
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed ItemSubClass Check");
    }

    uint32 itemTypeMask = 1 << itemType;

    if (!itemTypeMask)
    {
        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed ItemType Check");
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", ">> Trying to get enchant with role mask {}, class {}, subClass {}, itemType {}, and itemlevel {} from pool.", role, itemClassMask, itemSubClassMask, itemTypeMask, itemLevel);
    }

    //Fetch an enchant from the enchant pool.
    auto enchant = sBoostConfigMgr->Enchants.Get(role, itemClassMask, itemSubClassMask, itemTypeMask, itemLevel);

    //Failed to find a valid enchant.
    if (!enchant)
    {
        if (sBoostConfigMgr->VerboseEnable)
        {
            LOG_INFO("module", "Failed Enchant Check.");
        }

        return false;
    }

    if (sBoostConfigMgr->VerboseEnable)
    {
        LOG_INFO("module", "Passed Enchant Check. Enchant({})", enchant->Id);
    }

    if (itemClass != ITEM_CLASS_WEAPON)
    {
        return EnchantItem(player, item, TEMP_ENCHANTMENT_SLOT, enchant->Id, sBoostConfigMgr->OverwriteEnchantEnable);
    }
    else
    {
        EnchantmentSlot enchantSlot = GetFreeSocketSlotForItem(item);

        if (enchantSlot != MAX_ENCHANTMENT_SLOT)
        {
            return EnchantItem(player, item, enchantSlot, enchant->Id, sBoostConfigMgr->OverwriteEnchantEnable) &&
                EnchantItem(player, item, PRISMATIC_ENCHANTMENT_SLOT, StatBoostMgr::ENCHANT_DUMMY, sBoostConfigMgr->OverwriteEnchantEnable);
        }
    }

    return false;
}

EnchantmentSlot StatBoostMgr::GetFreeSocketSlotForItem(Item* item)
{
    auto itemTemplate = item->GetTemplate();

    if (!itemTemplate->Socket[0].Color)
    {
        return SOCK_ENCHANTMENT_SLOT;
    }
    if (!itemTemplate->Socket[1].Color)
    {
        return SOCK_ENCHANTMENT_SLOT_2;
    }
    if (!itemTemplate->Socket[2].Color)
    {
        return SOCK_ENCHANTMENT_SLOT_3;
    }

    return MAX_ENCHANTMENT_SLOT;
}

bool StatBoostMgr::IsEquipment(Item* item)
{
    auto itemTemplate = item->GetTemplate();

    if (itemTemplate->Class != ITEM_CLASS_WEAPON &&
        itemTemplate->Class != ITEM_CLASS_ARMOR)
    {
        return false;
    }

    return true;
}

bool StatBoostMgr::EnchantItem(Player* player, Item* item, EnchantmentSlot slot, uint32 enchantId, bool overwrite)
{
    if (item->GetEnchantmentId(slot) && !overwrite)
    {
        return false;
    }

    player->ApplyEnchantment(item, false);
    item->SetEnchantment(EnchantmentSlot(slot), enchantId, 0, 0);
    player->ApplyEnchantment(item, true);

    item->SetFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_UNK26);

    return true;
}

#ifndef MODULE_STAT_BOOST_MGR
#define MODULE_STAT_BOOST_MGR

#include "StatBoostCommon.h"

class StatBoostMgr
{
public:
    static bool BoostItem(Player* player, Item* item, uint32 chance);
    static bool IsBoosted(Item* item);
    static void MakeSoulbound(Item* item, Player* player);

private:
    enum StatRole
    {
        STAT_ROLE_NONE = 0,
        STAT_ROLE_TANK = 1,
        STAT_ROLE_PHYS = 2,
        STAT_ROLE_HYBRID = 4,
        STAT_ROLE_SPELL = 8
    };

    static constexpr uint32 ENCHANT_DUMMY = 2814; // Scaling stat, used due to enchanting weirdness.

    struct ScoreData
    {
        StatRole Role;
        uint32 Score;
    };

    static StatRole GetStatRoleFromSubClass(Item* item);
    static EnchantmentSlot GetFreeSocketSlotForItem(Item* item);
    static bool EnchantItem(Player* player, Item* item, EnchantmentSlot slot, uint32 enchantId, bool overwrite = false);
    static StatRole ScoreItem(Item* item, bool hasAdditionalSpells = false);
    static StatRole AnalyzeItem(Item* item);
    static bool IsEquipment(Item* item);
};

#endif

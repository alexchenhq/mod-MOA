/*
 * mod-moa — challenge modes
 *
 * Semi-Hardcore : die to a creature in the open world -> all equipped items and
 *                 carried gold are lost. Deaths inside dungeons/raids and inside
 *                 battlegrounds/arenas are ignored, and PvP deaths never reach
 *                 this hook at all (they fire OnPlayerPVPKill instead).
 * Slow XP       : handled in MOA_Progression.cpp (OnPlayerGiveXP multiplier).
 */

#include "MOA.h"

#include "Chat.h"
#include "Creature.h"
#include "Item.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "ScriptMgr.h"

class MOA_ChallengeScript : public PlayerScript
{
public:
    MOA_ChallengeScript() : PlayerScript("MOA_ChallengeScript") { }

    void OnPlayerKilledByCreature(Creature* /*killer*/, Player* player) override
    {
        if (!sMOA->Enabled() || !sMOA->SemiHardcoreOn() || !player)
            return;

        if (!sMOA->PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE))
            return;

        if (player->IsGameMaster())
            return;

        Map* map = player->GetMap();
        if (!map)
            return;

        if (sMOA->ExcludeDungeon() && map->IsDungeon())
            return;

        if (sMOA->ExcludeBattleground() && map->IsBattlegroundOrArena())
            return;

        uint32 destroyed = 0;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;

            ChatHandler(player->GetSession()).PSendSysMessage(
                MOAPlayerText(player, MOAMsg::SemiLostItem),
                item->GetEntry(), item->GetTemplate()->Name1);

            player->DestroyItem(INVENTORY_SLOT_BAG_0, slot, true);
            ++destroyed;
        }

        uint32 const lostMoney = player->GetMoney();
        player->SetMoney(0);

        ChatHandler(player->GetSession()).PSendSysMessage(
            MOAPlayerText(player, MOAMsg::SemiDeath),
            destroyed, lostMoney);

        LOG_INFO("module", "Semi-Hardcore: player {} ({}) died in map {} and lost {} items, {} copper.",
            player->GetName(), player->GetGUID().ToString(), map->GetId(), destroyed, lostMoney);
    }
};

void AddMOAChallengeScript()
{
    new MOA_ChallengeScript();
}

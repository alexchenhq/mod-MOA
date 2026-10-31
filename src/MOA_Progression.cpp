/*
 * mod-moa — progression gating
 *
 * Instance gate : PlayerScript::OnPlayerCanEnterMap (MapMgr::PlayerCannotEnter only path)
 * Level gate    : OnPlayerCanGiveLevel (hard stop) + OnPlayerGiveXP (no xp piling)
 * Client cap    : PLAYER_FIELD_MAX_LEVEL via OnPlayerSetMaxLevel / OnPlayerLogin
 * Stage advance : creature kill credit on a configured stage boss
 */

#include "MOA.h"

#include "Chat.h"
#include "Creature.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <set>

class MOA_ProgressionScript : public PlayerScript
{
public:
    MOA_ProgressionScript() : PlayerScript("MOA_ProgressionScript") { }

    // ---------------------------------------------------------------- instances
    bool OnPlayerCanEnterMap(Player* player, MapEntry const* entry, InstanceTemplate const* /*instance*/,
                             MapDifficulty const* /*mapDiff*/, bool loginCheck) override
    {
        if (!sMOA->Enabled() || !entry || !entry->IsDungeon())
            return true;

        if (player->IsGameMaster())
            return true;

        // A character that logged out inside an instance which got locked later
        // (stage rollback) must still be able to log in. Refusing here would fail
        // the whole login instead of just blocking re-entry.
        if (loginCheck)
            return true;

        uint8 const needed = sMOA->InstanceStage(entry->MapID);
        if (needed == 0 || needed <= sMOA->CurrentStage())
            return true;

        if (sMOA->AnnounceLocked())
            ChatHandler(player->GetSession()).PSendSysMessage(
                MOAPlayerText(player, MOAMsg::InstanceLocked),
                uint32(needed), uint32(sMOA->CurrentStage()));

        return false;
    }

    // -------------------------------------------------------------------- levels
    bool OnPlayerCanGiveLevel(Player* player, uint8 newLevel) override
    {
        if (!sMOA->Enabled() || player->IsGameMaster())
            return true;

        return newLevel <= sMOA->MaxLevel();
    }

    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* /*victim*/, uint8 /*xpSource*/) override
    {
        if (!sMOA->Enabled() || amount == 0)
            return;

        // Level cap: stop experience from piling up at the cap.
        if (player->GetLevel() >= sMOA->MaxLevel())
        {
            amount = 0;
            return;
        }

        // Slow XP challenge.
        if (sMOA->ChallengeOn() && sMOA->PlayerHas(player, MOA_CHALLENGE_SLOW_XP))
            amount = std::max<uint32>(1, uint32(float(amount) * sMOA->ChallengeXpMultiplier()));
    }

    void OnPlayerSetMaxLevel(Player* /*player*/, uint32& maxPlayerLevel) override
    {
        if (sMOA->Enabled())
            maxPlayerLevel = std::min<uint32>(maxPlayerLevel, sMOA->MaxLevel());
    }

    void OnPlayerLogin(Player* player) override
    {
        if (!sMOA->Enabled())
            return;

        // InitStatsForLevel is not called on login, so the client-side cap has to
        // be refreshed here or the XP bar keeps showing the previous stage.
        player->SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, sMOA->MaxLevel());

        ChatHandler(player->GetSession()).PSendSysMessage(
            MOAPlayerText(player, MOAMsg::LoginStatus),
            uint32(sMOA->CurrentStage()), uint32(sMOA->StageCount()), sMOA->MaxLevel());
    }

    // ------------------------------------------------------------- stage bosses
    void OnPlayerCreatureKillCredit(Player* player, Creature* killed) override
    {
        if (!sMOA->Enabled() || !killed)
            return;

        Map* map = killed->GetMap();
        if (!map || !map->IsDungeon())
            return;

        sMOA->OnBossKilled(killed->GetEntry(), player);
    }

    // ------------------------------------------------------------------- RDF
    bool OnPlayerCanJoinLfg(Player* player, uint8 /*roles*/, std::set<uint32>& /*dungeons*/,
                            std::string const& /*comment*/) override
    {
        if (!sMOA->Enabled() || !sMOA->DisableRdf() || player->IsGameMaster())
            return true;

        ChatHandler(player->GetSession()).PSendSysMessage(
            MOAPlayerText(player, MOAMsg::RdfDisabled));

        return false;
    }
};

void AddMOAProgressionScript()
{
    new MOA_ProgressionScript();
}

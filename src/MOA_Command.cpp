/*
 * mod-moa — chat commands
 *
 * .moa                     status
 * .moa stage               stage + boss progress
 * .moa stage set <n>       GM
 * .moa stage next          GM
 * .moa challenge           own challenge status
 * .moa challenge on semi   enable semi-hardcore (level 1 / DK 55)
 * .moa challenge on slow   enable slow xp
 * .moa reload              GM
 */

#include "MOA.h"

#include "Chat.h"
#include "ChatCommand.h"
#include "DatabaseEnv.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <cctype>
#include <string>

using namespace Acore::ChatCommands;

class MOA_CommandScript : public CommandScript
{
public:
    MOA_CommandScript() : CommandScript("MOA_CommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable stageTable =
        {
            { "",     HandleInfo,      SEC_PLAYER,     Console::Yes },
            { "set",  HandleStageSet,  SEC_GAMEMASTER, Console::Yes },
            { "next", HandleStageNext, SEC_GAMEMASTER, Console::Yes },
        };

        static ChatCommandTable challengeTable =
        {
            { "",   HandleInfo,        SEC_PLAYER, Console::No },
            { "on", HandleChallengeOn, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable moaTable =
        {
            { "",          HandleInfo,      SEC_PLAYER,     Console::Yes },
            { "stage",     stageTable },
            { "challenge", challengeTable },
            { "reload",    HandleReload,    SEC_GAMEMASTER, Console::Yes },
        };

        static ChatCommandTable root = { { "moa", moaTable } };
        return root;
    }

private:
    static Player* GetSessionPlayer(ChatHandler* handler)
    {
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    //! Console commands have no session: fall back to the broadcast language.
    static char const* CmdText(ChatHandler* handler, MOALocaleText const& text)
    {
        if (Player* player = GetSessionPlayer(handler))
            return MOAPlayerText(player, text);

        return MOALocalized(sMOA->Language(), text);
    }

    static uint32 CountKilled(uint8 stage)
    {
        if (QueryResult result = WorldDatabase.Query("SELECT COUNT(*) FROM moa_boss_kill WHERE stage = {}", uint32(stage)))
            return (*result)[0].Get<uint32>();

        return 0;
    }

    static bool HandleInfo(ChatHandler* handler)
    {
        uint32 unlocked = 0;
        for (auto const& [mapId, stage] : sMOA->InstanceUnlocks())
        {
            (void)mapId;
            if (stage <= sMOA->CurrentStage())
                ++unlocked;
        }

        handler->PSendSysMessage(CmdText(handler, MOAMsg::InfoStatus),
            uint32(sMOA->CurrentStage()), uint32(sMOA->StageCount()), sMOA->MaxLevel(),
            unlocked, uint32(sMOA->InstanceUnlocks().size()));

        auto const stageIt = sMOA->StageBosses().find(sMOA->CurrentStage());
        if (stageIt != sMOA->StageBosses().end() && !stageIt->second.base.empty())
        {
            uint32 const killed = std::min<uint32>(CountKilled(sMOA->CurrentStage()), uint32(stageIt->second.base.size()));
            handler->PSendSysMessage(CmdText(handler, MOAMsg::InfoBossProgress),
                killed, uint32(stageIt->second.base.size()));
        }
        else
            handler->PSendSysMessage(CmdText(handler, MOAMsg::InfoNoBoss));

        Player* player = GetSessionPlayer(handler);
        if (player)
        {
            MOALanguage const lang = MOAPlayerLanguage(player);
            handler->PSendSysMessage(CmdText(handler, MOAMsg::InfoChallenges),
                sMOA->PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE)
                    ? MOALocalized(lang, MOAMsg::StateOn) : MOALocalized(lang, MOAMsg::StateOff),
                sMOA->PlayerHas(player, MOA_CHALLENGE_SLOW_XP)
                    ? MOALocalized(lang, MOAMsg::StateOn) : MOALocalized(lang, MOAMsg::StateOff));
        }

        return true;
    }

    static bool HandleStageSet(ChatHandler* handler, uint32 stage)
    {
        if (stage < 1 || stage > sMOA->StageCount())
        {
            handler->PSendSysMessage(CmdText(handler, MOAMsg::StageRange), uint32(sMOA->StageCount()));
            return true;
        }

        uint8 const before = sMOA->CurrentStage();
        sMOA->SetStage(uint8(stage), true);

        handler->PSendSysMessage(CmdText(handler, MOAMsg::StageSetOk),
            uint32(before), uint32(sMOA->CurrentStage()), sMOA->MaxLevel());

        return true;
    }

    static bool HandleStageNext(ChatHandler* handler)
    {
        if (sMOA->CurrentStage() >= sMOA->StageCount())
        {
            handler->PSendSysMessage(CmdText(handler, MOAMsg::StageLast), uint32(sMOA->StageCount()));
            return true;
        }

        uint8 const next = uint8(sMOA->CurrentStage() + 1);
        sMOA->SetStage(next, true);
        handler->PSendSysMessage(CmdText(handler, MOAMsg::StageNextOk), uint32(next), sMOA->MaxLevel());

        return true;
    }

    static bool HandleChallengeOn(ChatHandler* handler, std::string mode)
    {
        Player* player = GetSessionPlayer(handler);
        if (!player)
            return false;

        std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return char(std::tolower(c)); });

        bool const isSemi = mode == "semi" || mode == "semi-hardcore" || mode == "semihardcore";
        bool const isSlow = mode == "slow" || mode == "slowxp" || mode == "slow-xp";

        if (!isSemi && !isSlow)
        {
            handler->PSendSysMessage(CmdText(handler, MOAMsg::ChallengeUsage));
            return true;
        }

        bool const levelOk = player->GetLevel() == 1
            || (player->getClass() == CLASS_DEATH_KNIGHT && player->GetLevel() <= 55);

        if (!levelOk)
        {
            handler->PSendSysMessage(CmdText(handler, MOAMsg::ChallengeLevel));
            return true;
        }

        if (isSemi)
        {
            if (!sMOA->SemiHardcoreOn())
            {
                handler->PSendSysMessage(CmdText(handler, MOAMsg::SemiDisabled));
                return true;
            }

            if (sMOA->PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE))
            {
                handler->PSendSysMessage(CmdText(handler, MOAMsg::SemiAlready));
                return true;
            }

            sMOA->GrantChallenge(player, MOA_CHALLENGE_SEMI_HARDCORE);
            handler->PSendSysMessage(CmdText(handler, MOAMsg::SemiEnabled));
        }
        else
        {
            if (!sMOA->SlowXpOn())
            {
                handler->PSendSysMessage(CmdText(handler, MOAMsg::SlowDisabled));
                return true;
            }

            if (sMOA->PlayerHas(player, MOA_CHALLENGE_SLOW_XP))
            {
                handler->PSendSysMessage(CmdText(handler, MOAMsg::SlowAlready));
                return true;
            }

            sMOA->GrantChallenge(player, MOA_CHALLENGE_SLOW_XP);
            handler->PSendSysMessage(CmdText(handler, MOAMsg::SlowEnabled),
                uint32(sMOA->SlowXpMultiplier() * 100.0f));
        }

        return true;
    }

    static bool HandleReload(ChatHandler* handler)
    {
        sMOA->LoadConfig(true);
        sMOA->LoadData();

        handler->PSendSysMessage(CmdText(handler, MOAMsg::ReloadOk), uint32(sMOA->CurrentStage()));
        return true;
    }
};

void AddMOACommandScript()
{
    new MOA_CommandScript();
}

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
            { "set",  HandleStageSet,  SEC_GAMEMASTER, Console::Yes },
            { "next", HandleStageNext, SEC_GAMEMASTER, Console::Yes },
        };

        static ChatCommandTable challengeTable =
        {
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

        handler->PSendSysMessage("|cff00ccff[MOA]|r 阶段 {}/{}，等级上限 {}，已解锁副本 {}/{}。",
            uint32(sMOA->CurrentStage()), uint32(sMOA->StageCount()), sMOA->MaxLevel(),
            unlocked, uint32(sMOA->InstanceUnlocks().size()));

        auto const stageIt = sMOA->StageBosses().find(sMOA->CurrentStage());
        if (stageIt != sMOA->StageBosses().end() && !stageIt->second.base.empty())
        {
            uint32 const killed = std::min<uint32>(CountKilled(sMOA->CurrentStage()), uint32(stageIt->second.base.size()));
            handler->PSendSysMessage("|cff00ccff[MOA]|r 本阶段尾王：{}/{}。全部击杀后自动进入下一阶段。",
                killed, uint32(stageIt->second.base.size()));
        }
        else
            handler->PSendSysMessage("|cff00ccff[MOA]|r 本阶段没有配置尾王，需要 GM 手动推进。");

        Player* player = GetSessionPlayer(handler);
        if (player)
        {
            handler->PSendSysMessage("|cff00ccff[MOA]|r 你的挑战 —— 半硬核：{}，慢速经验：{}。",
                sMOA->PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE) ? "开" : "关",
                sMOA->PlayerHas(player, MOA_CHALLENGE_SLOW_XP) ? "开" : "关");
        }

        return true;
    }

    static bool HandleStageSet(ChatHandler* handler, uint32 stage)
    {
        if (stage < 1 || stage > sMOA->StageCount())
        {
            handler->PSendSysMessage("|cffff8000[MOA]|r 阶段必须在 1..{} 之间。", uint32(sMOA->StageCount()));
            return true;
        }

        uint8 const before = sMOA->CurrentStage();
        sMOA->SetStage(uint8(stage), true);

        handler->PSendSysMessage("|cff00ccff[MOA]|r 阶段 {} -> {}，等级上限 {}。",
            uint32(before), uint32(sMOA->CurrentStage()), sMOA->MaxLevel());

        return true;
    }

    static bool HandleStageNext(ChatHandler* handler)
    {
        if (sMOA->CurrentStage() >= sMOA->StageCount())
        {
            handler->PSendSysMessage("|cffff8000[MOA]|r 已经是最后阶段（{}）。", uint32(sMOA->StageCount()));
            return true;
        }

        uint8 const next = uint8(sMOA->CurrentStage() + 1);
        sMOA->SetStage(next, true);
        handler->PSendSysMessage("|cff00ccff[MOA]|r 已推进到阶段 {}，等级上限 {}。", uint32(next), sMOA->MaxLevel());

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
            handler->PSendSysMessage("|cffff8000[MOA]|r 用法：.moa challenge on semi | slow");
            return true;
        }

        bool const levelOk = player->GetLevel() == 1
            || (player->getClass() == CLASS_DEATH_KNIGHT && player->GetLevel() <= 55);

        if (!levelOk)
        {
            handler->PSendSysMessage("|cffff8000[MOA]|r 挑战只能在 1 级（死亡骑士 55 级）时开启。");
            return true;
        }

        if (isSemi)
        {
            if (!sMOA->SemiHardcoreOn())
            {
                handler->PSendSysMessage("|cffff8000[MOA]|r 半硬核模式已在配置中关闭。");
                return true;
            }

            if (sMOA->PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE))
            {
                handler->PSendSysMessage("|cffff8000[MOA]|r 你已开启半硬核模式。");
                return true;
            }

            sMOA->GrantChallenge(player, MOA_CHALLENGE_SEMI_HARDCORE);
            handler->PSendSysMessage("|cffDA70D6[MOA]|r 半硬核已开启：野外被怪物杀死将失去全部装备和金币。副本、战场、竞技场及 PvP 死亡不触发。此模式无法关闭。");
        }
        else
        {
            if (!sMOA->SlowXpOn())
            {
                handler->PSendSysMessage("|cffff8000[MOA]|r 慢速经验已在配置中关闭。");
                return true;
            }

            if (sMOA->PlayerHas(player, MOA_CHALLENGE_SLOW_XP))
            {
                handler->PSendSysMessage("|cffff8000[MOA]|r 你已开启慢速经验。");
                return true;
            }

            sMOA->GrantChallenge(player, MOA_CHALLENGE_SLOW_XP);
            handler->PSendSysMessage("|cffDA70D6[MOA]|r 慢速经验已开启：经验获取为正常的 {}%。", uint32(sMOA->SlowXpMultiplier() * 100.0f));
        }

        return true;
    }

    static bool HandleReload(ChatHandler* handler)
    {
        sMOA->LoadConfig(true);
        sMOA->LoadData();

        handler->PSendSysMessage("|cff00ccff[MOA]|r 配置与阶段数据已重新载入（当前阶段 {}）。", uint32(sMOA->CurrentStage()));
        return true;
    }
};

void AddMOACommandScript()
{
    new MOA_CommandScript();
}

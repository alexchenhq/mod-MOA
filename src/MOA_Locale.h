/*
 * mod-moa — localisation
 *
 * Player-facing messages follow the client's DBC locale (enUS / zhCN, zhTW
 * falls back to zhCN). Server-wide broadcasts follow MOA.Locale, which defaults
 * to zhCN and can be set to enUS or "auto" (server DBC locale).
 *
 * Console log lines stay English on purpose — the Windows console code page is
 * not UTF-8 and Chinese would be mangled there.
 */

#ifndef MOD_MOA_LOCALE_H
#define MOD_MOA_LOCALE_H

#include "Common.h"
#include "Player.h"
#include "WorldSession.h"

#include <string>

enum MOALanguage : uint8
{
    MOA_LANG_EN = 0,
    MOA_LANG_ZH = 1,
};

struct MOALocaleText
{
    char const* en;
    char const* zh;
};

inline MOALanguage MOALanguageFromLocale(LocaleConstant locale)
{
    switch (locale)
    {
        case LOCALE_zhCN:
        case LOCALE_zhTW:
            return MOA_LANG_ZH;
        default:
            return MOA_LANG_EN;
    }
}

inline LocaleConstant MOALocaleFromLanguage(MOALanguage lang)
{
    return lang == MOA_LANG_ZH ? LOCALE_zhCN : LOCALE_enUS;
}

inline char const* MOALocalized(MOALanguage lang, MOALocaleText const& text)
{
    return lang == MOA_LANG_ZH ? text.zh : text.en;
}

inline MOALanguage MOAPlayerLanguage(Player* player)
{
    if (player && player->GetSession())
        return MOALanguageFromLocale(player->GetSession()->GetSessionDbLocaleIndex());

    return MOA_LANG_EN;
}

inline char const* MOAPlayerText(Player* player, MOALocaleText const& text)
{
    return MOALocalized(MOAPlayerLanguage(player), text);
}

namespace MOAMsg
{
    // ---------------------------------------------------------------- progression
    constexpr MOALocaleText InstanceLocked = {
        "|cffff8000[MOA]|r This instance is locked: requires stage {}, you are on stage {}.",
        "|cffff8000[MOA]|r 该副本尚未解锁：需要阶段 {}，当前阶段 {}。"
    };

    constexpr MOALocaleText LoginStatus = {
        "|cff00ccff[MOA]|r Stage {}/{}, level cap {}.",
        "|cff00ccff[MOA]|r 当前阶段 {}/{}，等级上限 {}。"
    };

    constexpr MOALocaleText RdfDisabled = {
        "|cffff8000[MOA]|r The dungeon finder is disabled. Travel to the instance entrance.",
        "|cffff8000[MOA]|r 随机副本已停用，请自行前往副本门口。"
    };

    // ------------------------------------------------------------------ challenge
    constexpr MOALocaleText SemiLostItem = {
        "|cffDA70D6[Semi-Hardcore]|r You lost |cffffffff|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r",
        "|cffDA70D6[半硬核]|r 你失去了 |cffffffff|Hitem:{}:0:0:0:0:0:0:0:0|h[{}]|h|r"
    };

    constexpr MOALocaleText SemiDeath = {
        "|cffDA70D6[Semi-Hardcore]|r You fell in the open world: lost {} items and {} copper.",
        "|cffDA70D6[半硬核]|r 你在野外倒下了：失去 {} 件装备、{} 铜。"
    };

    // -------------------------------------------------------------------- commands
    constexpr MOALocaleText InfoStatus = {
        "|cff00ccff[MOA]|r Stage {}/{}, level cap {}, instances unlocked {}/{}.",
        "|cff00ccff[MOA]|r 阶段 {}/{}，等级上限 {}，已解锁副本 {}/{}。"
    };

    constexpr MOALocaleText InfoBossProgress = {
        "|cff00ccff[MOA]|r Stage bosses: {}/{}. Kill them all to advance.",
        "|cff00ccff[MOA]|r 本阶段尾王：{}/{}。全部击杀后自动进入下一阶段。"
    };

    constexpr MOALocaleText InfoNoBoss = {
        "|cff00ccff[MOA]|r No stage bosses configured; a GM has to advance manually.",
        "|cff00ccff[MOA]|r 本阶段没有配置尾王，需要 GM 手动推进。"
    };

    constexpr MOALocaleText InfoChallenges = {
        "|cff00ccff[MOA]|r Your challenges - semi-hardcore: {}, slow xp: {}.",
        "|cff00ccff[MOA]|r 你的挑战 —— 半硬核：{}，慢速经验：{}。"
    };

    constexpr MOALocaleText StateOn  = { "on",  "开" };
    constexpr MOALocaleText StateOff = { "off", "关" };

    constexpr MOALocaleText StageRange = {
        "|cffff8000[MOA]|r Stage must be between 1 and {}.",
        "|cffff8000[MOA]|r 阶段必须在 1..{} 之间。"
    };

    constexpr MOALocaleText StageSetOk = {
        "|cff00ccff[MOA]|r Stage {} -> {}, level cap {}.",
        "|cff00ccff[MOA]|r 阶段 {} -> {}，等级上限 {}。"
    };

    constexpr MOALocaleText StageLast = {
        "|cffff8000[MOA]|r Already at the final stage ({}).",
        "|cffff8000[MOA]|r 已经是最后阶段（{}）。"
    };

    constexpr MOALocaleText StageNextOk = {
        "|cff00ccff[MOA]|r Advanced to stage {}, level cap {}.",
        "|cff00ccff[MOA]|r 已推进到阶段 {}，等级上限 {}。"
    };

    constexpr MOALocaleText ChallengeUsage = {
        "|cffff8000[MOA]|r Usage: .moa challenge on semi | slow",
        "|cffff8000[MOA]|r 用法：.moa challenge on semi | slow"
    };

    constexpr MOALocaleText ChallengeLevel = {
        "|cffff8000[MOA]|r Challenges can only be enabled at level 1 (death knight: 55 or lower).",
        "|cffff8000[MOA]|r 挑战只能在 1 级（死亡骑士 55 级）时开启。"
    };

    constexpr MOALocaleText SemiDisabled = {
        "|cffff8000[MOA]|r Semi-hardcore is disabled in the configuration.",
        "|cffff8000[MOA]|r 半硬核模式已在配置中关闭。"
    };

    constexpr MOALocaleText SemiAlready = {
        "|cffff8000[MOA]|r Semi-hardcore is already enabled.",
        "|cffff8000[MOA]|r 你已开启半硬核模式。"
    };

    constexpr MOALocaleText SemiEnabled = {
        "|cffDA70D6[MOA]|r Semi-hardcore enabled: dying to a creature in the open world costs all equipped items and carried gold. Dungeons, battlegrounds, arenas and PvP deaths do not trigger it. This cannot be undone.",
        "|cffDA70D6[MOA]|r 半硬核已开启：野外被怪物杀死将失去全部装备和金币。副本、战场、竞技场及 PvP 死亡不触发。此模式无法关闭。"
    };

    constexpr MOALocaleText SlowDisabled = {
        "|cffff8000[MOA]|r Slow XP is disabled in the configuration.",
        "|cffff8000[MOA]|r 慢速经验已在配置中关闭。"
    };

    constexpr MOALocaleText SlowAlready = {
        "|cffff8000[MOA]|r Slow XP is already enabled.",
        "|cffff8000[MOA]|r 你已开启慢速经验。"
    };

    constexpr MOALocaleText SlowEnabled = {
        "|cffDA70D6[MOA]|r Slow XP enabled: experience gain is {}% of normal.",
        "|cffDA70D6[MOA]|r 慢速经验已开启：经验获取为正常的 {}%。"
    };

    constexpr MOALocaleText ReloadOk = {
        "|cff00ccff[MOA]|r Configuration and stage data reloaded (current stage {}).",
        "|cff00ccff[MOA]|r 配置与阶段数据已重新载入（当前阶段 {}）。"
    };

    // ------------------------------------------------------------------- broadcasts
    constexpr MOALocaleText StageOpened = {
        "|cff00ccff[MOA]|r Stage {} is now open: level cap {}, new instances unlocked.",
        "|cff00ccff[MOA]|r 阶段 {} 已开启：等级上限 {}，新的副本已解锁。"
    };

    constexpr MOALocaleText StageRolledBack = {
        "|cffff6600[MOA]|r Stage rolled back to {}: level cap {}.",
        "|cffff6600[MOA]|r 阶段已回退至 {}：等级上限 {}。"
    };

    constexpr MOALocaleText BossKilled = {
        "|cff00ccff[MOA]|r Stage boss {} defeated ({}/{}).",
        "|cff00ccff[MOA]|r 关卡尾王 {} 已被击杀（{}/{}）。"
    };

    constexpr MOALocaleText AllStagesDone = {
        "|cffffd700[MOA]|r The final stage is complete. The world ends here.",
        "|cffffd700[MOA]|r 最终阶段已完成。世界走到了尽头。"
    };

    // ---------------------------------------------------------------------- rewards
    constexpr MOALocaleText RewardMailSubject = {
        "Challenge reward: level {}",
        "挑战奖励：{} 级"
    };

    constexpr MOALocaleText RewardMailBody = {
        "Your challenge character reached level {}. Take your reward from the attachment, and keep going.",
        "你的挑战角色已达到 {} 级。附件是这一档的奖励，继续往前走吧。"
    };
}

#endif // MOD_MOA_LOCALE_H

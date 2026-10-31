/*
 * mod-moa — MOAMgr implementation
 */

#include "MOA.h"

#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "StringFormat.h"
#include "World.h"
#include "WorldSessionMgr.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace
{
    constexpr char const* MOA_SOURCE = "mod-moa";

    std::vector<uint32> ParseLevelList(std::string const& csv)
    {
        std::vector<uint32> out;
        std::stringstream stream(csv);
        std::string item;
        while (std::getline(stream, item, ','))
        {
            size_t const first = item.find_first_not_of(" \t");
            if (first == std::string::npos)
                continue;

            size_t const last = item.find_last_not_of(" \t");
            out.push_back(uint32(std::strtoul(item.substr(first, last - first + 1).c_str(), nullptr, 10)));
        }
        return out;
    }
}

MOAMgr* MOAMgr::instance()
{
    static MOAMgr instance;
    return &instance;
}

void MOAMgr::LoadConfig(bool reload)
{
    _enabled        = sConfigMgr->GetOption<bool>("MOA.Enable", true);
    _announceLocked = sConfigMgr->GetOption<bool>("MOA.AnnounceLockedInstance", true);
    _disableRdf     = sConfigMgr->GetOption<bool>("MOA.DisableRDF", true);

    _challenge      = sConfigMgr->GetOption<bool>("MOA.Challenge.Enable", true);
    _excludeDungeon = sConfigMgr->GetOption<bool>("MOA.Challenge.ExcludeDungeon", true);
    _excludeBg      = sConfigMgr->GetOption<bool>("MOA.Challenge.ExcludeBattleground", true);

    _challengeXpMul = sConfigMgr->GetOption<float>("MOA.Challenge.XpMultiplier", 0.7f);
    if (_challengeXpMul <= 0.0f)
        _challengeXpMul = 0.7f;

    _maxLevels = ParseLevelList(sConfigMgr->GetOption<std::string>("MOA.MaxLevels", "60,60,60,70,70,70,70,80,80,80,80,80"));
    if (_maxLevels.empty())
    {
        LOG_ERROR("module", "MOA.MaxLevels is empty or invalid, falling back to 80 for every stage.");
        _maxLevels.push_back(80);
    }

    std::string const locale = sConfigMgr->GetOption<std::string>("MOA.Locale", "zhCN");
    if (locale == "enUS" || locale == "en")
        _language = MOA_LANG_EN;
    else if (locale == "auto")
        _language = MOALanguageFromLocale(sWorld->GetDefaultDbcLocale());
    else
        _language = MOA_LANG_ZH;

    // A reload may shrink the stage list (MOA.MaxLevels); keep the pointer valid.
    if (_currentStage < 1)
        _currentStage = 1;
    else if (_currentStage > StageCount())
        _currentStage = StageCount();

    if (reload)
        LOG_INFO("module", "Configuration reloaded ({} stages, current stage {}).", uint32(_maxLevels.size()), uint32(_currentStage));
}

void MOAMgr::LoadData()
{
    _instanceStage.clear();
    _stageBosses.clear();
    _bossBase.clear();

    if (QueryResult result = WorldDatabase.Query("SELECT map_id, stage FROM moa_instance_unlock"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 const mapId = fields[0].Get<uint32>();
            uint8 const stage  = fields[1].Get<uint8>();
            if (stage == 0)
                continue;
            _instanceStage[mapId] = stage;
        } while (result->NextRow());
    }
    else
        LOG_ERROR("module", "Table `moa_instance_unlock` is missing or unreadable. No instance will be gated.");

    if (QueryResult result = WorldDatabase.Query("SELECT stage, boss_entry FROM moa_stage_boss ORDER BY stage, boss_entry"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint8 const stage = fields[0].Get<uint8>();
            uint32 const boss = fields[1].Get<uint32>();
            if (stage == 0 || boss == 0)
                continue;
            _stageBosses[stage].base.insert(boss);
        } while (result->NextRow());
    }
    else
        LOG_ERROR("module", "Table `moa_stage_boss` is missing or unreadable. Stage progression will never complete automatically.");

    for (auto& [stage, info] : _stageBosses)
        for (uint32 base : info.base)
            CollectBoss(base, base, info);

    LOG_INFO("module", "Loaded {} gated instances and {} stages with stage bosses.",
        uint32(_instanceStage.size()), uint32(_stageBosses.size()));
}

void MOAMgr::LoadState()
{
    if (QueryResult result = WorldDatabase.Query("SELECT current_stage FROM moa_state WHERE id = 1"))
        _currentStage = std::max<uint8>(1, (*result)[0].Get<uint8>());
    else
    {
        _currentStage = std::max<uint8>(1, uint8(sConfigMgr->GetOption<uint32>("MOA.StartStage", 1)));
        WorldDatabase.Execute("INSERT INTO moa_state (id, current_stage, updated_at) VALUES (1, {}, NOW()) "
                              "ON DUPLICATE KEY UPDATE current_stage = VALUES(current_stage)",
            uint32(_currentStage));
        LOG_INFO("module", "Initialised moa_state at stage {}.", uint32(_currentStage));
    }

    if (_currentStage > StageCount())
        _currentStage = StageCount();
    LOG_INFO("module", "Current stage: {} / {} (level cap {}).",
        uint32(_currentStage), uint32(StageCount()), MaxLevel());
}

uint32 MOAMgr::MaxLevelOfStage(uint8 stage) const
{
    if (_maxLevels.empty())
        return 80;

    size_t index = stage == 0 ? 0 : size_t(stage - 1);
    if (index >= _maxLevels.size())
        index = _maxLevels.size() - 1;

    return _maxLevels[index];
}

uint8 MOAMgr::InstanceStage(uint32 mapId) const
{
    auto const it = _instanceStage.find(mapId);
    return it == _instanceStage.end() ? 0 : it->second;
}

bool MOAMgr::IsInstanceUnlocked(uint32 mapId) const
{
    uint8 const needed = InstanceStage(mapId);
    return needed == 0 || needed <= _currentStage;
}

uint32 MOAMgr::BaseBossOf(uint32 entry) const
{
    auto const it = _bossBase.find(entry);
    return it == _bossBase.end() ? 0 : it->second;
}

void MOAMgr::CollectBoss(uint32 entry, uint32 base, MOAStageBosses& out)
{
    if (entry == 0 || !out.expanded.insert(entry).second)
        return;

    _bossBase[entry] = base;

    CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(entry);
    if (!tmpl)
        return;

    for (uint8 i = 0; i < MAX_DIFFICULTY - 1; ++i)
        if (tmpl->DifficultyEntry[i] != 0)
            CollectBoss(tmpl->DifficultyEntry[i], base, out);
}

void MOAMgr::CollectKills(uint8 stage, std::unordered_set<uint32>& out) const
{
    out.clear();
    if (QueryResult result = WorldDatabase.Query("SELECT boss_entry FROM moa_boss_kill WHERE stage = {}", uint32(stage)))
    {
        do
            out.insert((*result)[0].Get<uint32>());
        while (result->NextRow());
    }
}

void MOAMgr::SetStage(uint8 stage, bool announce)
{
    if (stage == 0 || stage > StageCount())
    {
        LOG_ERROR("module", "Refusing to set invalid stage {} (valid range 1..{}).", uint32(stage), uint32(StageCount()));
        return;
    }

    uint8 const previous = _currentStage;
    _currentStage = stage;

    WorldDatabase.Execute("UPDATE moa_state SET current_stage = {}, updated_at = NOW() WHERE id = 1", uint32(stage));

    RefreshOnlinePlayers();

    if (!announce)
        return;

    if (stage > previous)
        Broadcast(Acore::StringFormat(MOALocalized(Language(), MOAMsg::StageOpened),
            uint32(stage), MaxLevel()));
    else
        Broadcast(Acore::StringFormat(MOALocalized(Language(), MOAMsg::StageRolledBack),
            uint32(stage), MaxLevel()));

    LOG_INFO("module", "Stage changed {} -> {} (level cap {}).", uint32(previous), uint32(stage), MaxLevel());
}

void MOAMgr::OnBossKilled(uint32 entry, Player* killer)
{
    if (!_enabled || entry == 0)
        return;

    uint32 const base = BaseBossOf(entry);
    if (base == 0)
        return;

    auto const stageIt = _stageBosses.find(_currentStage);
    if (stageIt == _stageBosses.end() || stageIt->second.base.count(base) == 0)
        return; // boss of a later/earlier stage: ignore

    if (WorldDatabase.Query("SELECT 1 FROM moa_boss_kill WHERE stage = {} AND boss_entry = {}", uint32(_currentStage), base))
        return; // already credited

    WorldDatabase.Execute("INSERT IGNORE INTO moa_boss_kill (stage, boss_entry, killed_at, killed_by) VALUES ({}, {}, NOW(), '{}')",
        uint32(_currentStage), base, killer ? killer->GetName() : "unknown");

    std::unordered_set<uint32> killed;
    CollectKills(_currentStage, killed);

    size_t const total = stageIt->second.base.size();
    size_t done = 0;
    for (uint32 boss : stageIt->second.base)
        if (killed.count(boss))
            ++done;

    Broadcast(Acore::StringFormat(MOALocalized(Language(), MOAMsg::BossKilled),
        BossName(base, Language()), uint32(done), uint32(total)));

    if (done < total)
        return;

    if (_currentStage >= StageCount())
    {
        Broadcast(MOALocalized(Language(), MOAMsg::AllStagesDone));
        return;
    }

    uint8 const completedStage = _currentStage;
    SetStage(uint8(completedStage + 1), true);
    WorldDatabase.Execute("DELETE FROM moa_boss_kill WHERE stage = {}", uint32(completedStage));}

void MOAMgr::RefreshOnlinePlayers()
{
    uint32 const cap = MaxLevel();
    for (auto const& [guid, session] : sWorldSessionMgr->GetAllSessions())
    {
        (void)guid;
        Player* player = session ? session->GetPlayer() : nullptr;
        if (!player)
            continue;

        player->SetUInt32Value(PLAYER_FIELD_MAX_LEVEL, cap);
    }
}

void MOAMgr::Broadcast(std::string const& text) const
{
    ChatHandler(nullptr).SendWorldText(text);
}

std::string MOAMgr::BossName(uint32 entry, MOALanguage lang) const
{
    uint32 const base = BaseBossOf(entry) ? BaseBossOf(entry) : entry;
    LocaleConstant const locale = MOALocaleFromLanguage(lang);

    if (CreatureLocale const* creatureLocale = sObjectMgr->GetCreatureLocale(base))
        if (size_t(locale) < creatureLocale->Name.size() && !creatureLocale->Name[locale].empty())
            return creatureLocale->Name[locale];

    if (CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(base))
        return tmpl->Name;

    return Acore::StringFormat("entry {}", base);
}

bool MOAMgr::PlayerHas(Player* player, MOAChallenge challenge) const
{
    if (!player)
        return false;

    return player->GetPlayerSetting(MOA_SOURCE, challenge).value != 0;
}

bool MOAMgr::HasFullChallenge(Player* player) const
{
    if (!player)
        return false;

    return PlayerHas(player, MOA_CHALLENGE_SEMI_HARDCORE) && PlayerHas(player, MOA_CHALLENGE_SLOW_XP);
}

void MOAMgr::GrantChallenge(Player* player, MOAChallenge challenge)
{
    if (!player || PlayerHas(player, challenge))
        return;

    player->UpdatePlayerSetting(MOA_SOURCE, challenge, 1);
}

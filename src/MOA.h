/*
 * mod-moa — My Own Azeroth
 * Progression + challenge modes for a personal AzerothCore server.
 *
 * Design rule: this module never touches spells, talents, items or creature
 * data. It only gates instance entry and level gain, and adds two opt-in
 * per-character challenges. That keeps it fully compatible with mod-playerbots.
 */

#ifndef MOD_MOA_H
#define MOD_MOA_H

#include "Define.h"
#include "MOA_Locale.h"
#include "Player.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class ChatHandler;

enum MOAChallenge : uint32
{
    MOA_CHALLENGE_SEMI_HARDCORE = 1,
    MOA_CHALLENGE_SLOW_XP       = 2,
};

struct MOAStageBosses
{
    std::unordered_set<uint32> base;     // entries configured in moa_stage_boss
    std::unordered_set<uint32> expanded; // base + all difficulty variants
};

class MOAMgr
{
public:
    static MOAMgr* instance();

    void LoadConfig(bool reload);
    void LoadData();
    void LoadState();

    [[nodiscard]] bool   Enabled()      const { return _enabled; }
    [[nodiscard]] uint8  CurrentStage() const { return _currentStage; }
    [[nodiscard]] uint8  StageCount()   const { return uint8(_maxLevels.size()); }
    [[nodiscard]] uint32 MaxLevel()     const { return MaxLevelOfStage(_currentStage); }
    [[nodiscard]] uint32 MaxLevelOfStage(uint8 stage) const;

    //! 0 means "not configured" -> no restriction.
    [[nodiscard]] uint8  InstanceStage(uint32 mapId) const;
    [[nodiscard]] bool   IsInstanceUnlocked(uint32 mapId) const;
    [[nodiscard]] uint32 BaseBossOf(uint32 entry) const;

    void SetStage(uint8 stage, bool announce);
    void OnBossKilled(uint32 entry, Player* killer);
    void RefreshOnlinePlayers();
    void Broadcast(std::string const& text) const;

    [[nodiscard]] bool PlayerHas(Player* player, MOAChallenge challenge) const;
    //! Challenge mode is all-or-nothing: both flags must be set.
    [[nodiscard]] bool HasFullChallenge(Player* player) const;
    void GrantChallenge(Player* player, MOAChallenge challenge);

    [[nodiscard]] bool  ChallengeOn()            const { return _challenge; }
    [[nodiscard]] float ChallengeXpMultiplier()  const { return _challengeXpMul; }
    [[nodiscard]] bool  ExcludeDungeon()         const { return _excludeDungeon; }
    [[nodiscard]] bool  ExcludeBattleground()    const { return _excludeBg; }
    [[nodiscard]] bool  DisableRdf()             const { return _disableRdf; }
    [[nodiscard]] bool  AnnounceLocked()         const { return _announceLocked; }

    [[nodiscard]] std::unordered_map<uint32, uint8> const& InstanceUnlocks() const { return _instanceStage; }
    [[nodiscard]] std::unordered_map<uint8, MOAStageBosses> const& StageBosses() const { return _stageBosses; }
    [[nodiscard]] std::string BossName(uint32 entry, MOALanguage lang) const;

    //! Language used for server-wide broadcasts (player messages follow the client locale).
    [[nodiscard]] MOALanguage Language() const { return _language; }

private:
    void CollectBoss(uint32 entry, uint32 base, MOAStageBosses& out);
    void CollectKills(uint8 stage, std::unordered_set<uint32>& out) const;

    bool  _enabled          = true;
    bool  _announceLocked   = true;
    bool  _disableRdf       = true;
    bool  _challenge        = true;
    bool  _excludeDungeon   = true;
    bool  _excludeBg        = true;
    float _challengeXpMul   = 0.7f;

    uint8               _currentStage = 1;
    std::vector<uint32> _maxLevels;
    MOALanguage         _language     = MOA_LANG_ZH;

    std::unordered_map<uint32, uint8>         _instanceStage; // mapId -> stage
    std::unordered_map<uint8, MOAStageBosses> _stageBosses;   // stage -> bosses
    std::unordered_map<uint32, uint32>        _bossBase;      // any entry -> base entry
};

#define sMOA MOAMgr::instance()

#endif // MOD_MOA_H

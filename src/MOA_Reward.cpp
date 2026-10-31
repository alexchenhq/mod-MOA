/*
 * mod-moa — challenge rewards
 *
 * Only characters that enabled at least one challenge get these, and the values
 * are intentionally hard-coded (no config): they are part of the challenge
 * design, not tuning knobs.
 *
 *   talent points : +1 at 15/25/35/45/55, +2 at 65/75 (on top of the normal curve)
 *   weapons       : mailed once, when the character first reaches 60/70/80
 *                   60 -> 90010 + 90020
 *                   70 -> 90011 + 90021
 *                   80 -> 90012 + 90022
 *
 * Talent points are stored in the native `characters.extraBonusTalentCount`
 * column via Player::SetBonusTalentCount(), not granted as free points and not
 * added inside a script hook. CalculateTalentsPoints() already adds that field
 * before any hook runs, so the bonus survives respecs, level ups and relogs
 * without MOA having to re-grant anything.
 */

#include "MOA.h"

#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "Mail.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "StringFormat.h"

namespace
{
    constexpr char const* REWARD_SOURCE = "mod-moa-reward";

    struct RewardMilestone
    {
        uint8  level;
        uint32 itemA;
        uint32 itemB;
        uint32 settingIndex;
    };

    constexpr RewardMilestone REWARDS[] =
    {
        { 60, 90010, 90020, 1 },
        { 70, 90011, 90021, 2 },
        { 80, 90012, 90022, 3 },
    };

    uint32 BonusTalentPoints(uint8 level)
    {
        uint32 bonus = 0;

        if (level >= 15) ++bonus;
        if (level >= 25) ++bonus;
        if (level >= 35) ++bonus;
        if (level >= 45) ++bonus;
        if (level >= 55) ++bonus;
        if (level >= 65) bonus += 2;
        if (level >= 75) bonus += 2;

        return bonus;
    }

    void SendRewardMail(Player* player, RewardMilestone const& reward)
    {
        std::string const subject = Acore::StringFormat(MOAPlayerText(player, MOAMsg::RewardMailSubject), uint32(reward.level));
        std::string const body    = Acore::StringFormat(MOAPlayerText(player, MOAMsg::RewardMailBody), uint32(reward.level));

        MailDraft draft(subject, body);

        uint32 attached = 0;
        for (uint32 entry : { reward.itemA, reward.itemB })
        {
            if (Item* item = Item::CreateItem(entry, 1, player))
            {
                draft.AddItem(item);
                ++attached;
            }
            else
                LOG_ERROR("module", "MOA reward: item entry {} could not be created (missing in item_template?).", entry);
        }

        if (!attached)
            return;

        CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
        draft.SendMailTo(trans, player, MailSender(MAIL_NORMAL, 0, MAIL_STATIONERY_GM), MAIL_CHECK_MASK_COPIED);
        CharacterDatabase.CommitTransaction(trans);

        LOG_INFO("module", "MOA reward: mailed level {} reward ({} items) to {}.",
            uint32(reward.level), attached, player->GetName());
    }
}

class MOA_RewardScript : public PlayerScript
{
public:
    MOA_RewardScript() : PlayerScript("MOA_RewardScript") { }

    // Keep characters.extraBonusTalentCount in sync with the level. Login covers
    // GM level changes, level up covers normal play; both only recompute when the
    // value actually changed, so a normal respec never regenerates anything.
    void OnPlayerLogin(Player* player) override
    {
        if (!sMOA->Enabled() || !player)
            return;

        SyncTalentBonus(player, true);
    }

    void OnPlayerLevelChanged(Player* player, uint8 /*oldlevel*/) override
    {
        if (!sMOA->Enabled() || !player)
            return;

        // GiveLevel() runs InitTalentForLevel() *before* this hook, so the pool has
        // already been calculated with the previous value. Recompute on change.
        SyncTalentBonus(player, true);

        if (!sMOA->HasAnyChallenge(player))
            return;

        uint8 const level = player->GetLevel();

        for (RewardMilestone const& reward : REWARDS)
        {
            if (level < reward.level)
                continue;

            // Already handed out (a GM jump can cross several milestones at once).
            if (player->GetPlayerSetting(REWARD_SOURCE, reward.settingIndex).value)
                continue;

            player->UpdatePlayerSetting(REWARD_SOURCE, reward.settingIndex, 1);
            SendRewardMail(player, reward);
        }
    }

private:
    static void SyncTalentBonus(Player* player, bool reapply)
    {
        uint32 const expected = sMOA->HasAnyChallenge(player) ? BonusTalentPoints(player->GetLevel()) : 0;

        if (player->GetBonusTalentCount() == expected)
            return;

        player->SetBonusTalentCount(expected);

        if (reapply)
            player->InitTalentForLevel();
    }
};

void AddMOARewardScript()
{
    new MOA_RewardScript();
}

/*
 * mod-moa — world script: load configuration, stage data and persisted state.
 */

#include "MOA.h"

#include "Config.h"
#include "Log.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldConfig.h"

class MOA_WorldScript : public WorldScript
{
public:
    MOA_WorldScript() : WorldScript("MOA_WorldScript") { }

    void OnAfterConfigLoad(bool reload) override
    {
        sMOA->LoadConfig(reload);
        sMOA->LoadData();

        if (!reload)
            sMOA->LoadState();
        else if (sMOA->CurrentStage() > sMOA->StageCount())
            sMOA->SetStage(sMOA->StageCount(), false);

        Validate();
    }

private:
    static void Validate()
    {
        if (!sMOA->Enabled())
        {
            LOG_INFO("module", "Module disabled by configuration.");
            return;
        }

        uint32 const serverCap = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
        uint32 const highestCap = sMOA->MaxLevelOfStage(sMOA->StageCount());

        if (serverCap < highestCap)
            LOG_ERROR("module",
                "MaxPlayerLevel is {} but the highest stage cap is {}. Set MaxPlayerLevel = {} in worldserver.conf, "
                "otherwise players will be hard-capped by the core before the module allows them to level.",
                serverCap, highestCap, highestCap);

        if (!sWorld->getBoolConfig(CONFIG_PLAYER_SETTINGS_ENABLED))
            LOG_ERROR("module",
                "EnablePlayerSettings is 0. Challenge choices will NOT be persisted. Set EnablePlayerSettings = 1 in worldserver.conf.");

        LOG_INFO("module", "MOA enabled: stage {}/{}, level cap {}, {} gated instances.",
            uint32(sMOA->CurrentStage()), uint32(sMOA->StageCount()), sMOA->MaxLevel(),
            uint32(sMOA->InstanceUnlocks().size()));
    }
};

void AddMOAWorldScript()
{
    new MOA_WorldScript();
}

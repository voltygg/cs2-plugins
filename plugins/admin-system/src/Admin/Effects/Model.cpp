#include "Model.hpp"

#include "../../Core/Managers.hpp"
#include "../Actions/ActionContext.hpp"
#include "EffectId.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::PlayerController;

namespace
{

// Base CS2 agent skins, used to restore a player whose model effect clears while alive (there is no
// cheap way to read back the original; respawn resets it anyway).
constexpr const char* DefaultModelT = "characters/models/tm_phoenix/tm_phoenix.vmdl";
constexpr const char* DefaultModelCt = "characters/models/ctm_sas/ctm_sas.vmdl";

// The team-default restore model, or nullptr for spectator/unassigned (nothing to restore).
const char* DefaultModelForTeam(int team)
{
    if (team == CS2Kit::Sdk::TeamT)
        return DefaultModelT;
    if (team == CS2Kit::Sdk::TeamCT)
        return DefaultModelCt;
    return nullptr;
}

}  // namespace

const std::vector<FunModel>& FunModels()
{
    // Chicken is animated; props render in bind pose. Paths missing on the client show the error
    // model, so keep these to precached, client-available assets.
    static const std::vector<FunModel> models = {
        {"Chicken", "models/chicken/chicken.vmdl"},
        {"Barrel", "models/props/de_train/hr_t/oil_drum/oil_drum.vmdl"},
        {"Crate", "models/props_crates/csgo_drop_crate.vmdl"},
        {"Gnome", "models/props_survival/gnome/gnome.vmdl"},
        {"Melon", "models/props_junk/watermelon01.vmdl"},
    };
    return models;
}

void PrecacheModels()
{
    for (const auto& model : FunModels())
    {
        Engine().Precache.Add(model.Path);
    }

    Engine().Precache.Add(DefaultModelT);
    Engine().Precache.Add(DefaultModelCt);
}

void ApplyModel(int adminSlot, int targetSlot, std::size_t modelIndex)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, Flag(Permission::Fun));
    if (!ctx.Valid() || modelIndex >= FunModels().size())
        return;

    // A dead pawn's model is the ragdoll; changing it is pointless and respawn overwrites it.
    if (!ctx.TargetCtrl.IsAlive())
        return;

    Engine().EntityOps.SetModel(ctx.TargetCtrl.GetPawn(), FunModels()[modelIndex].Path.c_str());

    // Apply cancels any prior Model effect first (re-select swaps); the cancel closure restores the
    // team default when cleared while alive (a no-op on death, where IsAlive is false).
    App().Effects.Apply(targetSlot, static_cast<int>(EffectId::Model), /*timerHandle*/ 0, [targetSlot]() {
        PlayerController pc(targetSlot);
        if (!pc.IsValid() || !pc.IsAlive())
            return;
        if (const char* def = DefaultModelForTeam(pc.GetTeam()))
            Engine().EntityOps.SetModel(pc.GetPawn(), def);
    });

    Actions::Broadcast(ctx, "broadcast.modelOn");
}

void ResetModel(int adminSlot, int targetSlot)
{
    auto ctx = Actions::Resolve(adminSlot, targetSlot, Flag(Permission::Fun));
    if (!ctx.Valid())
        return;

    if (!App().Effects.IsActive(targetSlot, static_cast<int>(EffectId::Model)))
        return;

    App().Effects.Cancel(targetSlot, static_cast<int>(EffectId::Model));
    Actions::Broadcast(ctx, "broadcast.modelOff");
}

}  // namespace AdminSystem::Admin::Effects

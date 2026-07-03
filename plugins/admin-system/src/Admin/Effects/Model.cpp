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
    // Addon-free, so only globally-mounted families render on every map: gameplay entities (chicken)
    // and weapon/grenade world models. Map-specific props box out where the map doesn't load them.
    static const std::vector<FunModel> models = {
        {"Chicken", "models/chicken/chicken.vmdl"},
        {"Fire Hydrant", "models/props_street/firehydrant.vmdl"},
        {"C4 Bomb", "weapons/models/c4/weapon_c4.vmdl"},
        {"HE Grenade", "weapons/models/grenade/hegrenade/weapon_hegrenade.vmdl"},
        {"Molotov", "weapons/models/grenade/molotov/weapon_molotov.vmdl"},
        {"Smoke Grenade", "weapons/models/grenade/smokegrenade/weapon_smokegrenade.vmdl"},
        {"Flashbang", "weapons/models/grenade/flashbang/weapon_flashbang.vmdl"},
        {"AK-47", "weapons/models/ak47/weapon_rif_ak47.vmdl"},
        {"AWP", "weapons/models/awp/weapon_snip_awp.vmdl"},
        {"Butterfly Knife", "weapons/models/knife/knife_butterfly/weapon_knife_butterfly.vmdl"},
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

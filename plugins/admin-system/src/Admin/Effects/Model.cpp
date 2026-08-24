#include "Model.hpp"

#include "Descriptors.hpp"
#include "EffectRegistry.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Runtime.hpp>
#include <VoltMod/Sdk/PawnOps.hpp>
#include <VoltMod/Sdk/PlayerController.hpp>

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using VoltMod::Sdk::PlayerController;

namespace
{

// Restore live players to a base team model; respawn restores their selected agent.
constexpr const char* DefaultModelT = "characters/models/tm_phoenix/tm_phoenix.vmdl";
constexpr const char* DefaultModelCt = "characters/models/ctm_sas/ctm_sas.vmdl";

// Spectators and unassigned players have no restore model.
const char* DefaultModelForTeam(int team)
{
    if (team == VoltMod::Sdk::TeamT)
        return DefaultModelT;
    if (team == VoltMod::Sdk::TeamCT)
        return DefaultModelCt;
    return nullptr;
}

}  // namespace

const std::vector<FunModel>& FunModels()
{
    // Use always-mounted assets so every vanilla map can render these models.
    static const std::vector<FunModel> models = {
        {"Chicken", "models/chicken/chicken.vmdl"},
        {"Fire Hydrant", "models/props_street/firehydrant.vmdl"},
        {"C4 Bomb", "weapons/models/c4/weapon_c4.vmdl"},
    };
    return models;
}

void PrecacheModels(VoltMod::Runtime& runtime)
{
    for (const auto& model : FunModels())
    {
        runtime.Precache.Add(model.Path);
    }

    runtime.Precache.Add(DefaultModelT);
    runtime.Precache.Add(DefaultModelCt);
}

const ParamEffect Model{.Permission = Flag(Permission::Fun),
                        .Id = static_cast<int>(EffectId::Model),
                        .NameKey = "action.model",
                        .OnKey = "broadcast.modelOn",
                        .OffKey = "broadcast.modelOff",
                        .ResetLabelKey = "action.modelReset",
                        .RequireAlive = true,
                        .Choices =
                            []() {
                                std::vector<EffectChoice> choices;
                                const auto& models = FunModels();
                                choices.reserve(models.size());
                                for (int i = 0; i < static_cast<int>(models.size()); ++i)
                                    choices.push_back({models[i].Name, i});
                                return choices;
                            },
                        .Setup = [](const ActionContext& ctx, int param) -> EffectInstance {
                            // Dispatch already bounds-checked param and required the target alive.
                            ctx.Rt.EntityOps.SetModel(ctx.TargetCtrl.GetPawn(), FunModels()[param].Path.c_str());

                            // EffectManager cancels any prior Model effect first (re-select swaps); OnStop restores the
                            // team default when cleared while alive (a no-op on death, where IsAlive is false).
                            int targetSlot = ctx.Target->GetSlot();
                            return {.OnStop = [&ops = ctx.Rt.EntityOps, targetSlot]() {
                                PlayerController pc(targetSlot);
                                if (!pc.IsValid() || !pc.IsAlive())
                                    return;
                                if (const char* def = DefaultModelForTeam(pc.GetTeam()))
                                    ops.SetModel(pc.GetPawn(), def);
                            }};
                        }};

}  // namespace AdminSystem::Admin::Effects

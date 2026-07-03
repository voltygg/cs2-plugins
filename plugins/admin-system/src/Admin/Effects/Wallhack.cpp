#include "Descriptors.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Sdk/Entity.hpp>
#include <CS2Kit/Sdk/EntityKeyValues.hpp>
#include <CS2Kit/Sdk/EntityOps.hpp>
#include <CS2Kit/Sdk/PawnOps.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <array>
#include <Color.h>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Effects
{

using Actions::ActionContext;
using CS2Kit::Sdk::EntityKeyValues;
using CS2Kit::Sdk::MaxPlayers;
using CS2Kit::Sdk::PlayerController;
using CS2Kit::Sdk::TeamCT;
using CS2Kit::Sdk::TeamT;

// Shows the target every other live player as a team-colored glow through walls. Each glowing
// player gets two prop_dynamic clones following their pawn - an invisible relay and a glow prop
// parented to it (the indirection renders only the outline) - both transmit-filtered to the
// target alone. A repeating reconcile pass tracks spawns, deaths, and team/model changes.

namespace
{

constexpr int ReconcileIntervalMs = 500;
constexpr int RenderModeNone = 10;
constexpr uint32_t InvalidHandle = 0xFFFFFFFFu;

// prop_dynamic keyvalues shared by the relay and glow clones.
constexpr int PropSpawnFlags = 256;
constexpr int GlowRangeUnits = 5000;
constexpr int GlowTeamAny = -1;
constexpr int GlowStateAlwaysOn = 3;
constexpr int GlowRenderAmt = 1;

const Color GlowColorT(255, 128, 0, 255);
const Color GlowColorCT(0, 160, 255, 255);

struct GlowPair
{
    uint32_t RelayHandle = InvalidHandle;
    uint32_t GlowHandle = InvalidHandle;
    int RelayIndex = -1;
    int GlowIndex = -1;
    int Team = 0;
    std::string Model;

    // The relay handle is the single source of truth for liveness; DestroyPair resets it.
    bool Active() const { return RelayHandle != InvalidHandle; }
};

struct WallhackState
{
    std::array<GlowPair, MaxPlayers> Pairs{};
};

void DestroyPair(GlowPair& pair)
{
    if (!pair.Active())
        return;

    // Unregister before removal: a recycled index still registered would filter
    // whatever entity the engine hands that index to next.
    auto& transmit = Engine().Transmit;
    transmit.ClearEntityExclusive(pair.RelayIndex);
    transmit.ClearEntityExclusive(pair.GlowIndex);

    auto& ops = Engine().EntityOps;
    auto& entities = Engine().Entities;
    if (auto* glow = entities.ResolveEntityHandleExact(pair.GlowHandle))
        ops.Remove(glow);
    if (auto* relay = entities.ResolveEntityHandleExact(pair.RelayHandle))
        ops.Remove(relay);

    pair = {};
}

void CreatePair(int beneficiarySlot, int slot, GlowPair& pair)
{
    PlayerController pc(slot);
    auto* pawn = pc.GetPawn();
    std::string model = pc.GetPawnModelName();
    int team = pc.GetTeam();
    if (!pawn || model.empty())
        return;

    auto& ops = Engine().EntityOps;

    EntityKeyValues relayKv;
    relayKv.Set("model", model.c_str()).Set("spawnflags", PropSpawnFlags).Set("rendermode", RenderModeNone);
    auto* relay = ops.Spawn("prop_dynamic", relayKv);
    if (!relay)
        return;

    EntityKeyValues glowKv;
    glowKv.Set("model", model.c_str())
        .Set("spawnflags", PropSpawnFlags)
        .Set("glowcolor", team == TeamT ? GlowColorT : GlowColorCT)
        .Set("glowrange", GlowRangeUnits)
        .Set("glowteam", GlowTeamAny)
        .Set("glowstate", GlowStateAlwaysOn)
        .Set("renderamt", GlowRenderAmt);
    auto* glow = ops.Spawn("prop_dynamic", glowKv);
    if (!glow)
    {
        ops.Remove(relay);
        return;
    }

    ops.AcceptInput(relay, "FollowEntity", "!activator", pawn);
    ops.AcceptInput(glow, "FollowEntity", "!activator", relay);

    auto& entities = Engine().Entities;
    pair.RelayHandle = entities.GetEntityHandle(relay);
    pair.GlowHandle = entities.GetEntityHandle(glow);
    pair.RelayIndex = entities.GetEntityIndex(relay);
    pair.GlowIndex = entities.GetEntityIndex(glow);
    pair.Team = team;
    pair.Model = std::move(model);

    auto& transmit = Engine().Transmit;
    transmit.SetEntityExclusive(pair.RelayIndex, beneficiarySlot);
    transmit.SetEntityExclusive(pair.GlowIndex, beneficiarySlot);
}

void Reconcile(int beneficiarySlot, WallhackState& state)
{
    for (int slot = 0; slot < MaxPlayers; ++slot)
    {
        auto& pair = state.Pairs[slot];

        PlayerController pc(slot);
        int team = pc.GetTeam();
        // Ghosted pawns never transmit to the target, so a clone would follow nothing.
        bool desired = slot != beneficiarySlot && pc.IsValid() && pc.IsAlive() &&
                       (team == TeamT || team == TeamCT) && !Engine().Transmit.IsPawnHidden(slot);

        if (pair.Active())
        {
            auto& entities = Engine().Entities;
            bool stale = !desired || team != pair.Team || !entities.ResolveEntityHandleExact(pair.RelayHandle) ||
                         !entities.ResolveEntityHandleExact(pair.GlowHandle) || pc.GetPawnModelName() != pair.Model;
            if (stale)
                DestroyPair(pair);
        }

        if (!pair.Active() && desired)
            CreatePair(beneficiarySlot, slot, pair);
    }
}

}  // namespace

const Effect Wallhack{
    .Flag = Flag(Permission::Fun),
    .Id = static_cast<int>(EffectId::Wallhack),
    .NameKey = "action.wallhack",
    .OnKey = "broadcast.wallhackOn",
    .OffKey = "broadcast.wallhackOff",
    .Scope = EffectScope::Round,
    .TickIntervalMs = ReconcileIntervalMs,
    .Setup =
        [](const ActionContext& ctx) -> EffectInstance {
        int slot = ctx.Target->GetSlot();
        auto state = std::make_shared<WallhackState>();

        // Build the glow clones immediately; the repeating tick then tracks spawns/deaths/team
        // changes. OnStop clears the transmit-filter entries and removes any surviving clones
        // (on a round restart the props are already gone).
        Reconcile(slot, *state);

        return {.OnTick = [slot, state]() { Reconcile(slot, *state); },
                .OnStop =
                    [state]() {
                        for (auto& pair : state->Pairs)
                            DestroyPair(pair);
                    }};
    }};

}  // namespace AdminSystem::Admin::Effects

#pragma once

#include "Aim/AimbotCore.hpp"
#include "Aim/AimlockCore.hpp"
#include "Aim/AntiAimCore.hpp"
#include "Aim/MouseCore.hpp"
#include "Aim/RecoilCore.hpp"
#include "Aim/SilentAimCore.hpp"
#include "Aim/TriggerbotCore.hpp"
#include "Aim/WallhackCore.hpp"
#include "Client/InvalidCvarRules.hpp"
#include "Client/NamechangerCore.hpp"
#include "Config.hpp"
#include "Core/Finding.hpp"
#include "Correlation/ShotCorrelatorCore.hpp"
#include "Response/ResponseManager.hpp"

#include <VoltMod/Api.hpp>
#include <optional>
#include <tuple>

namespace Anticheat
{

class Detectors
{
public:
    Detectors(VoltMod::Runtime& runtime, ConfigManager& config, ResponseManager& response)
        : _rt(runtime), _config(config), _response(response)
    {}

    /** Resolve the convars the gates read. */
    void Initialize();

    /** Disabled globally or while `sv_cheats` is enabled outside test mode. */
    bool Enabled() const;
    bool ModuleEnabled(DetectionKind kind) const;

    /** The gate for @p core, taken from the core's own Kind so the two cannot disagree. */
    template <class Core>
    bool ModuleEnabled(const Core& core) const
    {
        (void)core;
        return ModuleEnabled(Core::Kind);
    }

    /** True when @p slot is checked at all: a spawned human, or a bot while `debug.includeBots` is on. */
    bool IsEligible(int slot);
    bool IncludesBots() const;

    void Report(int slot, const std::optional<Finding>& finding);

    /** Cheat-protected client values only mean something once a disabled sv_cheats has reached them. */
    bool EnforceCheatCvars() const;

    /** Mirror a server convar change; true when accumulated evidence no longer applies. */
    bool OnConVarChanged(const VoltMod::ConVarChange& change);
    /** Update hostile-shot rules from `mp_teammates_are_enemies`. */
    void RefreshTeamRules();

    /** Clear every core; the caller clears its adapters alongside. */
    void Reset();
    void OnSlotChanged(int slot);

    ShotCorrelatorCore Correlator;
    AimbotCore Aimbot{Correlator};
    AimlockCore Aimlock{Correlator};
    AntiAimCore AntiAim;
    SilentAimCore SilentAim;
    TriggerbotCore Triggerbot{Correlator};
    RecoilCore Recoil;
    MouseCore Mouse{Correlator};
    WallhackCore Wallhack{Correlator};
    NamechangerCore Namechanger;
    InvalidCvarRules InvalidCvars;

private:
    /** Every core a reset or a slot change has to clear; a new core is wired in here only. */
    auto Cores()
    {
        return std::tie(Correlator, Aimbot, Aimlock, AntiAim, SilentAim, Triggerbot, Recoil, Mouse, Wallhack,
                        Namechanger, InvalidCvars);
    }

    VoltMod::Runtime& _rt;
    ConfigManager& _config;
    ResponseManager& _response;

    /** Resolved once in Initialize: a registered convar's handle is stable for the load cycle,
     *  and every detection path reads it. An unusable handle reads as invalid. */
    VoltMod::ConVar<bool> _svCheats;
    VoltMod::ConVar<bool> _teammatesAreEnemies;
    // Stamped when sv_cheats goes off, so replicated client values get time to catch up.
    double _cheatGraceUntil = 0.0;
};

}  // namespace Anticheat

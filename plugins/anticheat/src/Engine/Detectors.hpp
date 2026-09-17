#pragma once

#include "Config.hpp"
#include "Detect/Rules/Aimbot.hpp"
#include "Detect/Rules/Aimlock.hpp"
#include "Detect/Rules/AntiAim.hpp"
#include "Detect/Rules/InvalidCvar.hpp"
#include "Detect/Rules/MouseMismatch.hpp"
#include "Detect/Rules/Namechanger.hpp"
#include "Detect/Rules/Recoil.hpp"
#include "Detect/Rules/SilentAim.hpp"
#include "Detect/Rules/Triggerbot.hpp"
#include "Detect/Rules/Wallhack.hpp"
#include "Detect/Finding.hpp"
#include "Detect/ShotHistory.hpp"
#include "Detect/Suspicion.hpp"
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
    bool RuleEnabled(DetectionKind kind) const;

    /** The gate for @p rule, taken from the rule's own Kind so the two cannot disagree. */
    template <class Rule>
    bool RuleEnabled(const Rule& rule) const
    {
        (void)rule;
        return RuleEnabled(Rule::Kind);
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

    /** Clear every rule; the caller clears its adapters alongside. */
    void Reset();
    void OnSlotChanged(int slot);

    /** Every rule's evidence about every player, and the one place a report is decided. */
    Suspicion Scores;
    ShotHistory History;
    Rules::Aimbot Aimbot{History};
    Rules::Aimlock Aimlock{History};
    Rules::AntiAim AntiAim;
    Rules::SilentAim SilentAim{Scores};
    Rules::Triggerbot Triggerbot{History, Scores};
    Rules::Recoil Recoil{Scores};
    Rules::MouseMismatch Mouse{History};
    Rules::Wallhack Wallhack{History, Scores};
    Rules::Namechanger Namechanger{Scores};
    Rules::InvalidCvar InvalidCvars;

private:
    /** Every rule holding per-player state of its own; one that holds none is left out. */
    auto All()
    {
        return std::tie(Scores, History, Aimbot, Aimlock, AntiAim, Triggerbot, Recoil, Mouse, Wallhack,
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

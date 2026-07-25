#pragma once

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Players/Player.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace AdminSystem::Reports
{

/** Why a report attempt was refused. */
enum class ReportDenial
{
    Allowed,
    Disabled,
    OnCooldown,
};

/** Gate verdict; SecondsLeft carries the remaining wait for @ref ReportDenial::OnCooldown. */
struct ReportGate
{
    ReportDenial Reason = ReportDenial::Allowed;
    int64_t SecondsLeft = 0;

    explicit operator bool() const { return Reason == ReportDenial::Allowed; }
};

/**
 * Owns player-submitted reports: the anti-spam gate and the single database write. Nothing in-game
 * reads them back; an upstream website triages the `player_reports` table.
 *
 * State is keyed by SteamID, not slot (which rules out CS2Kit::SlotThrottle), and survives
 * disconnect and map change - otherwise rejoining would clear a cooldown.
 */
class ReportManager
{
public:
    ReportManager() = default;

    /** Enabled/cooldown gate - the `!report` entry check, before a target is chosen. */
    ReportGate CanReport(int64_t reporterSteamId) const;

    /** The above plus "already reported this target inside the duplicate window". */
    bool CanReport(int64_t reporterSteamId, int64_t targetSteamId) const;

    /**
     * Re-check the gate, persist, and arm the reporter's cooldown. @p onDone reports the write
     * outcome on the game thread; false also covers a refused gate. A failed write releases the
     * cooldown, so an outage costs the reporter nothing.
     */
    void Submit(const CS2Kit::Player& reporter, const CS2Kit::Player& target, const std::string& reasonCode,
                const std::string& reasonText, std::function<void(bool ok)> onDone);

private:
    /** Both CanReport overloads, parameterized by @p now so a submit gates and stamps from one
     *  clock read. @p targetSteamId engages the per-target duplicate window. */
    ReportGate EvaluateGate(int64_t reporterSteamId, std::optional<int64_t> targetSteamId, int64_t now) const;

    /** Claim the reporter's next slot before the write, so a double-press can't produce two rows. */
    void Arm(int64_t reporterSteamId, int64_t targetSteamId, int64_t now);
    void Release(int64_t reporterSteamId, int64_t targetSteamId);

    /** Drop entries past the longest configured window. Called from Arm, the only growth point. */
    void Prune(int64_t now);

    std::unordered_map<int64_t, int64_t> _lastReportAt;                // reporter -> epoch
    std::map<std::pair<int64_t, int64_t>, int64_t> _lastReportOfPair;  // (reporter, target) -> epoch
};

}  // namespace AdminSystem::Reports

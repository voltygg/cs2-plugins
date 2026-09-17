#pragma once

#include "Detect/Samples.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string_view>

namespace Anticheat
{

inline constexpr int TeamT = 2;
inline constexpr int TeamCT = 3;

/** One server tick of world state, one entry per slot. */
struct PositionFrame
{
    int32_t ServerTick = -1;
    std::array<PositionSample, MaxSlots> Players{};
};

class ShotHistory
{
public:
    /** Mirrors mp_teammates_are_enemies for hostile-shot matching. */
    void SetTeammatesAreEnemies(bool value) { _teammatesAreEnemies = value; }

    /** Map change or config reload: drop every command, shot and frame. */
    void Reset();

    /** Invalidate one slot's in-flight shots after connect or disconnect. */
    void ClearSlot(int slot);

    /** Duplicates (same CmdNum) are dropped. */
    void OnCommand(int slot, const CmdSample& cmd);

    /** Stamp the command the server is about to simulate for @p serverTick. */
    void OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, const Vec3& eyePos, bool airborne);

    /** A repeat of the newest tick replaces it rather than growing the ring. */
    void CaptureFrame(int32_t serverTick, const std::array<PositionSample, MaxSlots>& players);

    /** Drop commands and shots that can no longer match an event. */
    void Prune(int32_t serverTick);

    /** weapon_fire for a ballistic weapon. @p fireCmdNum binds exactly when the pawn names it (0
     *  when unknown); otherwise the window's one unique command binds and ambiguous ones are
     *  consumed. */
    ShotView* OnWeaponFire(int slot, std::string_view weapon, int32_t serverTick, const AimAngles& visibleAngles,
                           bool hasVisibleAngles, int32_t fireCmdNum = 0, int shotsFired = 0);

    ShotView* OnBulletImpact(int slot, const Vec3& impact, int32_t serverTick);
    ShotView* OnPlayerHurt(int attackerSlot, int victimSlot, bool headshot, int32_t serverTick);
    ShotView* OnPlayerDeath(int attackerSlot, int victimSlot, std::string_view weapon, bool wallbang,
                            int32_t serverTick);

    /** bullet_impact carries only the low byte of the shooter's userid. Matched against
     *  @p userIdBySlot, requiring both the slot and its in-window shot to be unique; -1 otherwise. */
    int ResolveImpactShooter(int truncatedUserId, int32_t serverTick, std::span<const int32_t> userIdBySlot) const;

    const PositionFrame* FindFrame(int32_t serverTick) const;
    std::optional<PositionSample> FindPosition(int32_t serverTick, int slot) const;

    /** Both teams playing, and either different or free-for-all. */
    static bool AreOpponents(int teamA, int teamB, bool teammatesAreEnemies);
    bool AreOpponents(int teamA, int teamB) const { return AreOpponents(teamA, teamB, _teammatesAreEnemies); }

    /** A trackable enemy of @p team: what every target scan means by a candidate. */
    bool IsOpponent(int team, const PositionSample& target) const
    {
        return target.Trackable() && AreOpponents(team, target.Team);
    }

    std::deque<ShotView>& Shots(int slot);
    const std::deque<ShotView>& Shots(int slot) const;

    size_t FrameCount() const { return _frames.size(); }
    size_t CommandCount(int slot) const;

private:
    struct PendingCommand
    {
        CmdSample Cmd;
        bool Simulated = false;
        bool Consumed = false;
    };

    struct SlotData
    {
        std::deque<PendingCommand> Commands;
        std::deque<ShotView> Shots;
    };

    /** Newest shot within the match window whose weapon matches, when it is the only one. */
    ShotView* MatchEvent(int slot, std::string_view weapon, int32_t serverTick);

    std::array<SlotData, MaxSlots> _slots{};
    std::deque<PositionFrame> _frames;
    /** Read-only empty result returned for an out-of-range slot. */
    std::deque<ShotView> _noShots;
    bool _teammatesAreEnemies = false;
};

}  // namespace Anticheat

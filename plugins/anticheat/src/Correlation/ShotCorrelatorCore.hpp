#pragma once

// Joins usercmds to the events they produced, so every aim module works from "this exact command
// fired this exact bullet at this exact world state" rather than from whatever the engine last
// reported. SDK-free: the adapter feeds it plain samples.

#include "Core/Samples.hpp"

#include <array>
#include <cstdint>
#include <deque>
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

class ShotCorrelatorCore
{
public:
    /** Mirrors mp_teammates_are_enemies. Only changed through a reset: it decides which shots are
     *  hostile. */
    void SetTeammatesAreEnemies(bool value) { _teammatesAreEnemies = value; }
    bool TeammatesAreEnemies() const { return _teammatesAreEnemies; }

    /** Map change or config reload: drop every command, shot and frame. */
    void Reset();

    /** Connect or disconnect: invalidate the slot's in-flight shots, leaving other slots alone. */
    void OnSlotChanged(int slot);

    /** Duplicates (same CmdNum) are dropped. */
    void OnCommand(int slot, const CmdSample& cmd);

    /** Stamp the command the server is about to simulate for @p serverTick. */
    void OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, const Vec3& eyePos, bool airborne, bool scoped);

    /** A repeat of the newest tick replaces it rather than growing the ring. */
    void CaptureFrame(int32_t serverTick, const std::array<PositionSample, MaxSlots>& players);

    /** Drop commands and shots that can no longer match an event. */
    void Prune(int32_t serverTick);

    /**
     * weapon_fire for a ballistic weapon. Null when zero or several commands could have fired it -
     * an ambiguous window is evidence of nothing, and its candidates are consumed so a later event
     * cannot pick one of them arbitrarily.
     */
    ShotView* OnWeaponFire(int slot, std::string_view weapon, int32_t serverTick, const AimAngles& visibleAngles,
                           bool hasVisibleAngles);

    ShotView* OnBulletImpact(int slot, const Vec3& impact, int32_t serverTick);
    ShotView* OnPlayerHurt(int attackerSlot, int victimSlot, bool headshot, int32_t serverTick);
    ShotView* OnPlayerDeath(int attackerSlot, int victimSlot, std::string_view weapon, bool wallbang,
                            int32_t serverTick);

    /**
     * bullet_impact carries only the low byte of the shooter's userid. Matches it against
     * @p userIdBySlot, requiring both the slot and its in-window shot to be unique; -1 otherwise.
     */
    int ResolveImpactShooter(int truncatedUserId, int32_t serverTick, std::span<const int32_t> userIdBySlot) const;

    const PositionFrame* FindFrame(int32_t serverTick) const;
    const PositionSample* FindPosition(int32_t serverTick, int slot) const;

    /** Both teams playing, and either different or free-for-all. */
    static bool AreOpponents(int teamA, int teamB, bool teammatesAreEnemies);
    bool AreOpponents(int teamA, int teamB) const { return AreOpponents(teamA, teamB, _teammatesAreEnemies); }

    std::deque<ShotView>& Shots(int slot);
    const std::deque<ShotView>& Shots(int slot) const;
    uint32_t Generation(int slot) const;

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
        uint32_t Generation = 1;
    };

    void AdvanceGeneration(SlotData& data);
    /** Newest shot within the match window whose weapon matches, when it is the only one. */
    ShotView* MatchEvent(int slot, std::string_view weapon, int32_t serverTick);

    std::array<SlotData, MaxSlots> _slots{};
    std::deque<PositionFrame> _frames;
    uint32_t _nextShotId = 1;
    bool _teammatesAreEnemies = false;
};

}  // namespace Anticheat

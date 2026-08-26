#include "ShotCorrelatorCore.hpp"

#include "Core/Geometry.hpp"
#include "Core/WeaponClass.hpp"

#include <algorithm>

namespace Anticheat
{

static constexpr size_t ShotCommandLimit = 32;
static constexpr size_t PendingShotLimit = 8;
static constexpr size_t PositionHistoryLimit = 128;
static constexpr int ShotMatchTicks = 1;
static constexpr int ShotLifetimeTicks = 2;
static_assert(ShotLifetimeTicks > ShotMatchTicks, "Shots must outlive the window every event matches in.");

static bool InWindow(int32_t serverTick, int32_t stampTick)
{
    const int64_t delta = static_cast<int64_t>(serverTick) - stampTick;
    return delta >= 0 && delta <= ShotMatchTicks;
}

void ShotCorrelatorCore::AdvanceGeneration(SlotData& data)
{
    data.Commands.clear();
    data.Shots.clear();
    if (++data.Generation == 0)
        data.Generation = 1;
}

void ShotCorrelatorCore::Reset()
{
    for (auto& data : _slots)
        AdvanceGeneration(data);
    _frames.clear();
}

void ShotCorrelatorCore::OnSlotChanged(int slot)
{
    if (!InSlotRange(slot))
        return;
    AdvanceGeneration(_slots[slot]);
    for (auto& frame : _frames)
        frame.Players[slot] = {};
}

void ShotCorrelatorCore::OnCommand(int slot, const CmdSample& cmd)
{
    if (!InSlotRange(slot) || !cmd.AttackAngles || !Geometry::IsFinite(*cmd.AttackAngles))
        return;

    auto& commands = _slots[slot].Commands;
    const bool duplicate = std::any_of(commands.rbegin(), commands.rend(),
                                       [&](const PendingCommand& stored) { return stored.Cmd.CmdNum == cmd.CmdNum; });
    if (duplicate)
        return;

    PendingCommand entry;
    entry.Cmd = cmd;
    entry.Cmd.ServerTick = -1;
    commands.push_back(std::move(entry));
    while (commands.size() > ShotCommandLimit)
        commands.pop_front();
}

void ShotCorrelatorCore::OnSimulated(int slot, int32_t cmdNum, int32_t serverTick, const Vec3& eyePos, bool airborne)
{
    if (!InSlotRange(slot) || !Geometry::IsFinite(eyePos))
        return;

    auto& commands = _slots[slot].Commands;
    auto found = std::find_if(commands.rbegin(), commands.rend(),
                              [&](const PendingCommand& stored) { return stored.Cmd.CmdNum == cmdNum; });
    if (found == commands.rend())
        return;

    found->Cmd.ServerTick = serverTick;
    found->Cmd.EyePos = eyePos;
    found->Cmd.Airborne = airborne;
    found->Simulated = true;
}

void ShotCorrelatorCore::CaptureFrame(int32_t serverTick, const std::array<PositionSample, MaxSlots>& players)
{
    PositionFrame frame;
    frame.ServerTick = serverTick;
    frame.Players = players;
    for (auto& player : frame.Players)
        if (player.Valid && (!Geometry::IsFinite(player.Origin) || !Geometry::IsFinite(player.EyePos)))
            player = {};

    if (!_frames.empty() && _frames.back().ServerTick == serverTick)
    {
        _frames.back() = frame;
        return;
    }
    _frames.push_back(std::move(frame));
    while (_frames.size() > PositionHistoryLimit)
        _frames.pop_front();
}

void ShotCorrelatorCore::Prune(int32_t serverTick)
{
    for (auto& data : _slots)
    {
        std::erase_if(data.Commands, [&](const PendingCommand& entry) {
            return entry.Simulated && static_cast<int64_t>(serverTick) - entry.Cmd.ServerTick > ShotLifetimeTicks;
        });
        std::erase_if(data.Shots, [&](const ShotView& shot) {
            return static_cast<int64_t>(serverTick) - shot.FireTick > ShotLifetimeTicks;
        });
    }
}

ShotView* ShotCorrelatorCore::OnWeaponFire(int slot, std::string_view weapon, int32_t serverTick,
                                           const AimAngles& visibleAngles, bool hasVisibleAngles)
{
    if (!InSlotRange(slot) || !IsBallisticWeapon(weapon))
        return nullptr;

    auto& data = _slots[slot];
    PendingCommand* match = nullptr;
    int matches = 0;
    for (auto& entry : data.Commands)
    {
        if (!entry.Simulated || entry.Consumed || !InWindow(serverTick, entry.Cmd.ServerTick))
            continue;
        match = &entry;
        ++matches;
    }
    if (matches != 1)
    {
        // Burn every candidate: an ambiguous window must not leave a command a later fire can claim.
        for (auto& entry : data.Commands)
            if (entry.Simulated && !entry.Consumed && InWindow(serverTick, entry.Cmd.ServerTick))
                entry.Consumed = true;
        return nullptr;
    }

    match->Consumed = true;
    ShotView shot;
    shot.Generation = data.Generation;
    shot.Slot = slot;
    shot.CmdNum = match->Cmd.CmdNum;
    shot.ClientTick = match->Cmd.ClientTick;
    shot.ServerTick = match->Cmd.ServerTick;
    shot.FireTick = serverTick;
    shot.VisibleAngles = visibleAngles;
    shot.HasVisibleAngles = hasVisibleAngles && Geometry::IsFinite(visibleAngles);
    shot.EyePos = match->Cmd.EyePos;
    shot.Weapon = NormalizeWeapon(weapon);
    shot.Airborne = match->Cmd.Airborne;

    data.Shots.push_back(std::move(shot));
    while (data.Shots.size() > PendingShotLimit)
        data.Shots.pop_front();
    return &data.Shots.back();
}

ShotView* ShotCorrelatorCore::MatchEvent(int slot, std::string_view weapon, int32_t serverTick)
{
    if (!InSlotRange(slot))
        return nullptr;

    weapon = NormalizeWeapon(weapon);
    auto& data = _slots[slot];
    ShotView* match = nullptr;
    int matches = 0;
    for (auto& shot : data.Shots)
    {
        if (shot.Generation != data.Generation || !InWindow(serverTick, shot.FireTick))
            continue;
        if (!weapon.empty() && NormalizeWeapon(shot.Weapon) != weapon)
            continue;
        match = &shot;
        ++matches;
    }
    return matches == 1 ? match : nullptr;
}

ShotView* ShotCorrelatorCore::OnBulletImpact(int slot, const Vec3& impact, int32_t serverTick)
{
    ShotView* shot = MatchEvent(slot, {}, serverTick);
    if (!shot || shot->ImpactSeen || !Geometry::IsFinite(impact))
        return nullptr;
    shot->ImpactPos = impact;
    shot->ImpactSeen = true;
    return shot;
}

ShotView* ShotCorrelatorCore::OnPlayerHurt(int attackerSlot, int victimSlot, bool headshot, int32_t serverTick)
{
    if (!InSlotRange(attackerSlot) || !InSlotRange(victimSlot) || attackerSlot == victimSlot)
        return nullptr;
    ShotView* shot = MatchEvent(attackerSlot, {}, serverTick);
    if (!shot || shot->HurtSeen)
        return nullptr;
    shot->HurtSeen = true;
    shot->VictimSlot = victimSlot;
    shot->Headshot = headshot;
    return shot;
}

ShotView* ShotCorrelatorCore::OnPlayerDeath(int attackerSlot, int victimSlot, std::string_view weapon, bool wallbang,
                                            int32_t serverTick)
{
    if (!InSlotRange(attackerSlot) || !InSlotRange(victimSlot) || attackerSlot == victimSlot)
        return nullptr;
    ShotView* shot = MatchEvent(attackerSlot, weapon, serverTick);
    if (!shot || shot->DeathSeen)
        return nullptr;
    shot->DeathSeen = true;
    shot->VictimSlot = victimSlot;
    shot->Wallbang = wallbang;
    return shot;
}

int ShotCorrelatorCore::ResolveImpactShooter(int truncatedUserId, int32_t serverTick,
                                             std::span<const int32_t> userIdBySlot) const
{
    if (truncatedUserId < 0)
        return -1;

    int match = -1;
    const int count = static_cast<int>(std::min<size_t>(userIdBySlot.size(), MaxSlots));
    for (int slot = 0; slot < count; ++slot)
    {
        if (userIdBySlot[slot] < 0 || (userIdBySlot[slot] & 0xff) != (truncatedUserId & 0xff))
            continue;

        const auto& data = _slots[slot];
        int compatible = 0;
        for (const auto& shot : data.Shots)
            compatible += shot.Generation == data.Generation && InWindow(serverTick, shot.FireTick);
        if (compatible != 1)
            continue;
        if (match >= 0)
            return -1;
        match = slot;
    }
    return match;
}

const PositionFrame* ShotCorrelatorCore::FindFrame(int32_t serverTick) const
{
    for (auto frame = _frames.rbegin(); frame != _frames.rend(); ++frame)
    {
        if (frame->ServerTick == serverTick)
            return &*frame;
        if (frame->ServerTick < serverTick)
            break;
    }
    return nullptr;
}

std::optional<PositionSample> ShotCorrelatorCore::FindPosition(int32_t serverTick, int slot) const
{
    const PositionFrame* frame = FindFrame(serverTick);
    if (!frame || !InSlotRange(slot) || !frame->Players[slot].Valid)
        return std::nullopt;
    return frame->Players[slot];
}

bool ShotCorrelatorCore::AreOpponents(int teamA, int teamB, bool teammatesAreEnemies)
{
    const bool aPlaying = teamA == TeamT || teamA == TeamCT;
    const bool bPlaying = teamB == TeamT || teamB == TeamCT;
    return aPlaying && bPlaying && (teamA != teamB || teammatesAreEnemies);
}

std::deque<ShotView>& ShotCorrelatorCore::Shots(int slot)
{
    return InSlotRange(slot) ? _slots[slot].Shots : _noShots;
}

const std::deque<ShotView>& ShotCorrelatorCore::Shots(int slot) const
{
    return InSlotRange(slot) ? _slots[slot].Shots : _noShots;
}

uint32_t ShotCorrelatorCore::Generation(int slot) const
{
    return InSlotRange(slot) ? _slots[slot].Generation : 0;
}

size_t ShotCorrelatorCore::CommandCount(int slot) const
{
    return InSlotRange(slot) ? _slots[slot].Commands.size() : 0;
}

}  // namespace Anticheat

#pragma once

// The violation funnel as pure decision logic: given who was detected, what the config says and
// what has already been done to them, decide what happens. SDK-free so every branch is testable.

#include "Core/Samples.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace Anticheat
{

/** Response ladder, from config "anticheat.mode". */
enum class Mode
{
    Observe,  // report only - this is the dry run
    Alert,    // observe + notify admins through admin-system
    Ban       // alert + kick or ban
};

constexpr Mode ParseMode(std::string_view mode)
{
    if (mode == "ban")
        return Mode::Ban;
    if (mode == "alert")
        return Mode::Alert;
    return Mode::Observe;
}

constexpr const char* ModeName(Mode mode)
{
    switch (mode)
    {
    case Mode::Observe:
        return "observe";
    case Mode::Alert:
        return "alert";
    case Mode::Ban:
        return "ban";
    }
    return "observe";
}

/** Ordered: a slot's punishment may only ever be raised. */
enum class PunishmentLevel
{
    None = 0,
    Kick = 1,
    Ban = 2,
};

constexpr const char* PunishmentName(PunishmentLevel level)
{
    switch (level)
    {
    case PunishmentLevel::None:
        return "none";
    case PunishmentLevel::Kick:
        return "kick";
    case PunishmentLevel::Ban:
        return "ban";
    }
    return "none";
}

/** What actually happened, for the log line and the webhook embed. */
enum class FunnelOutcome
{
    NoIdentity,       // SteamID not resolved yet - reported, never punished
    Whitelisted,      // reported, never punished
    Observed,         // observe mode
    Alerted,          // alert mode, or ban mode with nothing left to escalate
    AlreadyPunished,  // this slot already carries an equal or higher punishment
    KickIssued,
    BanIssued,
};

constexpr const char* OutcomeName(FunnelOutcome outcome)
{
    switch (outcome)
    {
    case FunnelOutcome::NoIdentity:
        return "no identity";
    case FunnelOutcome::Whitelisted:
        return "whitelisted";
    case FunnelOutcome::Observed:
        return "observed";
    case FunnelOutcome::Alerted:
        return "alerted";
    case FunnelOutcome::AlreadyPunished:
        return "already punished";
    case FunnelOutcome::KickIssued:
        return "kicked";
    case FunnelOutcome::BanIssued:
        return "banned";
    }
    return "observed";
}

struct FunnelInput
{
    int64_t SteamId = 0;
    bool Whitelisted = false;
    Mode CurrentMode = Mode::Observe;
    bool KickOnly = false;                           // from the Finding
    PunishmentLevel Issued = PunishmentLevel::None;  // this slot's latch
};

struct FunnelDecision
{
    FunnelOutcome Outcome = FunnelOutcome::Observed;
    bool SendAlert = false;
    PunishmentLevel Apply = PunishmentLevel::None;  // None = nothing to issue
};

/**
 * Every detection is logged and reported by the caller regardless; this decides only whether an
 * alert goes out and what punishment (if any) is issued.
 */
constexpr FunnelDecision Decide(const FunnelInput& input)
{
    if (input.SteamId == 0)
        return {.Outcome = FunnelOutcome::NoIdentity};
    if (input.Whitelisted)
        return {.Outcome = FunnelOutcome::Whitelisted};
    if (input.CurrentMode == Mode::Observe)
        return {.Outcome = FunnelOutcome::Observed};

    if (input.CurrentMode == Mode::Alert)
        return {.Outcome = FunnelOutcome::Alerted, .SendAlert = true};

    const PunishmentLevel requested = input.KickOnly ? PunishmentLevel::Kick : PunishmentLevel::Ban;
    if (input.Issued >= requested)
        return {.Outcome = FunnelOutcome::AlreadyPunished, .SendAlert = true};

    return {.Outcome = requested == PunishmentLevel::Kick ? FunnelOutcome::KickIssued : FunnelOutcome::BanIssued,
            .SendAlert = true,
            .Apply = requested};
}

/** Per-slot no-downgrade record of what has already been done. */
class PunishmentLatch
{
public:
    PunishmentLevel Level(int slot) const
    {
        return InSlotRange(slot) ? _levels[static_cast<size_t>(slot)] : PunishmentLevel::None;
    }

    /** Raises the slot to @p level; false when it was already at or above it. */
    bool Raise(int slot, PunishmentLevel level)
    {
        if (!InSlotRange(slot) || _levels[static_cast<size_t>(slot)] >= level)
            return false;
        _levels[static_cast<size_t>(slot)] = level;
        return true;
    }

    void Clear(int slot)
    {
        if (InSlotRange(slot))
            _levels[static_cast<size_t>(slot)] = PunishmentLevel::None;
    }

    void Reset() { _levels = {}; }

private:
    std::array<PunishmentLevel, MaxSlots> _levels{};
};

}  // namespace Anticheat

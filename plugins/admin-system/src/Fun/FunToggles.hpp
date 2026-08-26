#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

namespace AdminSystem::Fun
{

/** The server-wide round modifiers, in the order the Fun Mode menu lists them. */
enum class Toggle : uint8_t
{
    LowGravity,
    HeadshotOnly,
    KnifeRound,
    OneHitKill,
    Count,
};

inline constexpr std::size_t ToggleCount = static_cast<std::size_t>(Toggle::Count);

/** Which toggles are on. Plain state so the menu and the effects agree on one source. */
struct ToggleState
{
    bool Active[ToggleCount]{};

    bool IsOn(Toggle toggle) const { return Active[static_cast<std::size_t>(toggle)]; }
    void Set(Toggle toggle, bool on) { Active[static_cast<std::size_t>(toggle)] = on; }
    bool Flip(Toggle toggle)
    {
        auto& slot = Active[static_cast<std::size_t>(toggle)];
        slot = !slot;
        return slot;
    }
    void Clear() { *this = {}; }
    bool AnyOn() const
    {
        return std::ranges::any_of(Active, [](bool on) { return on; });
    }
};

/** The translation keys for one toggle: its menu label and its two broadcast lines. */
struct ToggleInfo
{
    Toggle Id;
    std::string_view NameKey;
    std::string_view OnKey;
    std::string_view OffKey;
};

/** Every toggle, in menu order and indexable by @ref Toggle. One table keyed by the enum, so a new
 *  modifier cannot be added with its label and its broadcast lines out of step. */
inline constexpr std::array<ToggleInfo, ToggleCount> Toggles{{
    {Toggle::LowGravity, "fun.lowGravity", "broadcast.lowGravityOn", "broadcast.lowGravityOff"},
    {Toggle::HeadshotOnly, "fun.headshotOnly", "broadcast.headshotOnlyOn", "broadcast.headshotOnlyOff"},
    {Toggle::KnifeRound, "fun.knifeRound", "broadcast.knifeRoundOn", "broadcast.knifeRoundOff"},
    {Toggle::OneHitKill, "fun.oneHitKill", "broadcast.oneHitKillOn", "broadcast.oneHitKillOff"},
}};

}  // namespace AdminSystem::Fun

#include "FunToggles.hpp"

#include <algorithm>
#include <array>
#include <cctype>

namespace AdminSystem::Fun
{

namespace
{

std::string Lower(std::string_view text)
{
    std::string out(text);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

constexpr std::array<std::string_view, ToggleCount> Words{
    "lowgravity", "headshotonly", "kniferound", "noscopeonly", "onehitkill", "infinitemoney", "chickenbots",
};

}  // namespace

bool ToggleState::AnyOn() const
{
    return std::any_of(std::begin(Active), std::end(Active), [](bool on) { return on; });
}

const std::vector<ToggleInfo>& Toggles()
{
    static const std::vector<ToggleInfo> toggles = {
        {Toggle::LowGravity, "fun.lowGravity", "broadcast.lowGravityOn", "broadcast.lowGravityOff"},
        {Toggle::HeadshotOnly, "fun.headshotOnly", "broadcast.headshotOnlyOn", "broadcast.headshotOnlyOff"},
        {Toggle::KnifeRound, "fun.knifeRound", "broadcast.knifeRoundOn", "broadcast.knifeRoundOff"},
        {Toggle::NoScopeOnly, "fun.noScopeOnly", "broadcast.noScopeOnlyOn", "broadcast.noScopeOnlyOff"},
        {Toggle::OneHitKill, "fun.oneHitKill", "broadcast.oneHitKillOn", "broadcast.oneHitKillOff"},
        {Toggle::InfiniteMoney, "fun.infiniteMoney", "broadcast.infiniteMoneyOn", "broadcast.infiniteMoneyOff"},
        {Toggle::ChickenBots, "fun.chickenBots", "broadcast.chickenBotsOn", "broadcast.chickenBotsOff"},
    };
    return toggles;
}

Toggle ParseToggle(std::string_view name)
{
    const std::string needle = Lower(name);
    for (std::size_t i = 0; i < Words.size(); ++i)
    {
        if (Words[i] == needle)
            return static_cast<Toggle>(i);
    }
    return Toggle::Count;
}

std::string_view ToggleWord(Toggle toggle)
{
    auto index = static_cast<std::size_t>(toggle);
    return index < Words.size() ? Words[index] : std::string_view{};
}

DamageDecision DecideDamage(const ToggleState& state, int hitGroup, bool attackerScoped, bool hasAttacker,
                            float incoming)
{
    DamageDecision decision{.Suppress = false, .Damage = incoming};

    // World damage (fall, fire, the bomb) has no aim to judge, so the aim rules leave it alone -
    // otherwise a headshot-only round would make players immortal to everything but bullets.
    if (hasAttacker)
    {
        if (state.IsOn(Toggle::HeadshotOnly) && hitGroup != HitGroupHead)
            decision.Suppress = true;
        if (state.IsOn(Toggle::NoScopeOnly) && attackerScoped)
            decision.Suppress = true;
    }

    // A suppressed hit is not allowed to land, so there is nothing to amplify.
    if (!decision.Suppress && state.IsOn(Toggle::OneHitKill))
        decision.Damage = OneHitKillDamage;

    return decision;
}

}  // namespace AdminSystem::Fun

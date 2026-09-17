#include "Detect/Rules/Namechanger.hpp"

#include <format>

namespace Anticheat::Rules
{

static constexpr int DetectionThreshold = 5;
static constexpr double CooldownSec = 60.0;

void Namechanger::Reset()
{
    _slots = {};
}

void Namechanger::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void Namechanger::OnBaseline(int slot, std::string_view name, std::string_view clan)
{
    if (!InSlotRange(slot) || name.empty())
        return;
    _slots[slot] = {.LastName = std::string(name), .LastClan = std::string(clan), .Initialized = true};
}

std::optional<Finding> Namechanger::OnIdentity(int slot, std::string_view name, std::string_view clan,
                                                   double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || name.empty())
        return out;

    auto& data = _slots[slot];
    if (!data.Initialized)
    {
        // A change before the baseline landed establishes it instead of counting.
        data.LastName.assign(name);
        data.LastClan.assign(clan);
        data.Initialized = true;
        return out;
    }

    const bool nameChanged = data.LastName != name;
    const bool clanChanged = data.LastClan != clan;
    if (!nameChanged && !clanChanged)
        return out;
    data.LastName.assign(name);
    data.LastClan.assign(clan);
    if (nowSec < data.CooldownUntil)
        return out;

    const int changes = data.Changes.Add(nowSec);
    if (changes < DetectionThreshold)
        return out;

    out = Finding{.Kind = DetectionKind::Namechanger,
                  .Evidence = std::format("{} visible name or clan tag changes occurred within one minute (last: "
                                          "name '{}', tag '{}').",
                                          changes, name, clan)};
    data.Changes.Clear();
    data.CooldownUntil = nowSec + CooldownSec;
    return out;
}

int Namechanger::ChangeCount(int slot) const
{
    return InSlotRange(slot) ? static_cast<int>(_slots[slot].Changes.Count()) : 0;
}

}  // namespace Anticheat::Rules

#include "Detect/Rules/Namechanger.hpp"

#include <format>

namespace Anticheat::Rules
{

static constexpr double BurstSeconds = 60.0;
static constexpr double CooldownSec = 60.0;

void Namechanger::Reset()
{
    _slots = {};
}

void Namechanger::ClearSlot(int slot)
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

void Namechanger::OnIdentity(int slot, std::string_view name, std::string_view clan, double nowSec)
{
    if (!InSlotRange(slot) || name.empty())
        return;

    auto& data = _slots[slot];
    if (!data.Initialized)
    {
        // A change before the baseline landed establishes it instead of counting.
        data.LastName.assign(name);
        data.LastClan.assign(clan);
        data.Initialized = true;
        return;
    }

    const bool nameChanged = data.LastName != name;
    const bool clanChanged = data.LastClan != clan;
    if (!nameChanged && !clanChanged)
        return;
    data.LastName.assign(name);
    data.LastClan.assign(clan);
    if (nowSec < data.CooldownUntil)
        return;

    data.Changes[data.Next] = nowSec;
    data.Next = (data.Next + 1) % BurstChanges;
    if (data.Held < BurstChanges)
        ++data.Held;

    if (data.Held < BurstChanges || nowSec - data.Changes[data.Next] > BurstSeconds)
        return;

    data.CooldownUntil = nowSec + CooldownSec;
    _suspicion.Add(slot,
                   {.Kind = Kind,
                    .Points = 1.0f,
                    .Reason = std::format("{} visible name or clan tag changes occurred within one minute "
                                          "(last: name '{}', tag '{}').",
                                          BurstChanges, name, clan)},
                   nowSec);
}

int Namechanger::RecentChanges(int slot, double nowSec) const
{
    if (!InSlotRange(slot))
        return 0;

    const auto& data = _slots[slot];
    int recent = 0;
    for (size_t i = 0; i < data.Held; ++i)
        if (nowSec - data.Changes[i] <= BurstSeconds)
            ++recent;
    return recent;
}

}  // namespace Anticheat::Rules

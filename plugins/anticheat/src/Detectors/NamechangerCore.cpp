#include "NamechangerCore.hpp"

#include <format>

namespace Anticheat
{

namespace
{
constexpr size_t DetectionThreshold = 5;
constexpr double EvidenceWindowSec = 60.0;
}  // namespace

void NamechangerCore::Reset()
{
    _slots = {};
}

void NamechangerCore::OnSlotChanged(int slot)
{
    if (InSlotRange(slot))
        _slots[slot] = {};
}

void NamechangerCore::OnBaseline(int slot, std::string_view name)
{
    if (!InSlotRange(slot) || name.empty())
        return;
    _slots[slot] = {.LastName = std::string(name), .Initialized = true};
}

std::optional<Finding> NamechangerCore::OnNameChanged(int slot, std::string_view name, double nowSec)
{
    std::optional<Finding> out;
    if (!InSlotRange(slot) || name.empty())
        return out;

    auto& data = _slots[slot];
    if (!data.Initialized)
    {
        // A settings change before the baseline landed establishes it instead of counting.
        data.LastName.assign(name);
        data.Initialized = true;
        return out;
    }
    if (data.LastName == name)
        return out;
    data.LastName.assign(name);

    while (!data.Changes.empty() && nowSec - data.Changes.front() >= EvidenceWindowSec)
        data.Changes.pop_front();
    data.Changes.push_back(nowSec);
    if (data.Changes.size() < DetectionThreshold)
        return out;

    out = Finding{.Kind = DetectionKind::Namechanger,
                  .Evidence = std::format("{} visible name changes occurred within one minute.", data.Changes.size())};
    data.Changes.clear();
    return out;
}

int NamechangerCore::ChangeCount(int slot) const
{
    return InSlotRange(slot) ? static_cast<int>(_slots[slot].Changes.size()) : 0;
}

}  // namespace Anticheat

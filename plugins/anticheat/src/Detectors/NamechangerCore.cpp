#include "NamechangerCore.hpp"

#include <format>

namespace Anticheat
{

namespace
{
constexpr int DetectionThreshold = 5;
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

    const int changes = data.Changes.Add(nowSec);
    if (changes < DetectionThreshold)
        return out;

    out = Finding{.Kind = DetectionKind::Namechanger,
                  .Evidence = std::format("{} visible name changes occurred within one minute.", changes)};
    data.Changes.Clear();
    return out;
}

int NamechangerCore::ChangeCount(int slot) const
{
    return InSlotRange(slot) ? static_cast<int>(_slots[slot].Changes.Count()) : 0;
}

}  // namespace Anticheat

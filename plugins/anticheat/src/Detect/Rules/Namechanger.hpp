#pragma once

#include "Detect/Evidence.hpp"
#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace Anticheat::Rules
{

class Namechanger
{
public:
    /** The settings toggle and catalog entry this rule reports under. */
    static constexpr DetectionKind Kind = DetectionKind::Namechanger;

    void Reset();
    void OnSlotChanged(int slot);

    /** The baseline the first change is measured against. */
    void OnBaseline(int slot, std::string_view name, std::string_view clan);

    /** What the scoreboard shows right now. Only an actually different name or tag counts. */
    std::optional<Finding> OnIdentity(int slot, std::string_view name, std::string_view clan, double nowSec);

    int ChangeCount(int slot) const;

private:
    /** Griefing is a burst, so the window is a minute rather than the aim modules' ten. */
    using ChangeWindow = EvidenceWindow<60>;

    struct SlotData
    {
        std::string LastName;
        std::string LastClan;
        ChangeWindow Changes;
        double CooldownUntil = 0.0;  // after a finding: an animated tag is one offence, not one a second
        bool Initialized = false;
    };

    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat::Rules

#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Samples.hpp"
#include "Detect/Suspicion.hpp"

#include <array>
#include <cstddef>
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

    explicit Namechanger(Suspicion& suspicion) : _suspicion(suspicion) {}

    void Reset();
    void OnSlotChanged(int slot);

    /** The baseline the first change is measured against. */
    void OnBaseline(int slot, std::string_view name, std::string_view clan);

    /** What the scoreboard shows right now. Only an actually different name or tag counts. */
    std::optional<Finding> OnIdentity(int slot, std::string_view name, std::string_view clan, double nowSec);

    /** Changes still inside the burst window. */
    int RecentChanges(int slot, double nowSec) const;

private:
    /** Griefing is a burst, so this rule judges over a minute rather than the aim rules' ten. */
    static constexpr size_t BurstChanges = 5;

    struct SlotData
    {
        std::string LastName;
        std::string LastClan;
        /** Ring of the last BurstChanges change times; once full, Next points at the oldest. */
        std::array<double, BurstChanges> Changes{};
        size_t Next = 0;
        size_t Held = 0;
        double CooldownUntil = 0.0;  // an animated tag is one offence, not one a second
        bool Initialized = false;
    };

    std::array<SlotData, MaxSlots> _slots{};
    Suspicion& _suspicion;
};

}  // namespace Anticheat::Rules

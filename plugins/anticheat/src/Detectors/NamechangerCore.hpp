#pragma once

// Rapid visible name changes are a griefing/evasion tool, not something the client UI produces by
// accident. SDK-free.

#include "Core/Evidence.hpp"
#include "Core/Finding.hpp"
#include "Core/Samples.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace Anticheat
{

class NamechangerCore
{
public:
    void Reset();
    void OnSlotChanged(int slot);

    /** The baseline the first change is measured against. */
    void OnBaseline(int slot, std::string_view name);

    /** A settings change. Only an actually different name counts. */
    std::optional<Finding> OnNameChanged(int slot, std::string_view name, double nowSec);

    int ChangeCount(int slot) const;

private:
    /** Griefing is a burst, so the window is a minute rather than the aim modules' ten. */
    using ChangeWindow = EvidenceWindow<60>;

    struct SlotData
    {
        std::string LastName;
        ChangeWindow Changes;
        bool Initialized = false;
    };

    std::array<SlotData, MaxSlots> _slots{};
};

}  // namespace Anticheat

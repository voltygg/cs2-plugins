#pragma once

#include "Engine/Detectors.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Subscription.hpp>

namespace Anticheat
{

class NamechangerDetector
{
public:
    NamechangerDetector(Detectors& detectors, VoltMod::Runtime& runtime) : _detectors(detectors), _rt(runtime) {}

    /** Install the periodic controller read. */
    void Initialize();

    /** Full connect: the first point the controller's name is meaningful. */
    void OnFullyConnected(VoltMod::Player& player);

    /** A replicated settings change; only an actually different name or tag counts. */
    void OnSettingsChanged(VoltMod::Player& player);

private:
    /** Both gates the identity reads answer to: detections at all, and this module. */
    bool Enabled() const;
    void CheckIdentity(int slot, double nowSec);
    void OnPoll();

    Detectors& _detectors;
    VoltMod::Runtime& _rt;
    VoltMod::Subscription _pollTimer;
};

}  // namespace Anticheat

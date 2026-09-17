#pragma once

#include "Detect/DetectionData.hpp"

#include <VoltMod/Core/Result.hpp>
#include <string_view>

namespace Anticheat
{

/** Loads @ref DetectionData, validating it before publishing. Reads a file and logs, so it lives
 *  beside the engine adapters rather than in the SDK-free half the tests compile. */
class DetectionDataManager
{
public:
    /** On failure the previously loaded tables stand unchanged. */
    VoltMod::Status Load(std::string_view path);

    const DetectionData& Get() const { return _data; }

private:
    DetectionData _data;
};

}  // namespace Anticheat

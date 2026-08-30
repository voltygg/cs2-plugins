#include "Core/DetectionData.hpp"

#include <VoltMod/Core/Log.hpp>
#include <format>
#include <string>
#include <utility>

namespace Anticheat
{

VoltMod::Status DetectionDataManager::Load(std::string_view path)
{
    auto document = VoltMod::Json::ReadFile<DetectionDocument, VoltMod::Json::StrictReadOptions>(path);
    if (!document)
        return std::unexpected(document.error());

    auto validated = ValidateDetectionData(std::move(*document));
    if (!validated)
        return std::unexpected(VoltMod::Error::Invalid(std::format("{}: {}", path, validated.error().Detail)));

    // Publish last: a rejected reload leaves the tables already in use untouched.
    _data = std::move(*validated);
    VoltMod::Log::Info("Loaded detection data from {}", path);
    return {};
}

}  // namespace Anticheat

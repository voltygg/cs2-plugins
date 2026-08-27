#pragma once

#include <VoltMod/Core/EnumNames.hpp>
#include <string_view>

namespace AdminSystem::Admin::CheatCheck
{

enum class CheatCheckMode
{
    FixedLink,
    WebsiteAutoRoom,
    PlayerProvided,
};

/** The configured mode, or FixedLink when the value names no mode. Case-insensitive. */
inline CheatCheckMode ParseMode(std::string_view value)
{
    return VoltMod::Parse<CheatCheckMode>(value).value_or(CheatCheckMode::FixedLink);
}

}  // namespace AdminSystem::Admin::CheatCheck

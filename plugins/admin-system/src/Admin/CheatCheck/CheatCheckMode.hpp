#pragma once

#include <string_view>

namespace AdminSystem::Admin::CheatCheck
{

enum class CheatCheckMode
{
    FixedLink,
    WebsiteAutoRoom,
    PlayerProvided,
};

inline CheatCheckMode ParseMode(std::string_view value)
{
    if (value == "websiteAutoRoom")
        return CheatCheckMode::WebsiteAutoRoom;
    if (value == "playerProvided")
        return CheatCheckMode::PlayerProvided;
    return CheatCheckMode::FixedLink;
}

}  // namespace AdminSystem::Admin::CheatCheck

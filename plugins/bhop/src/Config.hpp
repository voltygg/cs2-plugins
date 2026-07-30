#pragma once

#include <CS2Kit/Api.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace Bhop
{

/** "bhop.hopBoost" section: server-side velocity boost per chained hop. */
struct HopBoostSettings
{
    bool enabled = true;
    float factor = 1.08f;      // horizontal velocity multiplier per chained hop
    int chainWindowMs = 1000;  // a jump within this window of the previous one chains
    float maxSpeed = 1200.0f;  // horizontal speed cap for the boost
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(HopBoostSettings, enabled, factor, chainWindowMs, maxSpeed)

/** "bhop" section of settings.jsonc. Float fields use -1 = leave the server value untouched. */
struct BhopSettings
{
    std::string mode = "enabled";  // "enabled" = always-on for everyone; "grants" = per-player only
    bool autoBunnyhopping = true;
    bool enableBunnyhopping = true;
    float staminaJumpCost = 0.0f;
    float staminaLandCost = 0.0f;
    float airAccelerate = 150.0f;
    float airMaxWishSpeed = 60.0f;
    float maxVelocity = -1.0f;
    HopBoostSettings hopBoost;
    bool notifyPlayer = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(BhopSettings, mode, autoBunnyhopping, enableBunnyhopping,
                                                staminaJumpCost, staminaLandCost, airAccelerate, airMaxWishSpeed,
                                                maxVelocity, hopBoost, notifyPlayer)

/** Root of settings.jsonc. Add a struct + a member here for each new section. */
struct Settings
{
    CS2Kit::StandardPluginSettings plugin;
    BhopSettings bhop;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Settings, plugin, bhop)

using ConfigManager = CS2Kit::JsonConfig<Settings>;

/** The addon folder name - matches the CMake target and keys every addons/ path. */
inline constexpr std::string_view AddonName = "bhop";

}  // namespace Bhop

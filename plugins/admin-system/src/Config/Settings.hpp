#pragma once

#include "AbuseProtectionSettings.hpp"
#include "ChatSettings.hpp"
#include "CheatCheckSettings.hpp"
#include "DatabaseSettings.hpp"
#include "MapSettings.hpp"
#include "MenuSettings.hpp"
#include "PunishmentSettings.hpp"
#include "ReportSettings.hpp"
#include "ServerSettings.hpp"
#include "WeaponSettings.hpp"

#include <VoltMod/App/Config.hpp>
#include <string_view>

namespace AdminSystem::Config
{
inline constexpr std::string_view AddonName = "admin-system";

using PluginSettings = VoltMod::StandardPluginSettings;

/** The whole of `configs/settings.jsonc`. Each member is one top-level object in the file;
 *  the section headers next to this one document the individual keys. */
struct Settings
{
    PluginSettings plugin;
    ServerSettings server;
    DatabaseConfig database;
    PunishmentSettings punishments;
    AbuseProtectionSettings abuseProtection;
    ChatSettings chat;
    ReportSettings reports;
    CheatCheckSettings cheatCheck;
    MapSettings maps;
    WeaponSettings weapons;
    MenuSettings menu;
};

}  // namespace AdminSystem::Config

/** Accepts the `"$schema"` key settings.jsonc names for editor completion. */
VOLTMOD_SETTINGS_ROOT(AdminSystem::Config::Settings)

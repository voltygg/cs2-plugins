#pragma once

#include "../Maps/MapQuery.hpp"
#include "../Weapons/WeaponCatalog.hpp"
#include "Settings.hpp"

#include <VoltMod/App/Config.hpp>
#include <VoltMod/Core/Result.hpp>
#include <string_view>
#include <vector>

namespace AdminSystem::Config
{

/** Loads settings and resolves string-based punishment values. Admin and group
 *  records remain owned by the database. */
class ConfigManager
{
public:
    ConfigManager() = default;

    /**
     * @brief Parse @p path, validate it, then publish the result as one snapshot.
     *
     * Fails for a missing, malformed, or mistyped file; the error names the offending key with
     * its line and column. On failure the previously published configuration stands unchanged -
     * nothing observes a half-validated one.
     */
    VoltMod::Status LoadSettings(std::string_view path);

    /** The effective settings. `LoadStandardConfig` reads `plugin.locale` through this. */
    const Settings& Get() const { return _snapshot.Values; }

    const PluginSettings& GetPlugin() const { return _snapshot.Values.plugin; }
    const ServerSettings& GetServer() const { return _snapshot.Values.server; }
    const DatabaseConfig& GetDatabase() const { return _snapshot.Values.database; }
    const PunishmentSettings& GetPunishments() const { return _snapshot.Values.punishments; }
    const AppealSettings& GetAppeal() const { return _snapshot.Values.punishments.appeal; }
    const MapSettings& GetMaps() const { return _snapshot.Values.maps; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return _snapshot.Values.abuseProtection; }
    const ChatSettings& GetChat() const { return _snapshot.Values.chat; }
    const ReportSettings& GetReports() const { return _snapshot.Values.reports; }
    const CheatCheckSettings& GetCheatCheck() const { return _snapshot.Values.cheatCheck; }

    /** Valid templates. Invalid entries are logged and skipped. */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _snapshot.Templates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _snapshot.MenuDurationSecs; }

    /** Which menu host to open menus on, and the layout behind it. */
    const MenuSettings& GetMenu() const { return _snapshot.Values.menu; }

    /** `menu.style` parsed once at load; an unrecognized value has already warned and reads Auto. */
    MenuStyle GetMenuStyle() const { return _snapshot.Style; }

    /** Offerable maps. Invalid entries are logged and skipped. */
    const std::vector<Maps::MapEntry>& GetMapCycle() const { return _snapshot.Maps; }

    /** Giveable weapons. Invalid entries are logged and skipped. */
    const std::vector<Weapons::WeaponEntry>& GetWeaponMenu() const { return _snapshot.Weapons; }

private:
    /**
     * One consistent view of the configuration: the normalized settings and everything derived
     * from them. Replaced wholesale, never edited in place - which is why validation runs on a
     * local that has not been published yet.
     */
    struct ConfigSnapshot
    {
        Settings Values;
        MenuStyle Style = MenuStyle::Auto;
        std::vector<ResolvedTemplate> Templates;
        std::vector<int> MenuDurationSecs;
        std::vector<Maps::MapEntry> Maps;
        std::vector<Weapons::WeaponEntry> Weapons;
    };

    /** Normalize @p raw and derive everything that hangs off it. Takes by value: the caller's
     *  parse result is consumed, and nothing here can touch published state. */
    static ConfigSnapshot BuildSnapshot(Settings raw);

    ConfigSnapshot _snapshot;
};

}  // namespace AdminSystem::Config

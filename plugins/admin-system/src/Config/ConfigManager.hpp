#pragma once

#include "Config/Settings.hpp"
#include "Maps/MapQuery.hpp"
#include "Weapons/WeaponCatalog.hpp"

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
    VoltMod::Status LoadSettings(std::string_view path) { return _options.Load(path); }

    /** The effective settings. */
    const Settings& Get() const { return _options.Get().Values; }

    const PluginSettings& GetPlugin() const { return _options.Get().Values.plugin; }
    const ServerSettings& GetServer() const { return _options.Get().Values.server; }
    const VoltMod::DatabaseConfig& GetDatabase() const { return _options.Get().Values.database; }
    const PunishmentSettings& GetPunishments() const { return _options.Get().Values.punishments; }
    const AppealSettings& GetAppeal() const { return _options.Get().Values.punishments.appeal; }
    const MapSettings& GetMaps() const { return _options.Get().Values.maps; }
    const MenuSettings& GetMenu() const { return _options.Get().Values.menu; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return _options.Get().Values.abuseProtection; }
    const ChatSettings& GetChat() const { return _options.Get().Values.chat; }
    const ReportSettings& GetReports() const { return _options.Get().Values.reports; }
    const CheatCheckSettings& GetCheatCheck() const { return _options.Get().Values.cheatCheck; }

    /** Valid templates. Invalid entries are logged and skipped. */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _options.Get().Templates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _options.Get().MenuDurationSecs; }

    /** Offerable maps. Invalid entries are logged and skipped. */
    const std::vector<Maps::MapEntry>& GetMapCycle() const { return _options.Get().Maps; }

    /** Giveable weapons. Invalid entries are logged and skipped. */
    const std::vector<Weapons::WeaponEntry>& GetWeaponMenu() const { return _options.Get().Weapons; }

private:
    struct ConfigSnapshot
    {
        Settings Values;
        std::vector<ResolvedTemplate> Templates;
        std::vector<int> MenuDurationSecs;
        std::vector<Maps::MapEntry> Maps;
        std::vector<Weapons::WeaponEntry> Weapons;
    };

    static ConfigSnapshot BuildSnapshot(Settings raw);

    VoltMod::Options<Settings, ConfigSnapshot> _options{&BuildSnapshot};
};

}  // namespace AdminSystem::Config

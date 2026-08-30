#pragma once

#include "../Maps/MapQuery.hpp"
#include "../Weapons/WeaponCatalog.hpp"
#include "Settings.hpp"

#include <VoltMod/App/Config.hpp>
#include <string>
#include <vector>

namespace AdminSystem::Config
{

/** Loads settings and resolves string-based punishment values. Admin and group
 *  records remain owned by the database. */
class ConfigManager : public VoltMod::JsonConfig<Settings>
{
public:
    ConfigManager() = default;

    /** Return false for missing, malformed, or mistyped settings. */
    bool LoadSettings(const std::string& path);

    const PluginSettings& GetPlugin() const { return Get().plugin; }
    const ServerSettings& GetServer() const { return Get().server; }
    const DatabaseConfig& GetDatabase() const { return Get().database; }
    const PunishmentSettings& GetPunishments() const { return Get().punishments; }
    const AppealSettings& GetAppeal() const { return Get().punishments.appeal; }
    const MapSettings& GetMaps() const { return Get().maps; }
    const AbuseProtectionSettings& GetAbuseProtection() const { return Get().abuseProtection; }
    const ChatSettings& GetChat() const { return Get().chat; }
    const ReportSettings& GetReports() const { return Get().reports; }
    const CheatCheckSettings& GetCheatCheck() const { return Get().cheatCheck; }

    /** Valid templates. Invalid entries are logged and skipped. */
    const std::vector<ResolvedTemplate>& GetPunishmentTemplates() const { return _resolvedTemplates; }

    /** Menu duration-picker rows in seconds (0 = permanent), parsed from `punishments.menuDurations`. */
    const std::vector<int>& GetMenuDurations() const { return _menuDurationSecs; }

    /** Which menu host to open menus on, and the layout behind it. */
    const MenuSettings& GetMenu() const { return Get().menu; }

    /** `menu.style` parsed once at load; an unrecognized value has already warned and reads Auto. */
    MenuStyle GetMenuStyle() const { return _menuStyle; }

    /** Offerable maps. Invalid entries are logged and skipped. */
    const std::vector<Maps::MapEntry>& GetMapCycle() const { return _resolvedMaps; }

    /** Giveable weapons. Invalid entries are logged and skipped. */
    const std::vector<Weapons::WeaponEntry>& GetWeaponMenu() const { return _resolvedWeapons; }

private:
    void ResolveRuntimeSettings();

    MenuStyle _menuStyle = MenuStyle::Auto;
    std::vector<ResolvedTemplate> _resolvedTemplates;
    std::vector<int> _menuDurationSecs;
    std::vector<Maps::MapEntry> _resolvedMaps;
    std::vector<Weapons::WeaponEntry> _resolvedWeapons;
};

}  // namespace AdminSystem::Config

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

    /** Parse and validate @p path, then publish it. A failed load keeps the previous settings. */
    VoltMod::Status LoadSettings(std::string_view path) { return _options.Load(path); }

    /** Read the file @ref LoadSettings read, again. */
    VoltMod::Status Reload() { return _options.Reload(); }

    const Settings& Get() const { return _options.Get().Values; }

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

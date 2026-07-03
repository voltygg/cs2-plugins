#include "Config.hpp"

#include <CS2Kit/Utils/Json.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/StringUtils.hpp>

namespace AdminSystem::Core
{

using namespace CS2Kit::Utils;

bool ConfigManager::LoadSettings(const std::string& path)
{
    auto loaded = Json::TryDeserializeFile<Settings>(path);
    if (!loaded)
        return false;

    _settings = std::move(*loaded);
    ResolveRuntimeSettings();
    Log::Info("Loaded settings from {}", path);
    return true;
}

// Bad entries are skipped with a warning rather than failing the load: a typo'd template
// must not take down ban/mute enforcement for the whole server.
void ConfigManager::ResolveRuntimeSettings()
{
    // A blank/oversized tag would silently orphan per-server grants (the DB column is
    // VARCHAR(64)), so normalize to the documented default instead of failing the load.
    auto& tag = _settings.server.tag;
    if (StringUtils::Trim(tag).empty() || tag.size() > 64)
    {
        Log::Warn("settings: server.tag is empty or longer than 64 chars; using \"default\"");
        tag = "default";
    }

    _resolvedTemplates.clear();
    const auto& punishments = _settings.punishments;
    for (std::size_t i = 0; i < punishments.templates.size(); ++i)
    {
        const auto& t = punishments.templates[i];
        auto type = Punishments::ParsePunishType(t.type);
        if (!type || !Punishments::IsTimed(*type))
        {
            Log::Warn(
                "settings: skipping punishments.templates[{}] ('{}'): type must be ban/voiceMute/textMute, got '{}'", i,
                t.name, t.type);
            continue;
        }

        int durationSec = ParseDuration(t.duration);
        if (durationSec < 0)
        {
            Log::Warn("settings: skipping punishments.templates[{}] ('{}'): bad duration '{}'", i, t.name, t.duration);
            continue;
        }

        if (t.name.empty() || t.reason.empty())
        {
            Log::Warn("settings: skipping punishments.templates[{}]: name and reason must be non-empty", i);
            continue;
        }

        _resolvedTemplates.push_back({t.name, *type, durationSec, t.reason});
    }

    _menuDurationSecs.clear();
    for (std::size_t i = 0; i < punishments.menuDurations.size(); ++i)
    {
        int seconds = ParseDuration(punishments.menuDurations[i]);
        if (seconds < 0)
        {
            Log::Warn("settings: skipping punishments.menuDurations[{}]: bad duration '{}'", i,
                      punishments.menuDurations[i]);
            continue;
        }
        _menuDurationSecs.push_back(seconds);
    }

    // An empty duration picker would dead-end the menu ban/mute flow; fall back to the
    // struct defaults so the list exists in exactly one place.
    if (_menuDurationSecs.empty())
    {
        Log::Warn("settings: punishments.menuDurations has no valid entries; using built-in defaults");
        for (const auto& entry : PunishmentSettings{}.menuDurations)
            _menuDurationSecs.push_back(ParseDuration(entry));
    }
}

}  // namespace AdminSystem::Core

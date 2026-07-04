#include "Config.hpp"

#include <CS2Kit/Utils/Validation.hpp>
#include <format>
#include <optional>

namespace AdminSystem::Core
{

using namespace CS2Kit::Utils;

bool ConfigManager::LoadSettings(const std::string& path)
{
    if (!Load(path))
        return false;

    ResolveRuntimeSettings();
    return true;
}

// Bad entries are skipped with a warning rather than failing the load: a typo'd template
// must not take down ban/mute enforcement for the whole server.
void ConfigManager::ResolveRuntimeSettings()
{
    auto& settings = Mutable();

    // A blank/oversized tag would silently orphan per-server grants (the DB column is VARCHAR(64)).
    Validation::NormalizeTag(settings.server.tag, 64, "default", "server.tag");

    auto& punishments = settings.punishments;
    Validation::FilterValid(
        punishments.templates,
        [](const PunishmentTemplate& t, std::size_t) -> std::optional<std::string> {
            auto type = Punishments::ParsePunishType(t.type);
            if (!type || !Punishments::IsTimed(*type))
                return std::format("type must be ban/voiceMute/textMute, got '{}'", t.type);
            if (ParseDuration(t.duration) < 0)
                return std::format("bad duration '{}'", t.duration);
            if (t.name.empty() || t.reason.empty())
                return std::string("name and reason must be non-empty");
            return std::nullopt;
        },
        "punishments.templates");

    _resolvedTemplates.clear();
    for (const auto& t : punishments.templates)
        _resolvedTemplates.push_back(
            {t.name, *Punishments::ParsePunishType(t.type), ParseDuration(t.duration), t.reason});

    // An empty duration picker would dead-end the menu ban/mute flow; the helper falls back to
    // the struct defaults so the list exists in exactly one place.
    _menuDurationSecs = Validation::ParseDurations(punishments.menuDurations, PunishmentSettings{}.menuDurations,
                                                   "punishments.menuDurations");
}

}  // namespace AdminSystem::Core

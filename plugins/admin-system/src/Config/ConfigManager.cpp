#include "ConfigManager.hpp"

#include <VoltMod/Core/EnumNames.hpp>
#include <VoltMod/Core/Validation.hpp>
#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using VoltMod::ParseDuration;
namespace Validation = VoltMod::Validation;

namespace AdminSystem::Config
{

// maps.cycle and weapons.menu are the same shape of list: convert the raw config rows to their
// runtime type, drop the ones their domain check rejects, and fall back to the struct defaults
// when nothing valid is left - an empty Map or Give page reads as a broken plugin rather than as
// a configuration gap, most often on a server whose settings.jsonc predates the section.
// Whether a map file exists is the engine's call; MapCycleState re-checks the resolved names at
// load so a stale entry surfaces in the load report.
template <class Entry, class Raw, class ToEntry>
static std::vector<Entry> ResolveList(const std::vector<Raw>& rows, const std::vector<Raw>& defaults, ToEntry toEntry,
                                      std::string (*validate)(const Entry&), std::string_view what)
{
    auto build = [&](const std::vector<Raw>& source) {
        std::vector<Entry> entries;
        entries.reserve(source.size());
        for (const auto& row : source)
            entries.push_back(toEntry(row));

        Validation::FilterValid(
            entries,
            [validate](const Entry& entry, std::size_t) -> std::optional<std::string> {
                auto problem = validate(entry);
                if (problem.empty())
                    return std::nullopt;
                return problem;
            },
            what);
        return entries;
    };

    auto entries = build(rows);
    Validation::FallbackIfEmpty(entries, [&] { return build(defaults); }, what);
    return entries;
}

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

    auto& reports = settings.reports;
    Validation::FilterValid(
        reports.reasons,
        [](const ReportReason& r, std::size_t) -> std::optional<std::string> {
            if (r.code.empty() || r.label.empty())
                return std::string("code and label must be non-empty");
            if (r.code.size() > 32)  // the reason_code column is VARCHAR(32)
                return std::format("code '{}' is longer than 32 chars", r.code);
            return std::nullopt;
        },
        "reports.reasons");

    // A reason picker with only the free-text row (or nothing at all) is a dead end.
    Validation::FallbackIfEmpty(reports.reasons, [] { return ReportSettings{}.reasons; }, "reports.reasons");

    // Negative windows would make every elapsed check pass; treat them as "disabled".
    reports.cooldownSec = std::max(reports.cooldownSec, 0);
    reports.duplicateWindowSec = std::max(reports.duplicateWindowSec, 0);

    // By name, so a typo cannot pick a host by accident: a misspelling read as Panorama falls back
    // silently on a server without the capability, and the bad value never surfaces.
    if (auto style = VoltMod::Parse<MenuStyle>(settings.menu.style))
    {
        _menuStyle = *style;
    }
    else
    {
        VoltMod::Log::Warn("menu.style '{}' is not auto/panorama/centerHtml; using auto.", settings.menu.style);
        _menuStyle = MenuStyle::Auto;
    }

    _resolvedMaps = ResolveList<Maps::MapEntry>(
        settings.maps.cycle, MapSettings{}.cycle,
        [](const MapConfigEntry& e) {
            return Maps::MapEntry{.Name = e.name, .DisplayName = e.displayName, .WorkshopId = e.workshopId};
        },
        Maps::ValidateMapEntry, "maps.cycle");

    _resolvedWeapons = ResolveList<Weapons::WeaponEntry>(
        settings.weapons.menu, WeaponSettings{}.menu,
        [](const WeaponConfigEntry& e) { return Weapons::WeaponEntry{.Name = e.name, .Item = e.item}; },
        Weapons::ValidateWeaponEntry, "weapons.menu");
}

}  // namespace AdminSystem::Config

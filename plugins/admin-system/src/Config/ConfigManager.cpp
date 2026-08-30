#include "ConfigManager.hpp"

#include <VoltMod/Core/EnumNames.hpp>
#include <VoltMod/Core/Validation.hpp>
#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

// Each helper below takes what it normalizes and hands it back. They run against a local Settings
// that has not been published, so a rejected entry cannot be observed half-removed.

/** A blank or oversized tag would silently orphan per-server grants (the DB column is VARCHAR(64)). */
static std::string NormalizedTag(std::string tag)
{
    Validation::NormalizeTag(tag, 64, "default", "server.tag");
    return tag;
}

/** Bad entries are skipped with a warning rather than failing the load: a typo'd template must
 *  not take down ban/mute enforcement for the whole server. */
static std::vector<PunishmentTemplate> ValidTemplates(std::vector<PunishmentTemplate> rows)
{
    Validation::FilterValid(
        rows,
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
    return rows;
}

/** @p rows must already have passed @ref ValidTemplates; the parses below cannot fail. */
static std::vector<ResolvedTemplate> ResolveTemplates(const std::vector<PunishmentTemplate>& rows)
{
    std::vector<ResolvedTemplate> resolved;
    resolved.reserve(rows.size());
    for (const auto& t : rows)
        resolved.push_back({t.name, *Punishments::ParsePunishType(t.type), ParseDuration(t.duration), t.reason});
    return resolved;
}

static ReportSettings NormalizedReports(ReportSettings reports)
{
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
    return reports;
}

// By name, so a typo cannot pick a host by accident: a misspelling read as Panorama falls back
// silently on a server without the capability, and the bad value never surfaces.
static MenuStyle ResolveMenuStyle(std::string_view style)
{
    if (auto parsed = VoltMod::Parse<MenuStyle>(style))
        return *parsed;

    VoltMod::Log::Warn("menu.style '{}' is not auto/panorama/centerHtml; using auto.", style);
    return MenuStyle::Auto;
}

VoltMod::Status ConfigManager::LoadSettings(std::string_view path)
{
    auto raw = VoltMod::Json::ReadFile<Settings>(path);
    if (!raw)
        return std::unexpected(raw.error());

    // Publish only after everything resolved: a failed reload leaves the previous snapshot whole.
    _snapshot = BuildSnapshot(std::move(*raw));
    VoltMod::Log::Info("Loaded settings from {}", path);
    return {};
}

ConfigManager::ConfigSnapshot ConfigManager::BuildSnapshot(Settings raw)
{
    ConfigSnapshot snapshot{.Values = std::move(raw)};
    auto& settings = snapshot.Values;

    settings.server.tag = NormalizedTag(std::move(settings.server.tag));
    settings.punishments.templates = ValidTemplates(std::move(settings.punishments.templates));
    settings.reports = NormalizedReports(std::move(settings.reports));

    snapshot.Templates = ResolveTemplates(settings.punishments.templates);

    // An empty duration picker would dead-end the menu ban/mute flow; the helper falls back to
    // the struct defaults so the list exists in exactly one place.
    snapshot.MenuDurationSecs = Validation::ParseDurations(
        settings.punishments.menuDurations, PunishmentSettings{}.menuDurations, "punishments.menuDurations");

    snapshot.Style = ResolveMenuStyle(settings.menu.style);

    snapshot.Maps = ResolveList<Maps::MapEntry>(
        settings.maps.cycle, MapSettings{}.cycle,
        [](const MapConfigEntry& e) {
            return Maps::MapEntry{.Name = e.name, .DisplayName = e.displayName, .WorkshopId = e.workshopId};
        },
        Maps::ValidateMapEntry, "maps.cycle");

    snapshot.Weapons = ResolveList<Weapons::WeaponEntry>(
        settings.weapons.menu, WeaponSettings{}.menu,
        [](const WeaponConfigEntry& e) { return Weapons::WeaponEntry{.Name = e.name, .Item = e.item}; },
        Weapons::ValidateWeaponEntry, "weapons.menu");

    return snapshot;
}

}  // namespace AdminSystem::Config

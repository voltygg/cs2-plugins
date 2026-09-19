#include "Config.hpp"

#include <VoltMod/App/Config/Validation.hpp>
#include <VoltMod/Core/Log.hpp>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <utility>

namespace Log = VoltMod::Log;
namespace Validation = VoltMod::Validation;

namespace MainMenu
{

static std::optional<std::string> EntryProblem(const EntrySettings& entry)
{
    if (entry.label.empty())
        return "no label";
    if (entry.kind == "command")
    {
        // The command runs as one console line; a separator would smuggle in a second one.
        if (entry.command.empty() || entry.command.find_first_of(";\r\n") != std::string::npos)
            return std::format("command '{}' must be one console command", entry.command);
        return std::nullopt;
    }
    if (entry.kind == "link")
    {
        if (!entry.url.starts_with("https://") && !entry.url.starts_with("http://"))
            return std::format("url '{}' must start with http:// or https://", entry.url);
        return std::nullopt;
    }
    if (entry.kind == "section")
        return entry.section.empty() ? std::optional<std::string>("no section id") : std::nullopt;
    return std::format("unknown kind '{}'", entry.kind);
}

Settings CleanSettings(Settings raw)
{
    if (raw.tabs.size() > static_cast<std::size_t>(MaxTabs))
    {
        Log::Warn("tabs: only the first {} of {} are shown", MaxTabs, raw.tabs.size());
        raw.tabs.resize(static_cast<std::size_t>(MaxTabs));
    }

    for (TabSettings& tab : raw.tabs)
    {
        Validation::FilterValid(
            tab.entries, [](const EntrySettings& entry, std::size_t) { return EntryProblem(entry); },
            std::format("tabs.{}.entries", tab.label));
    }
    return raw;
}

}  // namespace MainMenu

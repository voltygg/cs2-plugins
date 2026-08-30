#pragma once

#include <string>
#include <vector>

namespace AdminSystem::Config
{
/** Report reason. `label` is the fallback for a missing `report.reasons.<code>` translation. */
struct ReportReason
{
    std::string code;
    std::string label;
};

/** Player reports stored for the website; the plugin has no in-game report list. */
struct ReportSettings
{
    bool enabled = true;
    /** Seconds between reports by one player. Zero disables the limit. */
    int cooldownSec = 120;
    /** Seconds before one player may report the same target again. Zero disables the limit. */
    int duplicateWindowSec = 1800;
    /** Adds an "Other..." row storing typed text under the "other" code (capped at 64 chars). */
    bool allowCustomReason = true;
    std::vector<ReportReason> reasons = {
        {"cheating", "Cheating / aimbot"}, {"wallhack", "Wallhack"}, {"griefing", "Griefing / team damage"},
        {"abuse", "Toxic behavior"},       {"micspam", "Mic spam"},  {"nickname", "Inappropriate nickname"},
    };
};

}  // namespace AdminSystem::Config

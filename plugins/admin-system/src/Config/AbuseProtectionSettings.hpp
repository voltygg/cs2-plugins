#pragma once

namespace AdminSystem::Config
{
/** Automatic admin-freeze thresholds. Zero disables a counter; root admins are exempt. */
struct AbuseProtectionSettings
{
    bool enabled = true;
    int windowMinutes = 10;
    int maxBans = 5;
    int maxKicks = 10;
    int maxMutes = 15;  // voice + text combined
    int maxWarnings = 15;
};

}  // namespace AdminSystem::Config

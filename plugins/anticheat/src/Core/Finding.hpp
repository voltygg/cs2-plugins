#pragma once

#include <string>

namespace Anticheat
{

/** The detections the plugin ships. Order matches the config toggles. */
enum class DetectionKind
{
    Aimbot,
    Aimlock,
    AntiAim,
    SilentAim,
    DllInjection,
    InvalidCvar,
    Namechanger,
};

/** Every kind, for the config toggles and the status report. */
inline constexpr DetectionKind AllDetectionKinds[] = {
    DetectionKind::Aimbot,       DetectionKind::Aimlock,     DetectionKind::AntiAim,     DetectionKind::SilentAim,
    DetectionKind::DllInjection, DetectionKind::InvalidCvar, DetectionKind::Namechanger,
};

/** Console/webhook label for @p kind. */
constexpr const char* DisplayName(DetectionKind kind)
{
    switch (kind)
    {
    case DetectionKind::Aimbot:
        return "AIMBOT";
    case DetectionKind::Aimlock:
        return "AIMLOCK";
    case DetectionKind::AntiAim:
        return "ANTIAIM";
    case DetectionKind::SilentAim:
        return "SILENTAIM";
    case DetectionKind::DllInjection:
        return "DLL INJECTION";
    case DetectionKind::InvalidCvar:
        return "INVALID CVAR";
    case DetectionKind::Namechanger:
        return "NAMECHANGER";
    }
    return "UNKNOWN";
}

/** Space-free token for the admin-system console bridge, whose arguments split on whitespace. */
constexpr const char* TokenName(DetectionKind kind)
{
    switch (kind)
    {
    case DetectionKind::Aimbot:
        return "aimbot";
    case DetectionKind::Aimlock:
        return "aimlock";
    case DetectionKind::AntiAim:
        return "antiaim";
    case DetectionKind::SilentAim:
        return "silentaim";
    case DetectionKind::DllInjection:
        return "dll_injection";
    case DetectionKind::InvalidCvar:
        return "invalid_cvar";
    case DetectionKind::Namechanger:
        return "namechanger";
    }
    return "unknown";
}

/**
 * A confirmed detection. Cores self-threshold on their own rolling windows, so a Finding is
 * already a verdict - the response funnel decides the punishment, not whether one is warranted.
 *
 * KickOnly caps the punishment at a kick even in ban mode: it marks rules (a client cvar out of
 * range) whose false-positive cost must stay recoverable.
 */
struct Finding
{
    DetectionKind Kind = DetectionKind::Aimbot;
    bool KickOnly = false;
    std::string Evidence;
};

}  // namespace Anticheat

#pragma once

#include <cstddef>
#include <iterator>
#include <string>

namespace Anticheat
{

enum class DetectionKind
{
    Aimbot,
    Aimlock,
    AntiAim,
    SilentAim,
    DllInjection,
    InvalidCvar,
    Namechanger,
    Count,
};

/** How one detection is named. Token is space-free so it works as a detector key in alerts,
 *  webhooks and log lines. */
struct DetectionInfo
{
    DetectionKind Kind;
    const char* Display;
    const char* Token;
};

/** The one place a detection is named; adding a kind without a row here fails to compile. */
inline constexpr DetectionInfo DetectionCatalog[] = {
    {DetectionKind::Aimbot, "AIMBOT", "aimbot"},
    {DetectionKind::Aimlock, "AIMLOCK", "aimlock"},
    {DetectionKind::AntiAim, "ANTIAIM", "antiaim"},
    {DetectionKind::SilentAim, "SILENTAIM", "silentaim"},
    {DetectionKind::DllInjection, "DLL INJECTION", "dll_injection"},
    {DetectionKind::InvalidCvar, "INVALID CVAR", "invalid_cvar"},
    {DetectionKind::Namechanger, "NAMECHANGER", "namechanger"},
};

namespace Internal
{
constexpr bool CatalogMatchesEnum()
{
    if (std::size(DetectionCatalog) != static_cast<size_t>(DetectionKind::Count))
        return false;
    for (size_t index = 0; index < std::size(DetectionCatalog); ++index)
        if (DetectionCatalog[index].Kind != static_cast<DetectionKind>(index))
            return false;
    return true;
}
}  // namespace Internal
static_assert(Internal::CatalogMatchesEnum(), "DetectionCatalog must list every DetectionKind in order");

constexpr const DetectionInfo& Info(DetectionKind kind)
{
    return DetectionCatalog[static_cast<size_t>(kind)];
}

constexpr const char* DisplayName(DetectionKind kind)
{
    return Info(kind).Display;
}

constexpr const char* TokenName(DetectionKind kind)
{
    return Info(kind).Token;
}

/**
 * A confirmed detection: cores self-threshold on their own rolling windows, so the response funnel
 * decides the punishment, not whether one is warranted. KickOnly caps it at a kick even in ban
 * mode, for rules whose false-positive cost must stay recoverable.
 */
struct Finding
{
    DetectionKind Kind = DetectionKind::Aimbot;
    bool KickOnly = false;
    std::string Evidence;
};

}  // namespace Anticheat

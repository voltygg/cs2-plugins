#pragma once

#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>

namespace Anticheat
{

enum class DetectionKind
{
    Aimbot,
    Aimlock,
    AntiAim,
    SilentAim,
    Triggerbot,
    Recoil,
    AimAssist,
    Wallhack,
    DllInjection,
    InvalidCvar,
    Namechanger,
    Count,
};

/** How strong the evidence is, and so the strongest response allowed. Read off the fused total,
 *  so partial rules can raise a band together. */
enum class Confidence
{
    Suspect,
    Likely,
    Certain,
};

constexpr std::string_view ConfidenceName(Confidence level)
{
    switch (level)
    {
    case Confidence::Suspect:
        return "suspect";
    case Confidence::Likely:
        return "likely";
    case Confidence::Certain:
        return "certain";
    }
    return "suspect";
}

/** How one detection is named. Token is space-free so it works as a detector key in alerts,
 *  webhooks and log lines. */
struct DetectionInfo
{
    DetectionKind Kind;
    std::string_view Display;
    std::string_view Token;
};

/** The one place a detection is named; adding a kind without a row here fails to compile. */
inline constexpr DetectionInfo DetectionCatalog[] = {
    {DetectionKind::Aimbot, "AIMBOT", "aimbot"},
    {DetectionKind::Aimlock, "AIMLOCK", "aimlock"},
    {DetectionKind::AntiAim, "ANTIAIM", "antiaim"},
    {DetectionKind::SilentAim, "SILENTAIM", "silentaim"},
    {DetectionKind::Triggerbot, "TRIGGERBOT", "triggerbot"},
    {DetectionKind::Recoil, "RECOIL CONTROL", "recoil"},
    {DetectionKind::AimAssist, "AIM ASSIST", "aim_assist"},
    {DetectionKind::Wallhack, "WALLHACK", "wallhack"},
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

constexpr std::string_view DisplayName(DetectionKind kind)
{
    return Info(kind).Display;
}

constexpr std::string_view TokenName(DetectionKind kind)
{
    return Info(kind).Token;
}

/** A reportable detection. Kind names the rule that just fired, Evidence names the rest, and
 *  KickOnly caps the response at a kick for rules whose false positives must stay recoverable. */
struct Finding
{
    DetectionKind Kind = DetectionKind::Aimbot;
    Confidence Level = Confidence::Certain;
    bool KickOnly = false;
    float Suspicion = 0.0f;
    std::string Evidence;
};

}  // namespace Anticheat

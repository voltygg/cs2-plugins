#pragma once

#include "Detect/Finding.hpp"
#include "Detect/Suspicion.hpp"

#include <string_view>

namespace Anticheat::Rules
{

/** A subscription no stock client makes is a confirmed fact, so it weighs a whole unit and stays
 *  true while they are connected. Scanning for it is an engine read, so that half lives in
 *  Engine/. @p reason must outlive the call. */
constexpr Contribution DllInjectionEvidence(std::string_view reason)
{
    return {
        .Kind = DetectionKind::DllInjection,
        .Points = 1.0f,
        .HalfLifeSec = FadesOverTheSession,
        .Reason = reason,
    };
}

}  // namespace Anticheat::Rules

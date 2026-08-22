#pragma once

#include <cstdint>
#include <string_view>

namespace Contracts
{

/** Outcome of an automated ban request. No exceptions cross a module boundary. */
enum class BanResult : int
{
    Ok = 0,
    /** The SteamID did not parse as a valid 64-bit community ID. */
    InvalidSteamId,
    /** The ban was rejected or could not be written to the database. */
    PersistFailed,
    /** No plugin is offering IAdminActions - admin-system is not loaded. */
    Unavailable,
};

/**
 * @brief Admin-system actions offered to other plugins through CS2Kit's ServiceExchange.
 *
 * Published by admin-system in OnLoad; consumed by anticheat. Replaces the former
 * `as_ac_ban` / `as_ac_alert` console-command bridge, where every argument was formatted
 * into a command string and re-parsed - a quoting bug there was a command-injection bug,
 * and the caller learned nothing about whether the ban landed.
 *
 * Publisher and consumer are separate modules with separate `operator new` (cs2_add_plugin
 * compiles memoverride.cpp per plugin), so nothing here allocates on one side and frees on
 * the other: parameters are views the callee must not retain, returns are trivially copyable.
 * Any change to this vtable or to a parameter's meaning bumps the /N in InterfaceName.
 */
struct IAdminActions
{
    static constexpr const char* InterfaceName = "cs2plugins.IAdminActions/1";

    /**
     * Ban @p steamId for @p durationSec seconds (0 = permanent), attributed to the automated
     * system rather than to any admin, so it does not count against abuse-rate stats.
     * @p reason is copied before this returns.
     */
    virtual BanResult Ban(int64_t steamId, int64_t durationSec, std::string_view reason) = 0;

    /** Notify every online admin holding the ban permission about a detection. */
    virtual void AlertAdmins(int64_t steamId, std::string_view detector, int score) = 0;

protected:
    // Non-virtual and protected: consumers borrow, they never own or delete.
    ~IAdminActions() = default;
};

}  // namespace Contracts

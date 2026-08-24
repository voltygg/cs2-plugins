#pragma once

#include <cstdint>
#include <string_view>

namespace Contracts
{

/** Outcome of an automated ban. A return value, because no exception may cross modules. */
enum class BanResult : int
{
    Ok = 0,
    InvalidSteamId,
    /** Rejected, or the write to the database failed. */
    PersistFailed,
};

/**
 * @brief Admin-system actions offered to other plugins through VoltMod's ServiceExchange.
 *
 * Published by admin-system in OnLoad, consumed by anticheat. Callers get nullptr from the
 * exchange when admin-system is not loaded.
 *
 * Each plugin has its own operator new, so nothing crosses that owns memory: parameters are
 * views the callee must not retain, returns are trivially copyable. Any change to this
 * vtable or to what a parameter means bumps the /N in InterfaceName.
 */
struct IAdminActions
{
    static constexpr const char* InterfaceName = "cs2plugins.IAdminActions/1";

    /** Ban @p steamId for @p durationSec seconds, 0 being permanent. @p reason is copied
     *  before this returns. */
    virtual BanResult Ban(int64_t steamId, int64_t durationSec, std::string_view reason) = 0;

    /** Notify every online admin holding the ban permission. */
    virtual void AlertAdmins(int64_t steamId, std::string_view detector, int score) = 0;

protected:
    // Consumers borrow; they never own or delete.
    ~IAdminActions() = default;
};

}  // namespace Contracts

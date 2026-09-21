#pragma once

#include <cstdint>
#include <string_view>

namespace Contracts
{

/**
 * @brief admin-system's permission check, offered to other plugins through VoltMod's ServiceExchange.
 *
 * A consumer installs it as its own `Runtime::Policy.HasPermission`, asking the exchange on every
 * call: admin-system may load later or unload between calls. Nullptr from the exchange means
 * nobody holds anything.
 *
 * Any change to this vtable or to what a parameter means bumps the /N in InterfaceName.
 */
struct IPermissions
{
    static constexpr std::string_view InterfaceName = "cs2plugins.IPermissions/1";

    /** Whether @p steamId holds @p permission, through a group, a wildcard or root. */
    virtual bool HasPermission(int64_t steamId, std::string_view permission) = 0;

protected:
    // Consumers borrow; they never own or delete.
    ~IPermissions() = default;
};

}  // namespace Contracts

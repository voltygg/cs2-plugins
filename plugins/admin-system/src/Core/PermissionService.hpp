#pragma once

#include "Admin/Access.hpp"

#include <Contracts/IPermissions.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Core
{

/** admin-system's permission gate, published so other plugins can back their own policy with it. */
class PermissionService final : public Contracts::IPermissions
{
public:
    PermissionService(VoltMod::Runtime& runtime, Admin::Access& access) : _rt(runtime), _access(access) {}

    void Publish() { _rt.Exchange.Publish<Contracts::IPermissions>(this); }
    /** Called before the access gate this delegates to is destroyed. */
    void Unpublish() { _rt.Exchange.Unpublish<Contracts::IPermissions>(); }

    bool HasPermission(int64_t steamId, std::string_view permission) override
    {
        return _access.HasPermission(steamId, permission);
    }

private:
    VoltMod::Runtime& _rt;
    Admin::Access& _access;
};

}  // namespace AdminSystem::Core

#pragma once

#include "Admin/Access.hpp"

#include <VoltMod/Core/Signals/Subscription.hpp>
#include <VoltMod/Players/Permissions.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Core
{

/** admin-system's permission gate, published as the server's permission source: every plugin's runtime asks it. */
class PermissionService final : public VoltMod::IPermissions
{
public:
    PermissionService(VoltMod::Runtime& runtime, Admin::Access& access) : _rt(runtime), _access(access) {}

    /** Offer this to other plugins until this is destroyed. */
    void Publish() { _published = _rt.Exchange.Publish<VoltMod::IPermissions>(this); }

    bool HasPermission(int64_t steamId, std::string_view permission) override
    {
        return _access.HasPermission(steamId, permission);
    }

private:
    VoltMod::Runtime& _rt;
    Admin::Access& _access;
    /** Declared last, so the entry is withdrawn before anything it reaches. */
    VoltMod::Subscription _published;
};

}  // namespace AdminSystem::Core

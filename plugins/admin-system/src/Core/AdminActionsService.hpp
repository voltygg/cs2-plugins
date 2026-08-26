#pragma once

#include "../Admin/Access.hpp"
#include "../Punishments/PunishmentManager.hpp"

#include <Contracts/IAdminActions.hpp>
#include <VoltMod/Runtime.hpp>

namespace AdminSystem::Core
{

/**
 * admin-system's side of the cross-plugin admin surface, published into the runtime's
 * ServiceExchange in OnLoad.
 *
 * Bans land with AdminSteamId=0 / AdminName="AntiCheat": there is no admin to attribute,
 * and automated bans must not count against anyone's abuse-rate stats.
 */
class AdminActionsService final : public Contracts::IAdminActions
{
public:
    AdminActionsService(VoltMod::Runtime& runtime, Punishments::PunishmentManager& punishments, Admin::Access& access)
        : _rt(runtime), _punishments(punishments), _access(access)
    {}

    void Publish();
    /** Called before the managers this delegates to are destroyed. */
    void Unpublish();

    Contracts::BanResult Ban(int64_t steamId, int64_t durationSec, std::string_view reason) override;
    void AlertAdmins(int64_t steamId, std::string_view detector, int score) override;

private:
    VoltMod::Runtime& _rt;
    Punishments::PunishmentManager& _punishments;
    Admin::Access& _access;
};

}  // namespace AdminSystem::Core

#pragma once

#include <Contracts/IAdminActions.hpp>

namespace AdminSystem::Core
{

/**
 * admin-system's implementation of the cross-plugin admin surface, published into
 * `Engine().Exchange` during OnLoad so a separate module (anticheat) can ban and alert
 * through a typed call instead of a formatted console command.
 *
 * Bans land with AdminSteamId=0 / AdminName="AntiCheat": there is no admin Player to
 * attribute, and automated bans must not count against any admin's abuse-rate stats.
 */
class AdminActionsService final : public Contracts::IAdminActions
{
public:
    /** Offer this instance to other plugins under IAdminActions::InterfaceName. */
    void Publish();
    /** Withdraw the offer. Called before the managers this delegates to are destroyed. */
    void Unpublish();

    Contracts::BanResult Ban(int64_t steamId, int64_t durationSec, std::string_view reason) override;
    void AlertAdmins(int64_t steamId, std::string_view detector, int score) override;
};

}  // namespace AdminSystem::Core

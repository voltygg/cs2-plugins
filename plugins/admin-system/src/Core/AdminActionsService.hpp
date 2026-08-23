#pragma once

#include <Contracts/IAdminActions.hpp>

namespace AdminSystem
{
struct App;
}

namespace AdminSystem::Core
{

/**
 * admin-system's side of the cross-plugin admin surface, published into _app.Runtime.Exchange
 * in OnLoad.
 *
 * Bans land with AdminSteamId=0 / AdminName="AntiCheat": there is no admin to attribute,
 * and automated bans must not count against anyone's abuse-rate stats.
 */
class AdminActionsService final : public Contracts::IAdminActions
{
public:
    explicit AdminActionsService(AdminSystem::App& app) : _app(app) {}

    void Publish();
    /** Called before the managers this delegates to are destroyed. */
    void Unpublish();

    Contracts::BanResult Ban(int64_t steamId, int64_t durationSec, std::string_view reason) override;
    void AlertAdmins(int64_t steamId, std::string_view detector, int score) override;

private:
    AdminSystem::App& _app;
};

}  // namespace AdminSystem::Core

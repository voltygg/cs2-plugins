#include "AdminMenus.hpp"
#include "AdminManager.hpp"
#include "../Menu/MenuBuilder.hpp"
#include "../Menu/MenuManager.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Utils/Translations.hpp"

#include <ISmmPlugin.h>
#include <format>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Admin {

//-----------------------------------------------------------------------------
// Helper: Build a timed-punishment duration submenu (shared by Ban/Mute/Gag)
//-----------------------------------------------------------------------------

static std::shared_ptr<Menu::Menu> BuildTimedPunishmentMenu(
    int adminSlot, int targetSlot,
    const std::string& actionName,
    std::function<void(int slot, int target, int duration)> onDuration)
{
    auto& tr = Utils::Translations::Instance();

    struct DurationEntry
    {
        std::string Key;
        int Seconds;
    };

    static const DurationEntry Durations[] = {
        {"duration5min",   300},
        {"duration30min",  1800},
        {"duration1h",     3600},
        {"duration1d",     86400},
        {"duration7d",     604800},
        {"durationPerm",   0},
    };

    Menu::MenuBuilder builder(std::format("{}: {}", actionName, tr.Get("selectDuration")));

    for (const auto& dur : Durations)
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddItem(tr.Get(dur.Key),
            [callback, target, secs = dur.Seconds](int slot) {
                callback(slot, target, secs);
                Menu::MenuManager::Instance().CloseAllMenus(slot);
            });
    }

    return builder.Build();
}

//-----------------------------------------------------------------------------
// Helper: Create a punishment callback for Ban/Mute/Gag to reduce duplication
//-----------------------------------------------------------------------------

template<typename PunishmentT, typename IssueFunc>
static std::function<void(int, int, int)> MakePunishmentCallback(IssueFunc issueFunc)
{
    return [issueFunc](int adminSlot, int target, int duration) {
        auto& plrMgr = Players::PlayerManager::Instance();
        auto* a = plrMgr.GetPlayerBySlot(adminSlot);
        auto* t = plrMgr.GetPlayerBySlot(target);
        if (a && t)
        {
            PunishmentT punishment;
            punishment.TargetSteamId = t->GetSteamID();
            punishment.TargetName = t->GetName();
            punishment.AdminSteamId = a->GetSteamID();
            punishment.AdminName = a->GetName();
            punishment.Duration = duration;
            punishment.Reason = "Admin menu";

            auto& pm = Punishments::PunishmentManager::Instance();
            issueFunc(pm, punishment);
        }
    };
}

//-----------------------------------------------------------------------------
// Duration selection submenu (public API)
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildDurationMenu(int adminSlot, int targetSlot,
                                                const std::string& actionName,
                                                std::function<void(int, int, int)> onDuration)
{
    return BuildTimedPunishmentMenu(adminSlot, targetSlot, actionName, std::move(onDuration));
}

//-----------------------------------------------------------------------------
// Player actions submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildPlayerActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Utils::Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = Players::PlayerManager::Instance();

    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();

    bool canTarget = adminMgr.CanTarget(adminSid, targetSid);

    Menu::MenuBuilder builder(std::format("{}: {}", tr.Get("playerActions"), target->GetName()));

    // Kick
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'c') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionKick"),
            [tgt](int slot) {
                auto& plrMgr = Players::PlayerManager::Instance();
                auto* a = plrMgr.GetPlayerBySlot(slot);
                auto* t = plrMgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    META_CONPRINTF("[AdminSystem] %s kicked %s\n",
                                   a->GetName().c_str(), t->GetName().c_str());
                }
                Menu::MenuManager::Instance().CloseAllMenus(slot);
            },
            hasPerm);
    }

    // Ban (opens duration submenu)
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'b') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionBan"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(slot, tgt,
                    Utils::Translations::Instance().Get("actionBan"),
                    MakePunishmentCallback<Database::Ban>(
                        [](Punishments::PunishmentManager& pm, const Database::Ban& ban) {
                            pm.IssueBan(ban);
                        }));
                Menu::MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    // Mute (opens duration submenu)
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'i') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionMute"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(slot, tgt,
                    Utils::Translations::Instance().Get("actionMute"),
                    MakePunishmentCallback<Database::Mute>(
                        [](Punishments::PunishmentManager& pm, const Database::Mute& mute) {
                            pm.IssueMute(mute);
                        }));
                Menu::MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    // Gag (opens duration submenu)
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'i') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionGag"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(slot, tgt,
                    Utils::Translations::Instance().Get("actionGag"),
                    MakePunishmentCallback<Database::Gag>(
                        [](Punishments::PunishmentManager& pm, const Database::Gag& gag) {
                            pm.IssueGag(gag);
                        }));
                Menu::MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    // Warn
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'i') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionWarn"),
            [tgt](int slot) {
                auto& plrMgr = Players::PlayerManager::Instance();
                auto* a = plrMgr.GetPlayerBySlot(slot);
                auto* t = plrMgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    Database::Warning warn;
                    warn.TargetSteamId = t->GetSteamID();
                    warn.TargetName = t->GetName();
                    warn.AdminSteamId = a->GetSteamID();
                    warn.AdminName = a->GetName();
                    warn.Reason = "Admin menu";

                    Punishments::PunishmentManager::Instance().IssueWarning(warn);
                }
                Menu::MenuManager::Instance().CloseAllMenus(slot);
            },
            hasPerm);
    }

    return builder.Build();
}

//-----------------------------------------------------------------------------
// Player list submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildPlayerListMenu(int adminSlot)
{
    auto& tr = Utils::Translations::Instance();
    auto& plrMgr = Players::PlayerManager::Instance();

    Menu::MenuBuilder builder(tr.Get("playerManagement"));

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        int targetSlot = p->GetSlot();
        int admin = adminSlot;
        builder.AddItem(p->GetName(),
            [admin, targetSlot](int slot) {
                auto actions = BuildPlayerActionsMenu(admin, targetSlot);
                if (actions)
                    Menu::MenuManager::Instance().OpenMenu(slot, actions);
            });
    }

    if (players.empty())
    {
        builder.AddItem(tr.Get("noPlayers"), [](int) {}, false);
    }

    return builder.Build();
}

//-----------------------------------------------------------------------------
// Server management submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildServerMenu(int adminSlot)
{
    auto& tr = Utils::Translations::Instance();

    Menu::MenuBuilder builder(tr.Get("serverManagement"));

    // TODO: Implement server management actions
    builder.AddItem(tr.Get("actionChangemap"),
        [](int slot) {
            META_CONPRINTF("[AdminSystem] Map change requested by slot %d\n", slot);
            Menu::MenuManager::Instance().CloseAllMenus(slot);
        },
        false); // Disabled until implemented

    builder.AddItem(tr.Get("actionRestartRound"),
        [](int slot) {
            META_CONPRINTF("[AdminSystem] Round restart requested by slot %d\n", slot);
            Menu::MenuManager::Instance().CloseAllMenus(slot);
        },
        false);

    return builder.Build();
}

//-----------------------------------------------------------------------------
// Main admin menu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildAdminMainMenu(int adminSlot)
{
    auto& tr = Utils::Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = Players::PlayerManager::Instance();

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    Menu::MenuBuilder builder(tr.Get("adminPanel"));

    // Player Management
    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "bcdi");
        int slot = adminSlot;
        builder.AddItem(tr.Get("playerManagement"),
            [slot](int s) {
                auto playerList = BuildPlayerListMenu(slot);
                Menu::MenuManager::Instance().OpenMenu(s, playerList);
            },
            hasPerm);
    }

    // Server Management
    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "rz");
        int slot = adminSlot;
        builder.AddItem(tr.Get("serverManagement"),
            [slot](int s) {
                auto serverMenu = BuildServerMenu(slot);
                Menu::MenuManager::Instance().OpenMenu(s, serverMenu);
            },
            hasPerm);
    }

    return builder.Build();
}

} // namespace AdminSystem::Admin

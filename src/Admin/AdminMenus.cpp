#include "AdminMenus.hpp"
#include "AdminManager.hpp"
#include "../Menu/MenuBuilder.hpp"
#include "../Menu/MenuManager.hpp"
#include "../Players/PlayerManager.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Utils/Log.hpp"
#include "../Utils/Translations.hpp"

#include <ISmmPlugin.h>
#include <format>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace AdminSystem::Admin {

using namespace AdminSystem::Database;
using namespace AdminSystem::Players;
using namespace AdminSystem::Punishments;
using namespace AdminSystem::Utils;

// Import specific Menu types (not the whole namespace — it conflicts with the Menu struct)
using AdminSystem::Menu::MenuBuilder;
using AdminSystem::Menu::MenuManager;

//-----------------------------------------------------------------------------
// Helper: Build a timed-punishment duration submenu (shared by Ban/Mute/Gag)
//-----------------------------------------------------------------------------

static std::shared_ptr<Menu::Menu> BuildTimedPunishmentMenu(
    int adminSlot, int targetSlot,
    const std::string& actionName,
    std::function<void(int slot, int target, int duration)> onDuration)
{
    auto& tr = Translations::Instance();

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

    MenuBuilder builder(std::format("{}: {}", actionName, tr.Get("selectDuration")));

    for (const auto& dur : Durations)
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddItem(tr.Get(dur.Key),
            [callback, target, secs = dur.Seconds](int slot) {
                callback(slot, target, secs);
                MenuManager::Instance().CloseAllMenus(slot);
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
        auto& plrMgr = PlayerManager::Instance();
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

            auto& pm = PunishmentManager::Instance();
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
    auto& tr = Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = PlayerManager::Instance();

    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();

    bool canTarget = adminMgr.CanTarget(adminSid, targetSid);

    MenuBuilder builder(std::format("{}: {}", tr.Get("playerActions"), target->GetName()));

    // Kick
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'c') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionKick"),
            [tgt](int slot) {
                auto& plrMgr = PlayerManager::Instance();
                auto* a = plrMgr.GetPlayerBySlot(slot);
                auto* t = plrMgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    Log::Info("{} kicked {}", a->GetName(), t->GetName());
                }
                MenuManager::Instance().CloseAllMenus(slot);
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
                    Translations::Instance().Get("actionBan"),
                    MakePunishmentCallback<Ban>(
                        [](PunishmentManager& pm, const Ban& ban) {
                            pm.IssueBan(ban);
                        }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
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
                    Translations::Instance().Get("actionMute"),
                    MakePunishmentCallback<Mute>(
                        [](PunishmentManager& pm, const Mute& mute) {
                            pm.IssueMute(mute);
                        }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
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
                    Translations::Instance().Get("actionGag"),
                    MakePunishmentCallback<Gag>(
                        [](PunishmentManager& pm, const Gag& gag) {
                            pm.IssueGag(gag);
                        }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    // Warn
    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'i') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(tr.Get("actionWarn"),
            [tgt](int slot) {
                auto& plrMgr = PlayerManager::Instance();
                auto* a = plrMgr.GetPlayerBySlot(slot);
                auto* t = plrMgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    Warning warn;
                    warn.TargetSteamId = t->GetSteamID();
                    warn.TargetName = t->GetName();
                    warn.AdminSteamId = a->GetSteamID();
                    warn.AdminName = a->GetName();
                    warn.Reason = "Admin menu";

                    PunishmentManager::Instance().IssueWarning(warn);
                }
                MenuManager::Instance().CloseAllMenus(slot);
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
    auto& tr = Translations::Instance();
    auto& plrMgr = PlayerManager::Instance();

    MenuBuilder builder(tr.Get("playerManagement"));

    auto players = plrMgr.GetAllPlayers();
    for (auto* p : players)
    {
        int targetSlot = p->GetSlot();
        int admin = adminSlot;
        builder.AddItem(p->GetName(),
            [admin, targetSlot](int slot) {
                auto actions = BuildPlayerActionsMenu(admin, targetSlot);
                if (actions)
                    MenuManager::Instance().OpenMenu(slot, actions);
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
    auto& tr = Translations::Instance();

    MenuBuilder builder(tr.Get("serverManagement"));

    // TODO: Implement server management actions
    builder.AddItem(tr.Get("actionChangemap"),
        [](int slot) {
            Log::Info("Map change requested by slot {}", slot);
            MenuManager::Instance().CloseAllMenus(slot);
        },
        false); // Disabled until implemented

    builder.AddItem(tr.Get("actionRestartRound"),
        [](int slot) {
            Log::Info("Round restart requested by slot {}", slot);
            MenuManager::Instance().CloseAllMenus(slot);
        },
        false);

    return builder.Build();
}

//-----------------------------------------------------------------------------
// Main admin menu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu::Menu> BuildAdminMainMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    auto& adminMgr = AdminManager::Instance();
    auto& plrMgr = PlayerManager::Instance();

    auto* adminPlayer = plrMgr.GetPlayerBySlot(adminSlot);
    if (!adminPlayer)
        return nullptr;

    int64_t adminSid = adminPlayer->GetSteamID();

    MenuBuilder builder(tr.Get("adminPanel"));

    // Player Management
    {
        bool hasPerm = adminMgr.HasAnyPermission(adminSid, "bcdi");
        int slot = adminSlot;
        builder.AddItem(tr.Get("playerManagement"),
            [slot](int s) {
                auto playerList = BuildPlayerListMenu(slot);
                MenuManager::Instance().OpenMenu(s, playerList);
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
                MenuManager::Instance().OpenMenu(s, serverMenu);
            },
            hasPerm);
    }

    return builder.Build();
}

} // namespace AdminSystem::Admin

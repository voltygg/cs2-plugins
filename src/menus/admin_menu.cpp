#include "admin_menu.h"
#include "menu_manager.h"
#include "../admin/admin_manager.h"
#include "../player/player_manager.h"
#include "../punishments/punishment_manager.h"
#include "../utils/translations.h"

#include <ISmmPlugin.h>
#include <format>

extern ISmmAPI* g_SMAPI;
extern SourceMM::ISmmPlugin* g_PLAPI;

namespace menus {

//-----------------------------------------------------------------------------
// Duration selection submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu> BuildDurationMenu(int admin_slot, int target_slot,
                                         const std::string& action_name,
                                         std::function<void(int slot, int target, int duration)> on_duration)
{
    auto& tr = utils::Translations::Instance();
    auto menu = std::make_shared<Menu>();
    menu->title = std::format("{}: {}", action_name, tr.Get("select_duration"));

    struct DurationEntry
    {
        std::string key;
        int seconds;
    };

    static const DurationEntry durations[] = {
        {"duration_5min",   300},
        {"duration_30min",  1800},
        {"duration_1h",     3600},
        {"duration_1d",     86400},
        {"duration_7d",     604800},
        {"duration_perm",   0},
    };

    for (const auto& dur : durations)
    {
        int target = target_slot;
        auto callback = on_duration;
        menu->items.push_back({
            .title = tr.Get(dur.key),
            .on_select = [callback, target, secs = dur.seconds](int slot) {
                callback(slot, target, secs);
                // Close all menus after executing
                MenuManager::Instance().CloseAllMenus(slot);
            },
        });
    }

    return menu;
}

//-----------------------------------------------------------------------------
// Player actions submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu> BuildPlayerActionsMenu(int admin_slot, int target_slot)
{
    auto& tr = utils::Translations::Instance();
    auto& admin_mgr = admin::AdminManager::Instance();
    auto& player_mgr = player::PlayerManager::Instance();

    auto* target = player_mgr.GetPlayerBySlot(target_slot);
    if (!target)
        return nullptr;

    auto* admin = player_mgr.GetPlayerBySlot(admin_slot);
    if (!admin)
        return nullptr;

    int64_t admin_sid = admin->GetSteamID();
    int64_t target_sid = target->GetSteamID();

    auto menu = std::make_shared<Menu>();
    menu->title = std::format("{}: {}", tr.Get("player_actions"), target->GetName());

    bool can_target = admin_mgr.CanTarget(admin_sid, target_sid);

    // Kick
    {
        bool has_perm = admin_mgr.HasPermission(admin_sid, 'c') && can_target;
        int tgt = target_slot;
        menu->items.push_back({
            .title = tr.Get("action_kick"),
            .on_select = [tgt](int slot) {
                auto& pm = punishments::PunishmentManager::Instance();
                auto& plr_mgr = player::PlayerManager::Instance();
                auto* a = plr_mgr.GetPlayerBySlot(slot);
                auto* t = plr_mgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    // TODO: Implement actual kick via engine
                    META_CONPRINTF("[AdminSystem] %s kicked %s\n",
                                   a->GetName().c_str(), t->GetName().c_str());
                }
                MenuManager::Instance().CloseAllMenus(slot);
            },
            .enabled = has_perm,
        });
    }

    // Ban (opens duration submenu)
    {
        bool has_perm = admin_mgr.HasPermission(admin_sid, 'b') && can_target;
        int tgt = target_slot;
        menu->items.push_back({
            .title = tr.Get("action_ban"),
            .on_select = [tgt](int slot) {
                auto dur_menu = BuildDurationMenu(slot, tgt, utils::Translations::Instance().Get("action_ban"),
                    [](int admin_slot, int target, int duration) {
                        auto& pm = punishments::PunishmentManager::Instance();
                        auto& plr_mgr = player::PlayerManager::Instance();
                        auto* a = plr_mgr.GetPlayerBySlot(admin_slot);
                        auto* t = plr_mgr.GetPlayerBySlot(target);
                        if (a && t)
                        {
                            database::Ban ban;
                            ban.target_steam_id = t->GetSteamID();
                            ban.target_name = t->GetName();
                            ban.admin_steam_id = a->GetSteamID();
                            ban.admin_name = a->GetName();
                            ban.duration = duration;
                            ban.reason = "Admin menu";
                            pm.IssueBan(ban);
                        }
                    });
                MenuManager::Instance().OpenMenu(slot, dur_menu);
            },
            .enabled = has_perm,
        });
    }

    // Mute (opens duration submenu)
    {
        bool has_perm = admin_mgr.HasPermission(admin_sid, 'i') && can_target;
        int tgt = target_slot;
        menu->items.push_back({
            .title = tr.Get("action_mute"),
            .on_select = [tgt](int slot) {
                auto dur_menu = BuildDurationMenu(slot, tgt, utils::Translations::Instance().Get("action_mute"),
                    [](int admin_slot, int target, int duration) {
                        auto& pm = punishments::PunishmentManager::Instance();
                        auto& plr_mgr = player::PlayerManager::Instance();
                        auto* a = plr_mgr.GetPlayerBySlot(admin_slot);
                        auto* t = plr_mgr.GetPlayerBySlot(target);
                        if (a && t)
                        {
                            database::Mute mute;
                            mute.target_steam_id = t->GetSteamID();
                            mute.target_name = t->GetName();
                            mute.admin_steam_id = a->GetSteamID();
                            mute.admin_name = a->GetName();
                            mute.duration = duration;
                            mute.reason = "Admin menu";
                            pm.IssueMute(mute);
                        }
                    });
                MenuManager::Instance().OpenMenu(slot, dur_menu);
            },
            .enabled = has_perm,
        });
    }

    // Gag (opens duration submenu)
    {
        bool has_perm = admin_mgr.HasPermission(admin_sid, 'i') && can_target;
        int tgt = target_slot;
        menu->items.push_back({
            .title = tr.Get("action_gag"),
            .on_select = [tgt](int slot) {
                auto dur_menu = BuildDurationMenu(slot, tgt, utils::Translations::Instance().Get("action_gag"),
                    [](int admin_slot, int target, int duration) {
                        auto& pm = punishments::PunishmentManager::Instance();
                        auto& plr_mgr = player::PlayerManager::Instance();
                        auto* a = plr_mgr.GetPlayerBySlot(admin_slot);
                        auto* t = plr_mgr.GetPlayerBySlot(target);
                        if (a && t)
                        {
                            database::Gag gag;
                            gag.target_steam_id = t->GetSteamID();
                            gag.target_name = t->GetName();
                            gag.admin_steam_id = a->GetSteamID();
                            gag.admin_name = a->GetName();
                            gag.duration = duration;
                            gag.reason = "Admin menu";
                            pm.IssueGag(gag);
                        }
                    });
                MenuManager::Instance().OpenMenu(slot, dur_menu);
            },
            .enabled = has_perm,
        });
    }

    // Warn
    {
        bool has_perm = admin_mgr.HasPermission(admin_sid, 'i') && can_target;
        int tgt = target_slot;
        menu->items.push_back({
            .title = tr.Get("action_warn"),
            .on_select = [tgt](int slot) {
                auto& pm = punishments::PunishmentManager::Instance();
                auto& plr_mgr = player::PlayerManager::Instance();
                auto* a = plr_mgr.GetPlayerBySlot(slot);
                auto* t = plr_mgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    database::Warning warn;
                    warn.target_steam_id = t->GetSteamID();
                    warn.target_name = t->GetName();
                    warn.admin_steam_id = a->GetSteamID();
                    warn.admin_name = a->GetName();
                    warn.reason = "Admin menu";
                    pm.IssueWarning(warn);
                }
                MenuManager::Instance().CloseAllMenus(slot);
            },
            .enabled = has_perm,
        });
    }

    return menu;
}

//-----------------------------------------------------------------------------
// Player list submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu> BuildPlayerListMenu(int admin_slot)
{
    auto& tr = utils::Translations::Instance();
    auto& player_mgr = player::PlayerManager::Instance();

    auto menu = std::make_shared<Menu>();
    menu->title = tr.Get("player_management");

    auto players = player_mgr.GetAllPlayers();
    for (auto* p : players)
    {
        int target_slot = p->GetSlot();
        int admin = admin_slot;
        menu->items.push_back({
            .title = p->GetName(),
            .on_select = [admin, target_slot](int slot) {
                auto actions = BuildPlayerActionsMenu(admin, target_slot);
                if (actions)
                    MenuManager::Instance().OpenMenu(slot, actions);
            },
        });
    }

    if (menu->items.empty())
    {
        menu->items.push_back({
            .title = tr.Get("no_players"),
            .enabled = false,
        });
    }

    return menu;
}

//-----------------------------------------------------------------------------
// Server management submenu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu> BuildServerMenu(int admin_slot)
{
    auto& tr = utils::Translations::Instance();

    auto menu = std::make_shared<Menu>();
    menu->title = tr.Get("server_management");

    // TODO: Implement server management actions
    menu->items.push_back({
        .title = tr.Get("action_changemap"),
        .on_select = [](int slot) {
            // TODO: Open map selection submenu
            META_CONPRINTF("[AdminSystem] Map change requested by slot %d\n", slot);
            MenuManager::Instance().CloseAllMenus(slot);
        },
        .enabled = false, // Disabled until implemented
    });

    menu->items.push_back({
        .title = tr.Get("action_restart_round"),
        .on_select = [](int slot) {
            // TODO: Restart round via server command
            META_CONPRINTF("[AdminSystem] Round restart requested by slot %d\n", slot);
            MenuManager::Instance().CloseAllMenus(slot);
        },
        .enabled = false,
    });

    return menu;
}

//-----------------------------------------------------------------------------
// Main admin menu
//-----------------------------------------------------------------------------

std::shared_ptr<Menu> BuildAdminMainMenu(int admin_slot)
{
    auto& tr = utils::Translations::Instance();
    auto& admin_mgr = admin::AdminManager::Instance();
    auto& player_mgr = player::PlayerManager::Instance();

    auto* admin_player = player_mgr.GetPlayerBySlot(admin_slot);
    if (!admin_player)
        return nullptr;

    int64_t admin_sid = admin_player->GetSteamID();

    auto menu = std::make_shared<Menu>();
    menu->title = tr.Get("admin_panel");

    // Player Management
    {
        bool has_perm = admin_mgr.HasAnyPermission(admin_sid, "bcdi");
        int slot = admin_slot;
        menu->items.push_back({
            .title = tr.Get("player_management"),
            .on_select = [slot](int s) {
                auto player_list = BuildPlayerListMenu(slot);
                MenuManager::Instance().OpenMenu(s, player_list);
            },
            .enabled = has_perm,
        });
    }

    // Server Management
    {
        bool has_perm = admin_mgr.HasAnyPermission(admin_sid, "rz");
        int slot = admin_slot;
        menu->items.push_back({
            .title = tr.Get("server_management"),
            .on_select = [slot](int s) {
                auto server_menu = BuildServerMenu(slot);
                MenuManager::Instance().OpenMenu(s, server_menu);
            },
            .enabled = has_perm,
        });
    }

    return menu;
}

} // namespace menus

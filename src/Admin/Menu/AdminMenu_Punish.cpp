#include "AdminMenu_Punish.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Punishments/PunishmentManager.hpp"
#include "../AdminManager.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Database;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;

static std::shared_ptr<::CS2Kit::Menu::Menu> BuildTimedPunishmentMenu(
    int adminSlot, int targetSlot, const std::string& actionName,
    std::function<void(int slot, int target, int duration)> onDuration)
{
    auto& tr = Translations::Instance();

    struct DurationEntry
    {
        std::string Key;
        int Seconds;
    };

    static const DurationEntry Durations[] = {
        {"duration5min", 300}, {"duration30min", 1800}, {"duration1h", 3600},
        {"duration1d", 86400}, {"duration7d", 604800},  {"durationPerm", 0},
    };

    MenuBuilder builder(std::format("{}: {}", actionName, tr.Get("selectDuration")));

    for (const auto& dur : Durations)
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddItem(tr.Get(dur.Key), [callback, target, secs = dur.Seconds](int slot) {
            callback(slot, target, secs);
            MenuManager::Instance().CloseAllMenus(slot);
        });
    }

    return builder.Build();
}

template <typename PunishmentT, typename IssueFunc>
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

std::shared_ptr<::CS2Kit::Menu::Menu> BuildDurationMenu(int adminSlot, int targetSlot, const std::string& actionName,
                                                        std::function<void(int, int, int)> onDuration)
{
    return BuildTimedPunishmentMenu(adminSlot, targetSlot, actionName, std::move(onDuration));
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishMenu(int adminSlot)
{
    auto& tr = Translations::Instance();
    return BuildPlayerPicker(adminSlot, tr.Get("categoryPunish"), [](int admin, int target) {
        auto actions = BuildPunishActionsMenu(admin, target);
        if (actions)
            MenuManager::Instance().OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishActionsMenu(int adminSlot, int targetSlot)
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

    MenuBuilder builder(std::format("{}: {}", tr.Get("categoryPunish"), target->GetName()));

    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'c') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(
            tr.Get("actionKick"),
            [tgt](int slot) {
                auto& plrMgr = PlayerManager::Instance();
                auto* a = plrMgr.GetPlayerBySlot(slot);
                auto* t = plrMgr.GetPlayerBySlot(tgt);
                if (a && t)
                {
                    const char* reason = "Kicked via admin menu";
                    CS2Kit::Sdk::PlayerController(t->GetSlot()).Kick(reason);
                    AdminSystem::Core::ChatService::Instance().BroadcastPunishment("kicked", a->GetName(),
                                                                                   t->GetName(), reason, 0);
                }
                MenuManager::Instance().CloseAllMenus(slot);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'd') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(
            tr.Get("actionBan"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(
                    slot, tgt, Translations::Instance().Get("actionBan"),
                    MakePunishmentCallback<Ban>([](PunishmentManager& pm, Ban& ban) { pm.IssueBan(ban); }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'o') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(
            tr.Get("actionMute"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(
                    slot, tgt, Translations::Instance().Get("actionMute"),
                    MakePunishmentCallback<Mute>([](PunishmentManager& pm, Mute& mute) { pm.IssueMute(mute); }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'p') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(
            tr.Get("actionGag"),
            [tgt](int slot) {
                auto durMenu = BuildTimedPunishmentMenu(
                    slot, tgt, Translations::Instance().Get("actionGag"),
                    MakePunishmentCallback<Gag>([](PunishmentManager& pm, Gag& gag) { pm.IssueGag(gag); }));
                MenuManager::Instance().OpenMenu(slot, durMenu);
            },
            hasPerm);
    }

    {
        bool hasPerm = adminMgr.HasPermission(adminSid, 'q') && canTarget;
        int tgt = targetSlot;
        builder.AddItem(
            tr.Get("actionWarn"),
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

}  // namespace AdminSystem::Admin::Menu

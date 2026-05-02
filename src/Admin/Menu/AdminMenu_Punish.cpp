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

#include <cctype>
#include <charconv>
#include <format>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Database;
using namespace AdminSystem::Punishments;
using namespace CS2Kit::Players;
using namespace CS2Kit::Utils;

using CS2Kit::Menu::MenuBuilder;
using CS2Kit::Menu::MenuManager;

namespace
{

// Parse "30s", "5m", "2h", "7d" or a bare integer (interpreted as seconds).
// Returns -1 on parse failure, 0 for a permanent ban request, otherwise the duration in seconds.
int ParseDuration(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
        text.remove_prefix(1);
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())))
        text.remove_suffix(1);

    if (text.empty())
        return -1;
    if (text == "0" || text == "perm" || text == "permanent")
        return 0;

    int multiplier = 1;
    char suffix = text.back();
    if (!std::isdigit(static_cast<unsigned char>(suffix)))
    {
        switch (suffix)
        {
            case 's': multiplier = 1; break;
            case 'm': multiplier = 60; break;
            case 'h': multiplier = 3600; break;
            case 'd': multiplier = 86400; break;
            default: return -1;
        }
        text.remove_suffix(1);
    }

    int value = 0;
    auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size() || value < 0)
        return -1;

    return value * multiplier;
}

}  // namespace

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
        builder.AddButton(tr.Get(dur.Key), [callback, target, secs = dur.Seconds](int slot) {
            callback(slot, target, secs);
            MenuManager::Instance().CloseAllMenus(slot);
        });
    }

    // Custom duration row — opens a chat-input prompt; accepts "5m"/"2h"/"7d" or seconds.
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddInput(
            tr.Get("durationCustom"), tr.Get("durationCustomPrompt"), [](int) { return std::string{}; },
            [callback, target](int slot, std::string_view text) -> bool {
                int seconds = ParseDuration(text);
                if (seconds < 0)
                    return false;  // re-prompt
                callback(slot, target, seconds);
                MenuManager::Instance().CloseAllMenus(slot);
                return true;
            },
            32);
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

    builder.AddButton(
        tr.Get("actionKick"),
        [targetSlot](int slot) {
            auto& plrMgr = PlayerManager::Instance();
            auto* a = plrMgr.GetPlayerBySlot(slot);
            auto* t = plrMgr.GetPlayerBySlot(targetSlot);
            if (a && t)
            {
                const char* reason = "Kicked via admin menu";
                CS2Kit::Sdk::PlayerController(t->GetSlot()).Kick(reason);
                AdminSystem::Core::ChatService::Instance().BroadcastPunishment("kicked", a->GetName(), t->GetName(),
                                                                               reason, 0);
            }
            MenuManager::Instance().CloseAllMenus(slot);
        },
        adminMgr.HasPermission(adminSid, 'c') && canTarget);

    builder.AddButton(
        tr.Get("actionBan"),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Translations::Instance().Get("actionBan"),
                MakePunishmentCallback<Ban>([](PunishmentManager& pm, Ban& ban) { pm.IssueBan(ban); }));
            MenuManager::Instance().OpenMenu(slot, durMenu);
        },
        adminMgr.HasPermission(adminSid, 'd') && canTarget);

    builder.AddButton(
        tr.Get("actionMute"),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Translations::Instance().Get("actionMute"),
                MakePunishmentCallback<Mute>([](PunishmentManager& pm, Mute& mute) { pm.IssueMute(mute); }));
            MenuManager::Instance().OpenMenu(slot, durMenu);
        },
        adminMgr.HasPermission(adminSid, 'o') && canTarget);

    builder.AddButton(
        tr.Get("actionGag"),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Translations::Instance().Get("actionGag"),
                MakePunishmentCallback<Gag>([](PunishmentManager& pm, Gag& gag) { pm.IssueGag(gag); }));
            MenuManager::Instance().OpenMenu(slot, durMenu);
        },
        adminMgr.HasPermission(adminSid, 'p') && canTarget);

    builder.AddButton(
        tr.Get("actionWarn"),
        [targetSlot](int slot) {
            auto& plrMgr = PlayerManager::Instance();
            auto* a = plrMgr.GetPlayerBySlot(slot);
            auto* t = plrMgr.GetPlayerBySlot(targetSlot);
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
        adminMgr.HasPermission(adminSid, 'q') && canTarget);

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu

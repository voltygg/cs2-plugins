#include "AdminMenu_Punish.hpp"
#include "../../Core/Managers.hpp"
#include <CS2Kit/Core/Services.hpp>

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

using CS2Kit::Core::Kit;

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
        case 's':
            multiplier = 1;
            break;
        case 'm':
            multiplier = 60;
            break;
        case 'h':
            multiplier = 3600;
            break;
        case 'd':
            multiplier = 86400;
            break;
        default:
            return -1;
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
    auto& tr = Kit().Translations;

    struct DurationEntry
    {
        std::string Key;
        int Seconds;
    };

    static const DurationEntry Durations[] = {
        {"duration.5min", 300}, {"duration.30min", 1800}, {"duration.1h", 3600},
        {"duration.1d", 86400}, {"duration.7d", 604800},  {"duration.perm", 0},
    };

    MenuBuilder builder(std::format("{}: {}", actionName, tr.Get("panel.selectDuration", adminSlot)));

    for (const auto& dur : Durations)
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddButton(tr.Get(dur.Key, adminSlot), [callback, target, secs = dur.Seconds](int slot) {
            callback(slot, target, secs);
            Kit().Menus.CloseAllMenus(slot);
        });
    }

    // Custom duration row — opens a chat-input prompt; accepts "5m"/"2h"/"7d" or seconds.
    {
        int target = targetSlot;
        auto callback = onDuration;
        builder.AddInput(
            tr.Get("duration.custom", adminSlot), tr.Get("duration.customPrompt", adminSlot),
            [](int) { return std::string{}; },
            [callback, target](int slot, std::string_view text) -> bool {
                int seconds = ParseDuration(text);
                if (seconds < 0)
                    return false;  // re-prompt
                callback(slot, target, seconds);
                Kit().Menus.CloseAllMenus(slot);
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
        auto& plrMgr = Kit().Players;
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

            auto& pm = Sys().Punishments;
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
    auto& tr = Kit().Translations;
    return BuildPlayerPicker(adminSlot, tr.Get("category.punish", adminSlot), [](int admin, int target) {
        auto actions = BuildPunishActionsMenu(admin, target);
        if (actions)
            Kit().Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Kit().Translations;
    auto& adminMgr = Sys().Admins;
    auto& plrMgr = Kit().Players;

    auto* target = plrMgr.GetPlayerBySlot(targetSlot);
    if (!target)
        return nullptr;

    auto* admin = plrMgr.GetPlayerBySlot(adminSlot);
    if (!admin)
        return nullptr;

    int64_t adminSid = admin->GetSteamID();
    int64_t targetSid = target->GetSteamID();

    MenuBuilder builder(std::format("{}: {}", tr.Get("category.punish", adminSlot), target->GetName()));

    builder.AddButton(
        tr.Get("action.kick", adminSlot),
        [targetSlot](int slot) {
            auto& plrMgr = Kit().Players;
            auto* a = plrMgr.GetPlayerBySlot(slot);
            auto* t = plrMgr.GetPlayerBySlot(targetSlot);
            if (a && t)
            {
                const char* reason = "Kicked via admin menu";
                CS2Kit::Sdk::PlayerController(t->GetSlot()).Kick(reason);
                Sys().Chat.BroadcastPunishment("kicked", a->GetName(), t->GetName(),
                                                                               reason, 0);
            }
            Kit().Menus.CloseAllMenus(slot);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Kick));

    builder.AddButton(
        tr.Get("action.ban", adminSlot),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Kit().Translations.Get("action.ban", slot),
                MakePunishmentCallback<Ban>([](PunishmentManager& pm, Ban& ban) { pm.IssueBan(ban); }));
            Kit().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Ban));

    builder.AddButton(
        tr.Get("action.voiceMute", adminSlot),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Kit().Translations.Get("action.voiceMute", slot),
                MakePunishmentCallback<VoiceMute>(
                    [](PunishmentManager& pm, VoiceMute& mute) { pm.IssueVoiceMute(mute); }));
            Kit().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::VoiceMute));

    builder.AddButton(
        tr.Get("action.textMute", adminSlot),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Kit().Translations.Get("action.textMute", slot),
                MakePunishmentCallback<TextMute>(
                    [](PunishmentManager& pm, TextMute& mute) { pm.IssueTextMute(mute); }));
            Kit().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::TextMute));

    builder.AddButton(
        tr.Get("action.warn", adminSlot),
        [targetSlot](int slot) {
            auto& plrMgr = Kit().Players;
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
                Sys().Punishments.IssueWarning(warn);
            }
            Kit().Menus.CloseAllMenus(slot);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Warn));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu

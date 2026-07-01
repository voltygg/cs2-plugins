#include "AdminMenu_Punish.hpp"

#include "../../Core/ChatService.hpp"
#include "../../Core/Managers.hpp"
#include "../../Punishments/PunishmentManager.hpp"
#include "../AdminManager.hpp"
#include "PlayerPicker.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Menu/MenuPresets.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

using CS2Kit::Core::Engine;

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
    auto& tr = Engine().Translations;

    struct DurationEntry
    {
        const char* Key;
        int Seconds;
    };

    static const DurationEntry Durations[] = {
        {"duration.5min", 300}, {"duration.30min", 1800}, {"duration.1h", 3600},
        {"duration.1d", 86400}, {"duration.7d", 604800},  {"duration.perm", 0},
    };

    std::vector<std::pair<std::string, int>> presets;
    presets.reserve(std::size(Durations));
    for (const auto& dur : Durations)
        presets.emplace_back(tr.Get(dur.Key, adminSlot), dur.Seconds);

    // The preset is content-agnostic, so it fires onPick(viewerSlot, seconds) only - the plugin
    // applies the punishment and closes the menu stack here to preserve the prior behavior.
    auto onPick = [target = targetSlot, callback = std::move(onDuration)](int slot, int seconds) {
        callback(slot, target, seconds);
        Engine().Menus.CloseAllMenus(slot);
    };

    return ::CS2Kit::Menu::BuildDurationPicker(
        adminSlot, std::format("{}: {}", actionName, tr.Get("panel.selectDuration", adminSlot)), presets,
        std::move(onPick), tr.Get("duration.custom", adminSlot), tr.Get("duration.customPrompt", adminSlot), 32);
}

template <typename PunishmentT, typename IssueFunc>
static std::function<void(int, int, int)> MakePunishmentCallback(IssueFunc issueFunc)
{
    return [issueFunc](int adminSlot, int target, int duration) {
        auto& plrMgr = Engine().Players;
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

            auto& pm = App().Punishments;
            issueFunc(pm, punishment);
        }
    };
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishMenu(int adminSlot)
{
    auto& tr = Engine().Translations;
    return BuildPlayerPicker(adminSlot, tr.Get("category.punish", adminSlot), [](int admin, int target) {
        auto actions = BuildPunishActionsMenu(admin, target);
        if (actions)
            Engine().Menus.OpenMenu(admin, actions);
    });
}

std::shared_ptr<::CS2Kit::Menu::Menu> BuildPunishActionsMenu(int adminSlot, int targetSlot)
{
    auto& tr = Engine().Translations;
    auto& adminMgr = App().Admins;
    auto& plrMgr = Engine().Players;

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
            auto& plrMgr = Engine().Players;
            auto* a = plrMgr.GetPlayerBySlot(slot);
            auto* t = plrMgr.GetPlayerBySlot(targetSlot);
            if (a && t)
            {
                const char* reason = "Kicked via admin menu";
                CS2Kit::Sdk::PlayerController(t->GetSlot()).Kick(reason);
                App().Chat.BroadcastPunishment("kicked", a->GetName(), t->GetName(), reason, 0);
            }
            Engine().Menus.CloseAllMenus(slot);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Kick));

    builder.AddButton(
        tr.Get("action.ban", adminSlot),
        [targetSlot](int slot) {
            auto durMenu = BuildTimedPunishmentMenu(
                slot, targetSlot, Engine().Translations.Get("action.ban", slot),
                MakePunishmentCallback<Ban>([](PunishmentManager& pm, Ban& ban) { pm.IssueBan(ban); }));
            Engine().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Ban));

    builder.AddButton(
        tr.Get("action.voiceMute", adminSlot),
        [targetSlot](int slot) {
            auto durMenu =
                BuildTimedPunishmentMenu(slot, targetSlot, Engine().Translations.Get("action.voiceMute", slot),
                                         MakePunishmentCallback<VoiceMute>(
                                             [](PunishmentManager& pm, VoiceMute& mute) { pm.IssueVoiceMute(mute); }));
            Engine().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::VoiceMute));

    builder.AddButton(
        tr.Get("action.textMute", adminSlot),
        [targetSlot](int slot) {
            auto durMenu =
                BuildTimedPunishmentMenu(slot, targetSlot, Engine().Translations.Get("action.textMute", slot),
                                         MakePunishmentCallback<TextMute>(
                                             [](PunishmentManager& pm, TextMute& mute) { pm.IssueTextMute(mute); }));
            Engine().Menus.OpenMenu(slot, durMenu);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::TextMute));

    builder.AddButton(
        tr.Get("action.warn", adminSlot),
        [targetSlot](int slot) {
            auto& plrMgr = Engine().Players;
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
                App().Punishments.IssueWarning(warn);
            }
            Engine().Menus.CloseAllMenus(slot);
        },
        adminMgr.CanActOn(adminSid, targetSid, Permission::Warn));

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu

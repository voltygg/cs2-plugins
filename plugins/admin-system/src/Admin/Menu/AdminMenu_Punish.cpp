#include "AdminMenu_Punish.hpp"

#include "../../Core/Managers.hpp"
#include "../AdminManager.hpp"
#include "PlayerPicker.hpp"
#include "PunishFlow.hpp"

#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <format>
#include <string>
#include <utility>

using CS2Kit::Core::Engine;

namespace AdminSystem::Admin::Menu
{

using namespace AdminSystem::Punishments;
using CS2Kit::Menu::MenuBuilder;

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

    if (AnyTemplateUsable(adminSlot, targetSlot))
    {
        builder.AddSubmenu(tr.Get("punish.quickPunish", adminSlot),
                           [targetSlot](int slot) { return BuildQuickPunishMenu(slot, targetSlot); });
    }

    for (PunishType type : {PunishType::Kick, PunishType::Ban, PunishType::VoiceMute, PunishType::TextMute,
                            PunishType::Warn})
    {
        PendingPunishment pending{
            .Type = type,
            .TargetSlot = targetSlot,
            .TargetSteamId = targetSid,
            .TargetName = target->GetName(),
        };
        builder.AddButton(
            tr.Get(ActionTranslationKey(type), adminSlot),
            [pending = std::move(pending)](int slot) { Engine().Menus.OpenMenu(slot, BuildFirstStep(slot, pending)); },
            adminMgr.CanActOn(adminSid, targetSid, PermissionFor(type)));
    }

    return builder.Build();
}

}  // namespace AdminSystem::Admin::Menu

#pragma once

#include "../../Core/Managers.hpp"
#include "../Actions/ActionContext.hpp"
#include "../AdminManager.hpp"
#include "../Effects/EffectDescriptor.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Menu/MenuBuilder.hpp>
#include <CS2Kit/Menu/MenuManager.hpp>
#include <CS2Kit/Players/PlayerManager.hpp>
#include <CS2Kit/Sdk/MoveType.hpp>
#include <CS2Kit/Sdk/PlayerController.hpp>
#include <CS2Kit/Utils/Translations.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace AdminSystem::Admin::Menu
{

inline constexpr int HealthPresets[] = {1, 50, 100, 200, 500, 999};
inline constexpr int ArmorPresets[] = {0, 50, 100, 200, 500, 999};

/** True if @p admin holds @p flag and outranks @p target; false if either slot has no live player. */
inline bool CanActOnSlot(int admin, int target, Permission flag)
{
    auto& plrMgr = CS2Kit::Engine().Players;
    auto* a = plrMgr.GetPlayerBySlot(admin);
    auto* t = plrMgr.GetPlayerBySlot(target);
    if (!a || !t)
        return false;
    return App().Admins.CanActOn(a->GetSteamID(), t->GetSteamID(), flag);
}

/** Flag-string variant for the data-driven Action/Effect descriptors. */
inline bool CanActOnSlot(int admin, int target, const std::string& flags)
{
    auto& plrMgr = CS2Kit::Engine().Players;
    auto* a = plrMgr.GetPlayerBySlot(admin);
    auto* t = plrMgr.GetPlayerBySlot(target);
    if (!a || !t)
        return false;
    return App().Admins.HasAnyPermission(a->GetSteamID(), flags) &&
           App().Admins.CanTarget(a->GetSteamID(), t->GetSteamID());
}

/** A plain button row that runs a single-target @ref Actions::Action against (admin, target). */
inline void AddAction(CS2Kit::MenuBuilder& builder, const std::string& label, int admin, int target,
                      const Actions::Action& action)
{
    const Actions::Action* a = &action;
    builder.AddButton(
        label, [admin, target, a](int) { Actions::Run(admin, target, *a); },
        CanActOnSlot(admin, target, action.Permission));
}

/** A toggle row whose live state is an active @ref Effects::Effect on the target. */
inline void AddEffectToggleRow(CS2Kit::MenuBuilder& builder, int admin, int target, const Effects::Effect& effect)
{
    auto& tr = CS2Kit::Engine().Translations;
    const Effects::Effect* e = &effect;
    builder.AddToggle(
        tr.Get(effect.NameKey, admin), tr.Get("effectState.on", admin), tr.Get("effectState.off", admin),
        [target, id = effect.Id](int) { return App().Effects.IsActive(target, id); },
        [admin, target, e](int) { Effects::Toggle(admin, target, *e); }, CanActOnSlot(admin, target, effect.Flag));
}

/** A per-option picker for a @ref Effects::ParamEffect: one button per choice plus a Reset row. */
inline std::shared_ptr<CS2Kit::MenuView> BuildParamEffectMenu(int admin, int target,
                                                                const Effects::ParamEffect& effect)
{
    auto& tr = CS2Kit::Engine().Translations;
    auto* t = CS2Kit::Engine().Players.GetPlayerBySlot(target);
    if (!t)
        return nullptr;

    bool allowed = CanActOnSlot(admin, target, effect.Flag);
    const Effects::ParamEffect* e = &effect;
    CS2Kit::MenuBuilder builder(std::format("{}: {}", tr.Get(effect.NameKey, admin), t->GetName()));

    auto choices = effect.Choices ? effect.Choices() : std::vector<Effects::EffectChoice>{};
    for (const auto& choice : choices)
    {
        int param = choice.Param;
        builder.AddButton(
            choice.Label,
            [admin, target, e, param](int slot) {
                Effects::Apply(admin, target, param, *e);
                CS2Kit::Engine().Menus.CloseAllMenus(slot);
            },
            allowed);
    }

    if (!effect.ResetLabelKey.empty())
        builder.AddButton(
            tr.Get(effect.ResetLabelKey, admin),
            [admin, target, e](int slot) {
                Effects::Clear(admin, target, *e);
                CS2Kit::Engine().Menus.CloseAllMenus(slot);
            },
            allowed);

    return builder.Build();
}

/** A submenu row that opens the @ref BuildParamEffectMenu picker for a @ref Effects::ParamEffect. */
inline void AddEffectSubmenuRow(CS2Kit::MenuBuilder& builder, int admin, int target,
                                const Effects::ParamEffect& effect)
{
    auto& tr = CS2Kit::Engine().Translations;
    const Effects::ParamEffect* e = &effect;
    builder.AddSubmenu(
        tr.Get(effect.NameKey, admin), [admin, target, e](int) { return BuildParamEffectMenu(admin, target, *e); },
        CanActOnSlot(admin, target, effect.Flag));
}

/**
 * A toggle row whose live on-state is computed by @p isActive against the target's pawn (re-read
 * on every redraw) and whose press runs the toggle @ref Actions::Action. Because the same action
 * flips the state back off on a second press, the row doubles as an "undo" control (e.g. unfreeze).
 * @p targetPredicate factories below build the common @p isActive checks (move type, m_fFlags bit).
 */
inline void AddStateToggle(CS2Kit::MenuBuilder& builder, const std::string& label, int admin, int target,
                           std::function<bool(const CS2Kit::PlayerController&)> isActive,
                           const Actions::Action& action)
{
    auto& tr = CS2Kit::Engine().Translations;
    const Actions::Action* a = &action;

    builder.AddToggle(
        label, tr.Get("effectState.on", admin), tr.Get("effectState.off", admin),
        [target, isActive = std::move(isActive)](int) {
            CS2Kit::PlayerController pc(target);
            return pc.IsValid() && isActive(pc);
        },
        [admin, target, a](int) { Actions::Run(admin, target, *a); }, CanActOnSlot(admin, target, action.Permission));
}

/** State predicate for @ref AddStateToggle: pawn is currently in @p activeType (e.g. frozen). */
inline auto InMoveType(CS2Kit::MoveType activeType)
{
    return [activeType](const CS2Kit::PlayerController& pc) { return pc.GetMoveType() == activeType; };
}

/** State predicate for @ref AddStateToggle: an m_fFlags bit is set on the pawn (e.g. godmode). */
inline auto HasFlag(uint32_t flag)
{
    return [flag](const CS2Kit::PlayerController& pc) { return (pc.GetFlags() & flag) != 0; };
}

/** An inline Choice row: A/D cycles preset values, E applies the @ref Actions::ParamAction and closes. */
template <typename PresetArray>
void AddPresetChoice(CS2Kit::MenuBuilder& builder, const std::string& title, const std::string& unit, int admin,
                     int target, const Actions::ParamAction& action, const PresetArray& presets)
{
    using Choice = CS2Kit::ChoiceOption<int>::Choice;
    std::vector<Choice> choices;
    choices.reserve(std::size(presets));
    for (int v : presets)
        choices.push_back({std::format("{} {}", v, unit), v});

    const Actions::ParamAction* a = &action;
    auto idx = std::make_shared<int>(0);
    builder.AddChoice<int>(
        title, std::move(choices), [idx](int) { return *idx; }, [idx](int, int newIdx) { *idx = newIdx; },
        [admin, target, a](int slot, const int& value) {
            Actions::Run(admin, target, value, *a);
            CS2Kit::Engine().Menus.CloseAllMenus(slot);
        },
        CanActOnSlot(admin, target, action.Permission));
}

}  // namespace AdminSystem::Admin::Menu

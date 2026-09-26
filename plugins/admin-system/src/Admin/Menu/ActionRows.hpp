#pragma once

#include "Admin/Actions/ActionDispatcher.hpp"
#include "Admin/Effects/EffectDescriptor.hpp"
#include "Admin/Effects/EffectDispatcher.hpp"
#include "Admin/Effects/EffectManager.hpp"

#include <VoltMod/Core/Text/Translations.hpp>
#include <VoltMod/Entities/EntitySystem.hpp>
#include <VoltMod/Entities/Pawn.hpp>
#include <VoltMod/Menu/MenuBuilder.hpp>
#include <VoltMod/Menu/MenuModel.hpp>
#include <VoltMod/Players/PlayerManager.hpp>
#include <VoltMod/Players/PlayerRef.hpp>
#include <VoltMod/Players/Policy.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/** Builds common admin actions for one admin and optional target. */
class ActionRows
{
public:
    /** Referenced objects must outlive the generated rows. */
    struct Services
    {
        Actions::ActionDispatcher& Actions;
        VoltMod::Policy& Policy;
        VoltMod::Translations& Translations;
        VoltMod::PlayerManager& Players;
        VoltMod::EntitySystem& Entities;
        VoltMod::MenuSurface& Menus;
        Effects::EffectDispatcher& PlayerEffects;
        const Effects::EffectManager& Effects;
    };

    /** Rows requiring a target are disabled when @p target is empty. */
    ActionRows(const Services& services, VoltMod::PlayerRef admin, std::optional<VoltMod::PlayerRef> target);

    /** Checks the permission each time the row is drawn or used. */
    VoltMod::EnabledCondition Allows(std::string_view permission) const;

    /** Translates @p key for the admin. */
    std::string Translate(std::string_view key, VoltMod::Tokens tokens = {}) const;

    /** A button that runs a single-target action. */
    VoltMod::MenuItem Action(std::string_view labelKey, const Actions::Action& action);

    /** A toggle row whose state is @p isActive over the target's pawn, re-read on every redraw,
     *  and whose flip runs @p action. */
    VoltMod::MenuItem StateToggle(std::string_view labelKey, std::function<bool(const VoltMod::Pawn&)> isActive,
                                  const Actions::Action& action);

    /** A choice row over a fixed list of numbers. */
    struct PresetSpec
    {
        std::string_view LabelKey;
        /** Appended to each preset, for example `"100 HP"`. */
        std::string_view Unit;
        std::span<const int> Presets;
        const Actions::ParamAction& Action;
        /** Which preset the row starts on. */
        int Index = 0;
    };

    /** Applies the selected preset after stepping stops. */
    VoltMod::MenuItem Presets(const PresetSpec& spec);

    /** An on/off row for a data-defined effect. */
    VoltMod::MenuItem Effect(const Effects::EffectDescriptor& effect);

    /** A submenu over an effect's choices. */
    VoltMod::MenuItem EffectPicker(const Effects::EffectDescriptor& effect);

private:
    std::shared_ptr<VoltMod::Menu> BuildPicker(const Effects::EffectDescriptor& effect,
                                               VoltMod::EnabledCondition allowed) const;

    VoltMod::PlayerRef TargetRef() const { return _target.value_or(VoltMod::PlayerRef{}); }

    /** Shared by row callbacks so each menu stores one copy. The referenced services must still
     *  outlive the rows. */
    std::shared_ptr<const Services> _services;
    VoltMod::PlayerRef _admin;
    std::optional<VoltMod::PlayerRef> _target;
};

}  // namespace AdminSystem::Admin::Menu

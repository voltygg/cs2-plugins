#pragma once

#include "Core/Permissions.hpp"

#include <array>
#include <span>
#include <string_view>

namespace AdminSystem::Admin::Menu
{

/**
 * @brief Every row the admin panel can draw.
 *
 * The tables below own three things a menu builder must not decide for itself: the order rows
 * appear in, the label they carry, and the permission that makes them visible at all. Each tab's
 * builder switches on @ref RowId to turn an entry into a `MenuItem`; that switch has no
 * `default:`, so a new enumerator here is a compile error until the tab handles it.
 *
 * This header stays free of the game SDK so `MenuCatalogTests` can link it.
 */
enum class RowId
{
    // Punish
    LiftList,
    PunishPlayers,
    LiftBans,
    LiftMutes,
    PunishKick,
    PunishBan,
    PunishVoiceMute,
    PunishTextMute,
    PunishWarn,
    // Player Actions
    CheatCheck,
    Kill,
    Bring,
    Goto,
    Swap,
    Freeze,
    Noclip,
    Bury,
    Unbury,
    ChangeTeam,
    Speed,
    Slap,
    Health,
    Armor,
    Godmode,
    Weapons,
    // Player Fun
    Ghost,
    Disco,
    Wallhack,
    Model,
    Bhop,
    Drunk,
    Smite,
    Size,
    // Round Modes
    RoundModes,
    // Map & Vote
    ChangeMap,
    SetNextMap,
    VoteMap,
    CancelVote,
    // My Settings
    Hide,
    ChatPrefix,
    NameColor,
    MessageColor,
};

/**
 * One row of one menu. An empty @ref Permission means any registered admin may see it; a
 * non-empty @ref Children makes the row visible exactly when one of its children is, so a list
 * that would open empty is never offered.
 */
struct RowSpec
{
    RowId Id;
    std::string_view LabelKey;
    std::string_view Permission;
    std::span<const RowSpec> Children;
};

/** The punishment kinds the merged lift list may show, each with the permission that lifts it. */
inline constexpr std::array<RowSpec, 2> LiftRows{{
    {RowId::LiftBans, "action.unban", Permission::Unban, {}},
    {RowId::LiftMutes, "action.unmute", Permission::Mute, {}},
}};

/** The player card under the Punish tab. Mirrors `Punishments::PunishTypes` row for row;
 *  MenuCatalogTests checks the two agree, because the permission here decides what an admin
 *  sees and the one there decides what the dispatcher accepts. */
inline constexpr std::array<RowSpec, 5> PunishCardRows{{
    {RowId::PunishKick, "action.kick", Permission::Kick, {}},
    {RowId::PunishBan, "action.ban", Permission::Ban, {}},
    {RowId::PunishVoiceMute, "action.voiceMute", Permission::Mute, {}},
    {RowId::PunishTextMute, "action.textMute", Permission::Mute, {}},
    {RowId::PunishWarn, "action.warn", Permission::Mute, {}},
}};

inline constexpr std::array<RowSpec, 2> PunishTabRows{{
    {RowId::LiftList, "punish.activeList", {}, LiftRows},
    // The player rows only lead to the card, so the tab is worth showing when any card row is.
    {RowId::PunishPlayers, "category.punish", {}, PunishCardRows},
}};

inline constexpr std::array<RowSpec, 16> PlayerActionRows{{
    {RowId::CheatCheck, "action.cheatCheck", Permission::Control, {}},
    {RowId::Kill, "action.kill", Permission::Control, {}},
    {RowId::Bring, "action.bring", Permission::Control, {}},
    {RowId::Goto, "action.goto", Permission::Control, {}},
    {RowId::Swap, "action.swap", Permission::Control, {}},
    {RowId::Freeze, "action.freeze", Permission::Control, {}},
    {RowId::Noclip, "action.noclip", Permission::Control, {}},
    {RowId::Bury, "action.bury", Permission::Control, {}},
    {RowId::Unbury, "action.unbury", Permission::Control, {}},
    {RowId::ChangeTeam, "action.changeTeam", Permission::Control, {}},
    {RowId::Speed, "action.speed", Permission::Control, {}},
    {RowId::Slap, "action.slap", Permission::Control, {}},
    {RowId::Health, "action.health", Permission::Health, {}},
    {RowId::Armor, "action.armor", Permission::Health, {}},
    {RowId::Godmode, "action.godmode", Permission::Health, {}},
    {RowId::Weapons, "action.giveWeapon", Permission::Weapon, {}},
}};

inline constexpr std::array<RowSpec, 8> PlayerFunRows{{
    {RowId::Ghost, "action.ghost", Permission::Fun, {}},
    {RowId::Disco, "action.disco", Permission::Fun, {}},
    {RowId::Wallhack, "action.wallhack", Permission::Wallhack, {}},
    {RowId::Model, "action.model", Permission::Fun, {}},
    {RowId::Bhop, "action.bhop", Permission::Bhop, {}},
    {RowId::Drunk, "action.drunk", Permission::Fun, {}},
    {RowId::Smite, "action.smite", Permission::Fun, {}},
    {RowId::Size, "action.size", Permission::Fun, {}},
}};

/** Every modifier shares one permission, so a single entry describes the tab. The rows themselves
 *  stay in `Fun::Toggles`, whose order its own test pins. */
inline constexpr std::array<RowSpec, 1> RoundModeRows{{
    {RowId::RoundModes, "category.roundModes", Permission::FunMode, {}},
}};

inline constexpr std::array<RowSpec, 4> MapVoteRows{{
    {RowId::ChangeMap, "action.changeMap", Permission::Map, {}},
    {RowId::SetNextMap, "action.setNextMap", Permission::Map, {}},
    {RowId::VoteMap, "action.voteMap", Permission::Vote, {}},
    {RowId::CancelVote, "action.cancelVote", Permission::Vote, {}},
}};

inline constexpr std::array<RowSpec, 4> MySettingsRows{{
    {RowId::Hide, "action.hide", Permission::Hide, {}},
    {RowId::ChatPrefix, "chat.displayPrefix", {}, {}},
    {RowId::NameColor, "chat.nameColor", {}, {}},
    {RowId::MessageColor, "chat.messageColor", {}, {}},
}};

enum class TabId
{
    Punish,
    PlayerActions,
    PlayerFun,
    RoundModes,
    MapVote,
    MySettings,
};

/** One tab of the panel. Visible exactly when one of its rows is, so a tab cannot outlive its
 *  contents the way the old hand-written permission lists did. */
struct TabSpec
{
    TabId Id;
    std::string_view LabelKey;
    /** One of the names generated into panorama/screens/admin_menu/icons.j2. That set is fixed at
     *  six; reusing these names is what keeps a menu change out of the workshop publish queue. */
    std::string_view Icon;
    std::span<const RowSpec> Rows;
};

inline constexpr std::array<TabSpec, 6> Tabs{{
    {TabId::Punish, "category.punish", "punish", PunishTabRows},
    {TabId::PlayerActions, "category.playerActions", "control", PlayerActionRows},
    {TabId::PlayerFun, "category.playerFun", "effects", PlayerFunRows},
    {TabId::RoundModes, "category.roundModes", "fun", RoundModeRows},
    {TabId::MapVote, "category.mapVote", "map", MapVoteRows},
    {TabId::MySettings, "category.mySettings", "chat", MySettingsRows},
}};

/** The exact icon names icons.j2 generates. A tab naming anything else draws no icon at all. */
inline constexpr std::array<std::string_view, 6> IconNames{"punish", "control", "effects", "fun", "map", "chat"};

}  // namespace AdminSystem::Admin::Menu

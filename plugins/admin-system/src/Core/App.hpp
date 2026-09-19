#pragma once

#include "Admin/Access.hpp"
#include "Admin/Actions/Descriptors.hpp"
#include "Admin/AdminManager.hpp"
#include "Admin/CheatCheck/CheatCheckManager.hpp"
#include "Admin/Effects/Descriptors.hpp"
#include "Admin/FreezeManager.hpp"
#include "Config/ConfigManager.hpp"
#include "Core/AdminActionsService.hpp"
#include "Core/AdminMenuSection.hpp"
#include "Core/ChatService.hpp"
#include "Core/PlayerChat.hpp"
#include "Database/Repositories.hpp"
#include "Fun/FunMode.hpp"
#include "Maps/MapCycleState.hpp"
#include "Maps/VoteState.hpp"
#include "Punishments/PunishmentManager.hpp"
#include "Reports/ReportManager.hpp"
#include "Reports/ReportMenuSection.hpp"

#include <Ui/AdminMenu.hpp>
#include <VoltMod/Api.hpp>
#include <VoltMod/Core/Signals/Subscription.hpp>
#include <VoltMod/Core/Signals/Subscriptions.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Menu/PanoramaMenu.hpp>
#include <VoltMod/Menu/PanoramaMenuLayout.hpp>
#include <VoltMod/Players/ActionDispatcher.hpp>
#include <VoltMod/Players/EffectDispatcher.hpp>
#include <VoltMod/Players/EffectManager.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace AdminSystem
{

/**
 * Load-cycle object graph. Members are declared in dependency order and destroyed
 * in reverse, so callbacks stop before captured state and database services.
 * Access composes admin flags with freeze state; PlayerChat owns inbound rules
 * while ChatService remains output-only.
 */
struct App final : VoltMod::Plugin
{
    explicit App(VoltMod::Runtime& runtime) : Plugin(runtime) {}
    ~App() override;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Load() override;

    /** Open the admin main menu for @p slot; false when it could not be built. */
    bool OpenAdminMenu(int slot);

    /** Replaces the framework's default chat handling: chat rules first, then menus and commands. */
    bool OnPlayerChat(VoltMod::Player* player, std::string_view message, bool teamChat) override
    {
        return PlayerChat.HandleSay(player, message, teamChat);
    }

    /**
     * The admin-panel rows for one admin/target pair, bound to this plugin's services.
     *
     * The one place the VoltMod::ActionRows::Services bag is spelled: every menu file asks here
     * instead of assembling it again. An empty @p target is a panel with no target yet, whose
     * rows deny.
     */
    [[nodiscard]] VoltMod::ActionRows MenuRows(VoltMod::PlayerRef admin,
                                               std::optional<VoltMod::PlayerRef> target = std::nullopt)
    {
        return VoltMod::ActionRows({.Actions = Actions,
                                    .Policy = Runtime.Policy,
                                    .Translations = Runtime.Translations,
                                    .Players = Runtime.Players,
                                    .Entities = Runtime.Entities,
                                    .Menus = Runtime.Menus,
                                    .Effects = &Effects},
                                   admin, std::move(target));
    }

    /** The admin menu layout, and the clickable menu drawn on it when settings turn Panorama on. */
    VoltMod::PanoramaMenuLayout MenuLayout{Runtime.Screens, AdminMenuLayout::Layout, AdminMenuLayout::Tabs.size(),
                                           AdminMenuLayout::Rows.size(), AdminMenuLayout::IconSetNames};
    std::optional<VoltMod::PanoramaMenu> Panorama;
    /** Starts sessions on Panorama while held. Declared after it, so it lets go first. */
    VoltMod::Subscription PreferPanorama;

    Config::ConfigManager Settings;
    /** Runs the action descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::ActionDispatcher Actions{Runtime.Policy, Runtime.Entities};
    /** Action descriptors whose body needs an engine service beyond ActionContext (Slap, Smite,
     *  SetSize), built from Runtime once here. */
    Admin::Actions::ActionDescriptors ActionDescriptors{Runtime};
    VoltMod::Database Db{Runtime.Scheduler};
    Database::Repositories Repos{Db};
    Core::ChatService Chat{Runtime, Settings};
    /** Configured map list, the queued next map, and the level change itself. */
    Maps::MapCycleState MapCycle{Runtime, Settings};
    /** Server-wide round modifiers (Fun Mode). */
    Fun::FunMode FunMode{Runtime};
    /** The yes/no map vote an admin opens from the Map menu. */
    Maps::VoteState Votes{Runtime, Settings, MapCycle};
    Admin::AdminManager Admins{Repos, Settings};
    Admin::FreezeManager Freeze{Repos, Settings, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Repos, Settings, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Settings, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Repos, Settings, Runtime};
    VoltMod::EffectManager Effects{Runtime.Scheduler};
    /** Runs the effect descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::EffectDispatcher PlayerEffects{Actions, Effects};
    /** Every effect descriptor for this load cycle, built from Runtime. */
    Admin::Effects::EffectDescriptors EffectDescriptors{Runtime};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Settings, Chat};
    /** Published to other plugins in Load; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{Runtime, Punishments, Access};
    /** The main menu's admin entry; published in Load, withdrawn before these managers die. */
    Core::AdminMenuSection AdminSection{*this};
    /** The main menu's report entry; published in Load, withdrawn before these managers die. */
    Reports::ReportMenuSection ReportSection{*this};
    /** Load-time migration outcome shown by `admin_status`. */
    VoltMod::MigrationResult Migration;

private:
    void InstallPolicy();
    /** Subscribe to the roster's connect/disconnect signals. */
    void RegisterPlayerLifecycle();
    void OnPlayerConnect(VoltMod::Player& player);
    void OnPlayerDisconnect(VoltMod::Player& player);
    VoltMod::Status ConnectDatabase();
    VoltMod::Status LoadAdminData();
    VoltMod::Status InitializePunishments();
    void RegisterGameEventListeners();
    /** The engine asks per (receiver, sender) pair; a voice-muted sender is never heard. */
    void RegisterVoiceMuteHook();
    void InstallStatusReporting();
    void RegisterCommands();

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::Subscriptions _subs;
};

}  // namespace AdminSystem

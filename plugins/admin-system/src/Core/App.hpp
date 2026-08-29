#pragma once

#include "../Admin/Access.hpp"
#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/Effects/Descriptors.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Fun/FunMode.hpp"
#include "../Maps/MapCycleState.hpp"
#include "../Maps/VoteState.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Reports/ReportManager.hpp"
#include "AdminActionsService.hpp"
#include "ChatService.hpp"
#include "Config.hpp"
#include "PlayerChat.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/EffectManager.hpp>
#include <VoltMod/Core/Subscriptions.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Players/ActionDispatcher.hpp>
#include <VoltMod/Players/EffectDispatcher.hpp>
#include <optional>
#include <string>
#include <utility>

namespace AdminSystem
{

/**
 * Load-cycle object graph. Members are declared in dependency order and destroyed
 * in reverse, so callbacks stop before captured state and database services.
 * Access composes admin flags with freeze state; PlayerChat owns inbound rules
 * while ChatService remains output-only.
 */
struct App
{
    App(VoltMod::Runtime& runtime, std::string version) : Runtime(runtime), Version(std::move(version)) {}
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /** Connect the database, run migrations, load admins and register commands. */
    bool Start();

    /**
     * The menu host every menu in this plugin opens on.
     *
     * Chosen once in Start from `menu.style` and whether VoltMod::Capability::CustomUi is on, so
     * a session never straddles two hosts. Everything else asks for it here rather than reaching
     * for Runtime.HtmlMenus or Runtime.UiMenus directly.
     */
    VoltMod::MenuHost& Menus() const { return *_menus; }

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
                                    .Menus = Menus(),
                                    .Effects = &Effects},
                                   admin, std::move(target));
    }

    VoltMod::Runtime& Runtime;
    const std::string Version;

    Core::ConfigManager Config;
    /** Runs the action descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::ActionDispatcher Actions{Runtime.Policy, Runtime.Players, Runtime.Entities};
    /** Action descriptors whose body needs an engine service beyond ActionContext (Slap, Smite,
     *  SetSize), built from Runtime once here. */
    Admin::Actions::ActionDescriptors ActionDescriptors{Runtime};
    VoltMod::PostgresDatabase Db{Runtime.Scheduler};
    Database::PlayerRepository PlayerRepo{Db};
    Core::ChatService Chat{Runtime, Config};
    /** Configured map list, the queued next map, and the level change itself. */
    Maps::MapCycleState MapCycle{Runtime, Config};
    /** Server-wide round modifiers (Fun Mode). */
    Fun::FunMode FunMode{Runtime};
    /** The yes/no map vote an admin opens from the Map menu. */
    Maps::VoteState Votes{Runtime, Config, MapCycle};
    Admin::AdminManager Admins{Db, Config};
    Admin::FreezeManager Freeze{Db, Config, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Db, Config, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Config, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Db, Config, Runtime};
    VoltMod::EffectManager Effects{Runtime.Scheduler};
    /** Runs the effect descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::EffectDispatcher PlayerEffects{Actions, Effects};
    /** Every effect descriptor for this load cycle, built from Runtime. */
    Admin::Effects::EffectDescriptors EffectDescriptors{Runtime};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Config, Chat};
    /** Published to other plugins in Start; withdrawn before these managers die. */
    Core::AdminActionsService AdminActions{Runtime, Punishments, Access};
    /** Load-time migration outcome shown by `admin_status`. */
    VoltMod::MigrationResult Migration;

private:
    void InstallPolicy();
    /** Subscribe to the roster's connect/disconnect signals. */
    void RegisterPlayerLifecycle();
    void OnPlayerConnect(VoltMod::Player& player);
    void OnPlayerDisconnect(VoltMod::Player& player);
    VoltMod::StageResult ConnectDatabase();
    VoltMod::StageResult LoadAdminData();
    VoltMod::StageResult StartPunishments();
    void RegisterGameEventListeners();
    void InstallStatusReporting();
    void RegisterCommands();

    /** Pick the menu host from settings and capabilities, and say which one and why. */
    void SelectMenuHost();

    /** Never null after Start; points at a Runtime member, so it outlives this. */
    VoltMod::MenuHost* _menus = &Runtime.HtmlMenus;

    /** Held only when a workshop addon carries the layout; dropping it drops the requirement. */
    VoltMod::Subscription _menuAddon;

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::Subscriptions _subs;
};

}  // namespace AdminSystem

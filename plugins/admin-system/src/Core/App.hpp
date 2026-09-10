#pragma once

#include "../Admin/Access.hpp"
#include "../Admin/Actions/Descriptors.hpp"
#include "../Admin/AdminManager.hpp"
#include "../Admin/CheatCheck/CheatCheckManager.hpp"
#include "../Admin/Effects/Descriptors.hpp"
#include "../Admin/FreezeManager.hpp"
#include "../Config/ConfigManager.hpp"
#include "../Database/Repositories/PlayerRepository.hpp"
#include "../Fun/FunMode.hpp"
#include "../Maps/MapCycleState.hpp"
#include "../Menu/PanoramaMenu.hpp"
#include "../Maps/VoteState.hpp"
#include "../Punishments/PunishmentManager.hpp"
#include "../Reports/ReportManager.hpp"
#include "AdminActionsService.hpp"
#include "ChatService.hpp"
#include "PlayerChat.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Core/EffectManager.hpp>
#include <VoltMod/Core/SubscriptionScope.hpp>
#include <VoltMod/Database/Api.hpp>
#include <VoltMod/Menu/ActionRows.hpp>
#include <VoltMod/Players/ActionDispatcher.hpp>
#include <VoltMod/Players/EffectDispatcher.hpp>
#include <memory>
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
                                    .Menus = MenuFor(admin.Slot),
                                    .Effects = &Effects},
                                   admin, std::move(target));
    }

    /**
     * Where @p slot's menu is drawn: this plugin's Panorama screen when the player can see one,
     * the framework's center HTML otherwise.
     *
     * Asked while rows are built as well as when a menu opens, and answers the same either way,
     * so a row's callbacks and the session they run against never disagree.
     */
    [[nodiscard]] VoltMod::MenuSurface& MenuFor(int slot)
    {
        return Panorama.CanDraw(slot) ? static_cast<VoltMod::MenuSurface&>(Panorama) : Runtime.Menus;
    }

    /** Open @p menu for @p slot wherever MenuFor would put it. The one place the two surfaces are
     *  chosen between, so neither can be given options the other was not. */
    void OpenMenu(int slot, std::shared_ptr<VoltMod::Menu> menu, VoltMod::MenuOptions options = {})
    {
        if (!Panorama.Open(slot, menu, options))
            Runtime.Menus.Open(slot, std::move(menu), options);
    }

    VoltMod::Runtime& Runtime;
    const std::string Version;

    /** The admin menu's own Panorama surface. Declared early: menus are built against it. */
    Menus::PanoramaMenu Panorama{Runtime};

    Config::ConfigManager Settings;
    /** Runs the action descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::ActionDispatcher Actions{Runtime.Policy, Runtime.Entities};
    /** Action descriptors whose body needs an engine service beyond ActionContext (Slap, Smite,
     *  SetSize), built from Runtime once here. */
    Admin::Actions::ActionDescriptors ActionDescriptors{Runtime};
    VoltMod::PostgresDatabase Db{Runtime.Scheduler};
    Database::PlayerRepository PlayerRepo{Db};
    Core::ChatService Chat{Runtime, Settings};
    /** Configured map list, the queued next map, and the level change itself. */
    Maps::MapCycleState MapCycle{Runtime, Settings};
    /** Server-wide round modifiers (Fun Mode). */
    Fun::FunMode FunMode{Runtime};
    /** The yes/no map vote an admin opens from the Map menu. */
    Maps::VoteState Votes{Runtime, Settings, MapCycle};
    Admin::AdminManager Admins{Db, Settings};
    Admin::FreezeManager Freeze{Db, Settings, Runtime, Chat, Admins};
    /** The permission gate: granted flags minus abuse-protection freezes. Ask this, not Admins. */
    Admin::Access Access{Admins, Freeze};
    Punishments::PunishmentManager Punishments{Db, Settings, Runtime, Chat};
    Core::PlayerChat PlayerChat{Runtime, Settings, Chat, Admins, Punishments};
    Reports::ReportManager Reports{Db, Settings, Runtime};
    VoltMod::EffectManager Effects{Runtime.Scheduler};
    /** Runs the effect descriptors through Runtime::Policy: permissions, targeting and broadcasts. */
    VoltMod::EffectDispatcher PlayerEffects{Actions, Effects};
    /** Every effect descriptor for this load cycle, built from Runtime. */
    Admin::Effects::EffectDescriptors EffectDescriptors{Runtime};
    Admin::CheatCheck::CheatCheckManager CheatCheck{Runtime, Settings, Chat};
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

    /** Listener registrations, released together. Declared last: reverse member destruction
     *  stops the callbacks before the state they capture goes away. */
    VoltMod::SubscriptionScope _subs;
};

}  // namespace AdminSystem

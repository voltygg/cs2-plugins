#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/Api.hpp>
#include <VoltMod/Menu/Api.hpp>
#include <VoltMod/Ui/Api.hpp>
#include <VoltMod/Unsafe/Api.hpp>
#include <format>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include <networksystem/iprotobufbinding.h>
#include <optional>
#include <string>
#include <utility>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Status;

namespace Args = VoltMod::Args;

namespace UiLab
{

// What the spawn keyvalue actually became. Read back rather than trusted: a keyvalue the engine
// rejects is reported on its own console, not to the caller that set it.
static const VoltMod::SchemaField<const char*> kLayoutResource{"CCSCustomHudLayout", "m_strLayout"};

/** A failed Status as the handler's error: the router prints `Error::Detail` for us. */
static Result<Reply> Done(const Status& status, std::string ok)
{
    if (!status)
        return std::unexpected(status.error());
    return Reply{std::move(ok)};
}

// ConsoleOnly throughout: these are operator commands with no permission, and the console is the
// server itself. That also makes every one of them drivable over RCON.
void RegisterCommands(App& app)
{
    auto& commands = app.Runtime.Commands;

    commands.Add("uilab_spawn")
        .Describe("Spawn the lab layout, or a named one: uilab_spawn [layout]")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Opt<Args::Rest> layout) -> Result<Reply> {
            const std::string& name = layout.Value ? layout.Value->Value : app.Config.Get().ui.layout;
            return Done(app.Spawn(name),
                        std::format("spawned '{}'. Watch the CLIENT console for validation lines.", name));
        });

    commands.Add("uilab_remove")
        .Describe("Remove the spawned layout.")
        .ConsoleOnly()
        .Run([&app](Caller) -> Result<Reply> {
            app.Layout.Remove();
            return Reply{"removed."};
        });

    // What actually breaks after a CS2 update is the byte-pattern signatures, and the capability
    // carries the reason. Schema offsets resolve themselves by name, so there is nothing to dump.
    commands.Add("uilab_probe")
        .Describe("Report feature availability and the spawned layout.")
        .ConsoleOnly()
        .Run([&app](Caller c) -> Result<Reply> {
            for (VoltMod::Capability capability :
                 {VoltMod::Capability::CustomUi, VoltMod::Capability::UiClicks, VoltMod::Capability::Addons})
            {
                const bool ready = app.Runtime.Capabilities.Has(capability);
                c.SayRaw(std::format("{}: {}", VoltMod::Name(capability),
                                     ready ? "ready" : app.Runtime.Capabilities.Reason(capability)));
            }

            if (!app.Layout)
                return Reply{"no layout spawned."};

            // Zero per-player states is why a `_slot` command would fail, so it is worth saying.
            c.SayRaw(std::format("layout: handle {}, {} per-player states", app.Layout.Ref().Handle,
                                 app.Layout.PlayerStateCount()));

            VoltMod::Entity entity = app.Runtime.Entities.Resolve(app.Layout.Ref());
            const char* resource = entity ? VoltMod::SchemaPtr{entity.Raw()}.Get(kLayoutResource, nullptr) : nullptr;
            return Reply{std::format("m_strLayout: {}", resource ? resource : "<unset>")};
        });

    // The ownership check: a layout is removed when its UiPanel handle drops, so this stays at 1
    // across a `meta unload` + `meta load`. A climbing count means one leaked into the world.
    commands.Add("uilab_count")
        .Describe("Count the custom_hud_layout entities alive in the world.")
        .ConsoleOnly()
        .Run([&app](Caller) -> Result<Reply> {
            // Entity copies but does not assign, so the cursor is re-seated through emplace.
            auto& entities = app.Runtime.Entities;
            std::optional<VoltMod::Entity> cursor(entities.FindByClassName({}, "custom_hud_layout"));

            int alive = 0;
            while (*cursor)
            {
                ++alive;
                cursor.emplace(entities.FindByClassName(*cursor, "custom_hud_layout"));
            }
            return Reply{std::format("{} custom_hud_layout entities alive.", alive)};
        });

    // Nothing in a layout is clickable without this: no capture means no cursor.
    commands.Add("uilab_capture")
        .Describe("Give everyone a cursor: uilab_capture <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int enabled) -> Result<Reply> {
            const bool on = enabled.Value != 0;
            return Done(app.Layout.SetInputCapture(on),
                        std::format("input capture {} for everyone.", on ? "on" : "off"));
        });

    commands.Add("uilab_capture_slot")
        .Describe("Give one player a cursor: uilab_capture_slot <slot> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Int enabled) -> Result<Reply> {
            const bool on = enabled.Value != 0;
            return Done(app.Layout.For(slot.Value).SetInputCapture(on),
                        std::format("slot {} input capture {}.", slot.Value, on ? "on" : "off"));
        });

    // The one side-effect-free bound call, and so the safest to run first after a CS2 update: it
    // proves the pattern binding and the calling convention without touching state.
    commands.Add("uilab_capture_get")
        .Describe("Read input capture for a slot: uilab_capture_get <slot>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            auto enabled = app.Layout.For(slot.Value).InputCaptureEnabled();
            if (!enabled)
                return std::unexpected(enabled.error());
            return Reply{std::format("slot {} input capture: {}", slot.Value, *enabled ? "on" : "off")};
        });

    commands.Add("uilab_var")
        .Describe("Set a dialog variable for everyone: uilab_var <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            return Done(app.Layout.SetText(panel.Value, variable.Value, value.Value),
                        std::format("{}.{} = '{}' for everyone.", panel.Value, variable.Value, value.Value));
        });

    commands.Add("uilab_var_slot")
        .Describe("Set a dialog variable for one slot: uilab_var_slot <slot> <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            return Done(app.Layout.For(slot.Value).SetText(panel.Value, variable.Value, value.Value),
                        std::format("{}.{} = '{}' for slot {}.", panel.Value, variable.Value, value.Value, slot.Value));
        });

    commands.Add("uilab_class")
        .Describe("Toggle a class for everyone: uilab_class <panelId> <className> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word className, Args::Int on) -> Result<Reply> {
            const bool present = on.Value != 0;
            return Done(app.Layout.SetClass(panel.Value, className.Value, present),
                        std::format("{} class '{}' {}.", panel.Value, className.Value, present ? "added" : "removed"));
        });

    commands.Add("uilab_class_slot")
        .Describe("Toggle a class for one slot: uilab_class_slot <slot> <panelId> <className> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word className, Args::Int on) -> Result<Reply> {
            const bool present = on.Value != 0;
            return Done(app.Layout.For(slot.Value).SetClass(panel.Value, className.Value, present),
                        std::format("{} class '{}' {} for slot {}.", panel.Value, className.Value,
                                    present ? "added" : "removed", slot.Value));
        });

    commands.Add("uilab_class_reset")
        .Describe("Hand a class back to the layout: uilab_class_reset <panelId> <className>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word className) -> Result<Reply> {
            return Done(app.Layout.ResetClass(panel.Value, className.Value),
                        std::format("{} class '{}' left to the layout.", panel.Value, className.Value));
        });

    commands.Add("uilab_clicks")
        .Describe("Log every custom HUD press from every layout: uilab_clicks <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int enabled) -> Result<Reply> {
            if (enabled.Value == 0)
            {
                app.ClickLogger.Reset();
                return Reply{"click logging off."};
            }

            app.ClickLogger = app.Runtime.Ui.Clicks.Clicked += [](const VoltMod::UiClick& click) {
                VoltMod::Log::Info("ui-lab: slot {} pressed '{}' in layout {:#x}.", click.Slot, click.ButtonId,
                                   click.Layout.Handle);
            };
            return Reply{"click logging on."};
        });

    // UiClicks needs the click message's id, and the registry has not answered to the name the
    // proto declares. This says what a given name does resolve to, so the right one can be found
    // by asking rather than by guessing in C++.
    commands.Add("uilab_msg")
        .Describe("Look a network message up by exact then partial name: uilab_msg <name>")
        .ConsoleOnly()
        .Run([&app](Caller c, Args::Rest name) -> Result<Reply> {
            auto* messages = app.Runtime.Unsafe.Interfaces.NetworkMessages;
            if (!messages)
                return std::unexpected(VoltMod::Error::NotReady("no INetworkMessages interface"));

            const auto report = [&c](const char* how, INetworkMessageInternal* found) {
                if (!found)
                {
                    c.SayRaw(std::format("  {}: no match", how));
                    return;
                }

                NetMessageInfo_t* info = found->GetNetMessageInfo();
                const char* resolved = info && info->m_pBinding ? info->m_pBinding->GetName() : "?";
                c.SayRaw(std::format("  {}: '{}' id {}", how, resolved, info ? info->m_MessageId : -1));
            };

            report("exact", messages->FindNetworkMessage(name.Value.c_str()));
            report("partial", messages->FindNetworkMessagePartial(name.Value.c_str()));
            return Reply::Silent();
        });

    // How the layout actually reaches a client that does not already have it: put the compiled
    // .vxml_c/.vcss_c in a workshop addon and require its id here.
    commands.Add("uilab_addon")
        .Describe("Require a workshop addon of every client: uilab_addon <id>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::U64 id) -> Result<Reply> {
            auto lease = app.Runtime.Addons.Require(id.Value);
            if (!lease)
                return std::unexpected(lease.error());

            // The lease is the requirement: held here, dropped with the plugin.
            app.Addons.push_back(std::move(*lease));
            return Reply{std::format("requiring addon {}; clients get it on their next connect.", id.Value)};
        });

    commands.Add("uilab_addons")
        .Describe("List the workshop addons clients are required to have.")
        .ConsoleOnly()
        .Run([&app](Caller c) -> Result<Reply> {
            const auto required = app.Runtime.Addons.Required();
            if (required.empty())
                return Reply{"no addons required."};

            for (uint64_t id : required)
                c.SayRaw(std::format("  {}", id));
            return Reply::Silent();
        });

    // The menu host on the same panel machinery, without admin-system's permissions in the way:
    // a submenu for Back, a toggle and a choice for the steppers, and Close on the footer.
    commands.Add("uilab_menu")
        .Describe("Open a sample menu on the Panorama host for a player: uilab_menu <slot>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            auto& menus = app.Runtime.UiMenus;
            auto submenu = [&app](int) {
                return VoltMod::MenuBuilder("Lab submenu")
                    .Text("Back should return to the lab menu.")
                    .Button("Log a press", [](int who) { VoltMod::Log::Info("ui-lab: slot {} pressed the row.", who); })
                    .Build();
            };
            auto menu = VoltMod::MenuBuilder("Lab menu")
                            .Subtitle("uilab_menu")
                            .Toggle(
                                "Toggle", "ON", "OFF", [&app](int) { return app.MenuToggle; },
                                [&app](int) { app.MenuToggle = !app.MenuToggle; })
                            .Choice<int>("Choice", {{"One", 1}, {"Two", 2}, {"Three", 3}}, [](int, const int&) {})
                            .Submenu("Submenu", submenu)
                            .Button("Close from a row", [&menus](int who) { menus.CloseAllMenus(who); })
                            .Build();
            menus.OpenMenu(slot.Value, std::move(menu), {.FreezeMovement = false});
            return Reply{std::format("menu opened for slot {}.", slot.Value)};
        });
}

}  // namespace UiLab

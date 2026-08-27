#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/Api.hpp>
#include <VoltMod/Hud/Api.hpp>
#include <format>
#include <optional>
#include <string>
#include <utility>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Status;

namespace Args = VoltMod::Args;

namespace HudLab
{

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

    commands.Add("hudlab_spawn")
        .Describe("Spawn the lab layout, or a named one: hudlab_spawn [layout]")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Opt<Args::Rest> layout) -> Result<Reply> {
            const std::string& name = layout.Value ? layout.Value->Value : app.Config.Get().hud.layout;
            return Done(app.Spawn(name),
                        std::format("spawned '{}'. Watch the CLIENT console for validation lines.", name));
        });

    commands.Add("hudlab_remove")
        .Describe("Remove the spawned layout.")
        .ConsoleOnly()
        .Run([&app](Caller) -> Result<Reply> {
            app.Layout.Remove();
            return Reply{"removed."};
        });

    // What actually breaks after a CS2 update is the byte-pattern signatures, and the capability
    // carries the reason. Schema offsets resolve themselves by name, so there is nothing to dump.
    commands.Add("hudlab_probe")
        .Describe("Report feature availability and the spawned layout.")
        .ConsoleOnly()
        .Run([&app](Caller c) -> Result<Reply> {
            for (VoltMod::Capability capability :
                 {VoltMod::Capability::CustomHud, VoltMod::Capability::HudClicks, VoltMod::Capability::Addons})
            {
                const bool ready = app.Runtime.Capabilities.Has(capability);
                c.SayRaw(std::format("{}: {}", VoltMod::Name(capability),
                                     ready ? "ready" : app.Runtime.Capabilities.Reason(capability)));
            }

            if (!app.Layout)
                return Reply{"no layout spawned."};

            // Zero per-player states is why a `_slot` command would fail, so it is worth saying.
            return Reply{std::format("layout: handle {}, {} per-player states", app.Layout.Ref().Handle,
                                     app.Layout.PlayerStateCount())};
        });

    // The ownership check: a layout is removed when its Hud handle drops, so this stays at 1
    // across a `meta unload` + `meta load`. A climbing count means one leaked into the world.
    commands.Add("hudlab_count")
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
    commands.Add("hudlab_capture")
        .Describe("Give everyone a cursor: hudlab_capture <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int enabled) -> Result<Reply> {
            const bool on = enabled.Value != 0;
            return Done(app.Layout.SetInputCapture(on),
                        std::format("input capture {} for everyone.", on ? "on" : "off"));
        });

    commands.Add("hudlab_capture_slot")
        .Describe("Give one player a cursor: hudlab_capture_slot <slot> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Int enabled) -> Result<Reply> {
            const bool on = enabled.Value != 0;
            return Done(app.Layout.For(slot.Value).SetInputCapture(on),
                        std::format("slot {} input capture {}.", slot.Value, on ? "on" : "off"));
        });

    // The one side-effect-free bound call, and so the safest to run first after a CS2 update: it
    // proves the pattern binding and the calling convention without touching state.
    commands.Add("hudlab_capture_get")
        .Describe("Read input capture for a slot: hudlab_capture_get <slot>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            auto enabled = app.Layout.For(slot.Value).InputCaptureEnabled();
            if (!enabled)
                return std::unexpected(enabled.error());
            return Reply{std::format("slot {} input capture: {}", slot.Value, *enabled ? "on" : "off")};
        });

    commands.Add("hudlab_var")
        .Describe("Set a dialog variable for everyone: hudlab_var <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            return Done(app.Layout.SetText(panel.Value, variable.Value, value.Value),
                        std::format("{}.{} = '{}' for everyone.", panel.Value, variable.Value, value.Value));
        });

    commands.Add("hudlab_var_slot")
        .Describe("Set a dialog variable for one slot: hudlab_var_slot <slot> <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            return Done(app.Layout.For(slot.Value).SetText(panel.Value, variable.Value, value.Value),
                        std::format("{}.{} = '{}' for slot {}.", panel.Value, variable.Value, value.Value, slot.Value));
        });

    commands.Add("hudlab_class")
        .Describe("Toggle a class for everyone: hudlab_class <panelId> <className> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word className, Args::Int on) -> Result<Reply> {
            const bool present = on.Value != 0;
            return Done(app.Layout.SetClass(panel.Value, className.Value, present),
                        std::format("{} class '{}' {}.", panel.Value, className.Value, present ? "added" : "removed"));
        });

    commands.Add("hudlab_class_slot")
        .Describe("Toggle a class for one slot: hudlab_class_slot <slot> <panelId> <className> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word className, Args::Int on) -> Result<Reply> {
            const bool present = on.Value != 0;
            return Done(app.Layout.For(slot.Value).SetClass(panel.Value, className.Value, present),
                        std::format("{} class '{}' {} for slot {}.", panel.Value, className.Value,
                                    present ? "added" : "removed", slot.Value));
        });

    commands.Add("hudlab_class_reset")
        .Describe("Hand a class back to the layout: hudlab_class_reset <panelId> <className>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word className) -> Result<Reply> {
            return Done(app.Layout.ResetClass(panel.Value, className.Value),
                        std::format("{} class '{}' left to the layout.", panel.Value, className.Value));
        });

    // How the layout actually reaches a client that does not already have it: put the compiled
    // .vxml_c/.vcss_c in a workshop addon and require its id here.
    commands.Add("hudlab_addon")
        .Describe("Require a workshop addon of every client: hudlab_addon <id>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::U64 id) -> Result<Reply> {
            auto lease = app.Runtime.Addons.Require(id.Value);
            if (!lease)
                return std::unexpected(lease.error());

            // The lease is the requirement: held here, dropped with the plugin.
            app.Addons.push_back(std::move(*lease));
            return Reply{std::format("requiring addon {}; clients get it on their next connect.", id.Value)};
        });

    commands.Add("hudlab_addons")
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
}

}  // namespace HudLab

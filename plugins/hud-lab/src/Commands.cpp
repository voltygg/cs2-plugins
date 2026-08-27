#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Entities/Api.hpp>
#include <cstdlib>
#include <format>
#include <string>

// Every framework name lives in VoltMod. Name the few this file leans on here, in the .cpp -
// never a using-directive, and never in a header.
using VoltMod::Caller;
using VoltMod::HudClass;
using VoltMod::HudLayout;
using VoltMod::Reply;
using VoltMod::Result;
using VoltMod::Status;

// The command argument types are the one nested namespace worth an alias.
namespace Args = VoltMod::Args;

namespace HudLab
{

/** Turns a failed Status into the console reply, so every handler reports the same way. */
static Result<Reply> Report(const Status& status, std::string ok)
{
    if (!status)
        return Reply{std::format("failed: {}", status.error().Detail)};
    return Reply{std::move(ok)};
}

/** The spawned layout, or a reply saying there is not one yet. */
static Result<HudLayout> Resolve(App& app)
{
    HudLayout layout = app.Runtime.World.CustomHud.Get(app.Layout);
    if (!layout)
        return std::unexpected(VoltMod::Error::NotFound("no layout is spawned; run hudlab_spawn first"));
    return layout;
}

/** `Present`, `Absent` or `Undefined`, parsed by name through EnumNames. */
static Result<HudClass> ParseClass(const std::string& text)
{
    if (auto value = VoltMod::Parse<HudClass>(text))
        return *value;
    return std::unexpected(
        VoltMod::Error::Invalid(std::format("'{}' is not one of Present, Absent or Undefined", text)));
}

// ConsoleOnly throughout: these are operator commands with no permission, and the console is the
// server itself. That also makes every one of them drivable over RCON.
void RegisterCommands(App& app)
{
    auto& commands = app.Runtime.Commands;

    commands.Add("hudlab_spawn")
        .Describe("Spawn a custom_hud_layout: hudlab_spawn <panorama/layout/custom_game/x.xml>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Rest resource) -> Result<Reply> {
            app.Runtime.World.CustomHud.Remove(app.Layout);  // one layout per lab session

            auto layout = app.Runtime.World.CustomHud.Spawn(resource.Value);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            app.Layout = layout->Ref();
            return Reply{std::format("spawned. Watch the CLIENT console for CustomHud validation lines.")};
        });

    commands.Add("hudlab_remove")
        .Describe("Remove the spawned custom_hud_layout.")
        .ConsoleOnly()
        .Run([&app](Caller) -> Result<Reply> {
            app.Runtime.World.CustomHud.Remove(app.Layout);
            app.Layout = {};
            return Reply{"removed."};
        });

    commands.Add("hudlab_probe")
        .Describe("Dump the resolved schema offsets and the live table contents.")
        .ConsoleOnly()
        .Run([&app](Caller c) -> Result<Reply> {
            c.SayRaw(std::format("CustomHud: {}", app.Runtime.Capabilities.Has(VoltMod::Capability::CustomHud)
                                                      ? "ready"
                                                      : app.Runtime.Capabilities.Reason(VoltMod::Capability::CustomHud)));
            c.SayRaw(std::format("HudClicks: {}", app.Runtime.Capabilities.Has(VoltMod::Capability::HudClicks)
                                                      ? "ready"
                                                      : app.Runtime.Capabilities.Reason(VoltMod::Capability::HudClicks)));

            for (const std::string& line : app.Runtime.World.CustomHud.Get(app.Layout).Describe())
                c.SayRaw(line);
            return Reply::Silent();
        });

    // Nothing in a layout is clickable without this: no capture means no cursor. The global form
    // is the one to reach for - per-player state only exists when the entity carries it.
    commands.Add("hudlab_capture")
        .Describe("Give everyone a cursor: hudlab_capture <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int enabled) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            const bool on = enabled.Value != 0;
            return Report(layout->SetInputCapture(on), std::format("input capture {} for everyone.", on ? "on" : "off"));
        });

    commands.Add("hudlab_capture_slot")
        .Describe("Give one player a cursor: hudlab_capture_slot <slot> <0|1>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Int enabled) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            const bool on = enabled.Value != 0;
            return Report(layout->SetInputCaptureFor(slot.Value, on),
                          std::format("slot {} input capture {}.", slot.Value, on ? "on" : "off"));
        });

    // The one side-effect-free bound call, and so the safest to run first after a CS2 update: it
    // proves the pattern binding and the calling convention without touching state.
    commands.Add("hudlab_capture_get")
        .Describe("Read input capture for a slot: hudlab_capture_get <slot>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            auto enabled = layout->InputCaptureEnabled(slot.Value);
            if (!enabled)
                return Reply{std::format("failed: {}", enabled.error().Detail)};
            return Reply{std::format("slot {} input capture: {}", slot.Value, *enabled ? "on" : "off")};
        });

    commands.Add("hudlab_var")
        .Describe("Set a dialog variable for everyone: hudlab_var <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            return Report(layout->SetText(panel.Value, variable.Value, value.Value),
                          std::format("{}.{} = '{}' for everyone.", panel.Value, variable.Value, value.Value));
        });

    commands.Add("hudlab_var_slot")
        .Describe("Set a dialog variable for one slot: hudlab_var_slot <slot> <panelId> <variable> <value...>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word variable, Args::Rest value) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            return Report(layout->SetTextFor(slot.Value, panel.Value, variable.Value, value.Value),
                          std::format("{}.{} = '{}' for slot {}.", panel.Value, variable.Value, value.Value, slot.Value));
        });

    commands.Add("hudlab_class")
        .Describe("Toggle a class for everyone: hudlab_class <panelId> <className> <Present|Absent|Undefined>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word panel, Args::Word className, Args::Word state) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            auto value = ParseClass(state.Value);
            if (!value)
                return Reply{std::format("failed: {}", value.error().Detail)};

            return Report(layout->SetClass(panel.Value, className.Value, *value),
                          std::format("{} class '{}' -> {} for everyone.", panel.Value, className.Value, state.Value));
        });

    commands.Add("hudlab_class_slot")
        .Describe("Toggle a class for one slot: hudlab_class_slot <slot> <panelId> <className> <state>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot, Args::Word panel, Args::Word className, Args::Word state) -> Result<Reply> {
            auto layout = Resolve(app);
            if (!layout)
                return Reply{std::format("failed: {}", layout.error().Detail)};

            auto value = ParseClass(state.Value);
            if (!value)
                return Reply{std::format("failed: {}", value.error().Detail)};

            return Report(layout->SetClassFor(slot.Value, panel.Value, className.Value, *value),
                          std::format("{} class '{}' -> {} for slot {}.", panel.Value, className.Value, state.Value,
                                      slot.Value));
        });

    // How the layout actually reaches a client that does not already have it: put the compiled
    // .vxml_c/.vcss_c in a workshop addon and require its id here.
    commands.Add("hudlab_addon")
        .Describe("Require a workshop addon of every client: hudlab_addon <id>")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Word id) -> Result<Reply> {
            const uint64_t workshopId = std::strtoull(id.Value.c_str(), nullptr, 10);
            if (workshopId == 0)
                return Reply{std::format("failed: '{}' is not a workshop id", id.Value)};

            app.Runtime.Addons.Require(workshopId);
            return Reply{std::format("requiring addon {}; clients get it on their next connect.", workshopId)};
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

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <format>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace Args = VoltMod::Args;

namespace Ui
{

// The two cards the demo fills. Any card but Contracts::ServerCard is free for a plugin to take.
static constexpr Contracts::HudCard WheelCard = Contracts::HudCard::Second;
static constexpr Contracts::HudCard DropCard = Contracts::HudCard::Third;

// Server console only: this fills the cards with sample content so a layout change can be seen
// without the plugin that would normally drive them.
void RegisterCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("ui_hud_demo")
        .Describe("Fill cards 1 and 2 and fire a toast for one slot (-1 = everyone).")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            const int target = slot.Value;
            app.Hud.SetCard(WheelCard, target,
                            {.Title = "Wheel of fortune",
                             .Value = "12:00",
                             .BarStep = 8,
                             .Accent = Contracts::HudAccent::Info});
            app.Hud.SetCard(DropCard, target,
                            {.Title = "UMP-45 | Crimson",
                             .Subtitle = "Winston",
                             .Value = "358 ₽",
                             .Icon = "ump45",
                             .Accent = Contracts::HudAccent::Rare});
            app.Hud.Toast(target, {.Title = "Prosthesis",
                                   .Description = "Immune to arm and leg hits",
                                   .Accent = Contracts::HudAccent::Success});
            return Reply{std::format("ui_hud_demo: drew the demo cards for {}.", target)};
        });
}

}  // namespace Ui

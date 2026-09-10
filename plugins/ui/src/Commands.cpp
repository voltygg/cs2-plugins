#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <format>

using VoltMod::Caller;
using VoltMod::Reply;
using VoltMod::Result;

namespace Args = VoltMod::Args;

namespace Ui
{

// Server console only: this fills the cards with sample content so a layout change can be seen
// without the plugin that would normally drive them.
void RegisterCommands(VoltMod::CommandManager& commands, App& app)
{
    commands.Add("ui_hud_demo")
        .Describe("Fill the HUD cards and fire a toast for one slot (-1 = everyone).")
        .ConsoleOnly()
        .Run([&app](Caller, Args::Int slot) -> Result<Reply> {
            const int target = slot.Value;
            app.Hud.SetCard(Contracts::HudCard::Wheel, target,
                                  {.Title = "Wheel of fortune",
                                   .Value = "12:00",
                                   .BarStep = 8,
                                   .Accent = Contracts::HudAccent::Info});
            app.Hud.SetCard(Contracts::HudCard::Drop, target,
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

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Hud/Api.hpp>
#include <format>
#include <string>
#include <utility>

namespace HudLab
{

void RegisterCommands(App& app);

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = "hud-lab"}))
        return false;

    RegisterCommands(*this);
    return true;
}

VoltMod::Status App::Spawn(std::string_view layout)
{
    auto hud = Runtime.Hud.Spawn(layout);
    if (!hud)
        return std::unexpected(hud.error());

    // Assigning removes the layout this was driving before, so the lab keeps one at a time.
    Layout = std::move(*hud);

    // Subscribing is what installs the click hook, and OnClick filters to this layout and this
    // button - two layouts with a `lab_accept` do not trigger each other.
    _buttons.clear();
    _buttons.push_back(Layout.OnClick("lab_accept", [this](int slot) { OnButton(slot, "accept"); }));
    _buttons.push_back(Layout.OnClick("lab_decline", [this](int slot) { OnButton(slot, "decline"); }));
    return {};
}

void App::OnButton(int slot, std::string_view button)
{
    VoltMod::Log::Info("hud-lab: slot {} pressed '{}'.", slot, button);

    // Per-player, so two players clicking see their own answer. lab_title is the Label carrying
    // the {s:title} dialog variable; lab_body next to it is static text and cannot be written.
    // This fails when the entity carries no per-player state - worth saying out loud in a lab
    // rather than looking like it worked.
    if (auto wrote = Layout.For(slot).SetText("lab_title", "title", std::format("you pressed {}", button)); !wrote)
        VoltMod::Log::Warn("hud-lab: per-player write failed ({}); hudlab_var writes it for everyone.",
                           wrote.error().Detail);
}

}  // namespace HudLab

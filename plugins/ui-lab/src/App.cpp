#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/Ui/Api.hpp>
#include <format>
#include <string>
#include <utility>

namespace UiLab
{

void RegisterCommands(App& app);

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = "ui-lab"}))
        return false;

    RegisterCommands(*this);
    return true;
}

VoltMod::Status App::Spawn(std::string_view layout)
{
    auto spawned = Runtime.Ui.Spawn(layout);
    if (!spawned)
        return std::unexpected(spawned.error());

    // Assigning removes the layout this was driving before, so the lab keeps one at a time.
    Layout = std::move(*spawned);

    // Subscribing is what installs the click hook, and Button filters to this layout and this
    // button id - two layouts with a `lab_accept` do not trigger each other.
    _buttons.Clear();
    _buttons.On(Layout.Button("lab_accept"), [this](int slot) { OnButton(slot, "accept"); });
    _buttons.On(Layout.Button("lab_decline"), [this](int slot) { OnButton(slot, "decline"); });
    return {};
}

void App::OnButton(int slot, std::string_view button)
{
    VoltMod::Log::Info("ui-lab: slot {} pressed '{}'.", slot, button);

    // Per-player, so two players clicking see their own answer. lab_title is the Label carrying
    // the {s:title} dialog variable; lab_body next to it is static text and cannot be written.
    // This fails when the entity carries no per-player state - worth saying out loud in a lab
    // rather than looking like it worked.
    if (auto wrote = Layout.Text(slot, "lab_title", "title", std::format("you pressed {}", button)); !wrote)
        VoltMod::Log::Warn("ui-lab: per-player write failed ({}); uilab_var writes it for everyone.",
                           wrote.error().Detail);
}

}  // namespace UiLab

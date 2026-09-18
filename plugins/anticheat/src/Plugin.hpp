#pragma once

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <optional>

/** Entry point: metadata, and the object graph for one load cycle. The base owns the Metamod
 *  lifecycle, standard hooks and player tracking. */
class AnticheatPlugin final : public VoltMod::Plugin
{
protected:
    VoltMod::PluginInfo Info() const override;
    bool OnLoad(VoltMod::Runtime& runtime) override;
    void OnUnload() override { _app.reset(); }
    void OnServerStartup(std::string_view mapName) override;

private:
    std::optional<Anticheat::App> _app;
};

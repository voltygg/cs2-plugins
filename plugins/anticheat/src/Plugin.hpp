#pragma once

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <optional>

/**
 * Anticheat plugin entry point. VoltMod::MetamodPlugin owns the Metamod lifecycle, standard
 * hooks and player tracking; this class adds the metadata and owns the plugin's object graph
 * for one load cycle.
 */
class AnticheatPlugin final : public VoltMod::MetamodPlugin
{
protected:
    VoltMod::PluginInfo Info() const override;
    bool OnLoad(VoltMod::Runtime& runtime, bool late) override;
    void OnUnload() override { _app.reset(); }
    void OnServerStartup(const char* mapName) override;
    void OnPlayerFullyConnected(VoltMod::Player* player) override;
    void OnPlayerSettingsChanged(VoltMod::Player* player) override;

private:
    std::optional<Anticheat::App> _app;
};

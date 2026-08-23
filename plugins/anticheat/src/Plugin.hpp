#pragma once

#include "App.hpp"

#include <CS2Kit/Api.hpp>
#include <optional>

/**
 * Anticheat plugin entry point. CS2Kit::MetamodPlugin owns the Metamod lifecycle, standard
 * hooks and player tracking; this class adds the metadata and owns the plugin's object graph
 * for one load cycle.
 */
class AnticheatPlugin final : public CS2Kit::MetamodPlugin
{
protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(CS2Kit::Runtime& runtime, bool late) override;
    void OnUnload() override { _app.reset(); }
    void OnServerStartup(const char* mapName) override;
    void OnPlayerFullyConnected(CS2Kit::Player* player) override;
    void OnPlayerSettingsChanged(CS2Kit::Player* player) override;

private:
    std::optional<Anticheat::App> _app;
};

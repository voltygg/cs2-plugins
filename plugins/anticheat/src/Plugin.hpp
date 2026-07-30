#pragma once

#include "Managers.hpp"

#include <CS2Kit/Api.hpp>

/**
 * Anticheat plugin entry point. CS2Kit::PluginBase owns the Metamod lifecycle, standard
 * hooks, player tracking, and the Managers container; this class adds plugin metadata
 * and subsystem wiring. Reach the managers via Anticheat::App().
 */
class AnticheatPlugin : public CS2Kit::PluginBase<Anticheat::Managers>
{
protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(bool late) override;
    void OnServerStartup(const char* mapName) override;
    void OnPlayerFullyConnected(CS2Kit::Players::Player* player) override;
    void OnPlayerSettingsChanged(CS2Kit::Players::Player* player) override;
};

#pragma once

#include "Managers.hpp"

#include <CS2Kit/Api.hpp>

/**
 * Bhop plugin entry point. CS2Kit::PluginBase owns the Metamod lifecycle, standard
 * hooks, player tracking, and the Managers container; this class adds plugin metadata
 * and subsystem wiring. Reach the managers via Bhop::App().
 */
class BhopPlugin : public CS2Kit::PluginBase<Bhop::Managers>
{
protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(bool late) override;
    void OnPlayerDisconnect(CS2Kit::Player* player) override;
};

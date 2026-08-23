#pragma once

#include "App.hpp"

#include <CS2Kit/Api.hpp>
#include <optional>

/**
 * Bhop plugin entry point. CS2Kit::MetamodPlugin owns the Metamod lifecycle, standard hooks
 * and player tracking; this class adds the metadata and owns the plugin's object graph for
 * one load cycle.
 */
class BhopPlugin final : public CS2Kit::MetamodPlugin
{
protected:
    CS2Kit::PluginInfo Info() const override;
    bool OnLoad(CS2Kit::Runtime& runtime, bool late) override;
    void OnUnload() override { _app.reset(); }
    void OnPlayerDisconnect(CS2Kit::Player* player) override;

private:
    std::optional<Bhop::App> _app;
};

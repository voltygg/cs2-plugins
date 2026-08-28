#pragma once

#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <optional>

/**
 * Bhop plugin entry point. VoltMod::MetamodPlugin owns the Metamod lifecycle, standard hooks
 * and player tracking; this class adds the metadata and owns the plugin's object graph for
 * one load cycle.
 */
class BhopPlugin final : public VoltMod::MetamodPlugin
{
protected:
    VoltMod::PluginInfo Info() const override;
    bool OnLoad(VoltMod::Runtime& runtime) override;
    void OnUnload() override { _app.reset(); }

private:
    std::optional<Bhop::App> _app;
};

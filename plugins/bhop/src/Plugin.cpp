#include "Plugin.hpp"

#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Log.hpp>
#include <CS2Kit/Utils/Translations.hpp>

using CS2Kit::Core::Engine;
namespace Log = CS2Kit::Utils::Log;

BhopPlugin g_BhopPlugin;
PLUGIN_EXPOSE(BhopPlugin, g_BhopPlugin);

namespace Bhop
{
Managers& App()
{
    return BhopPlugin::App();
}
}  // namespace Bhop

CS2Kit::PluginInfo BhopPlugin::Info() const
{
    return CS2Kit::PluginInfo{
        .Name = "Bhop",
        .Author = "m9snoi",
        .Description = "Smooth, client-predicted bunnyhop with per-player session grants.",
        .Url = "",
        .License = "MIT",
        .Version = "0.1.0",
        .LogTag = "BHOP",
    };
}

bool BhopPlugin::OnLoad(bool late)
{
    if (!Bhop::App().Config.Load(Bhop::SettingsPath))
        return false;

    Engine().Translations.SetLanguage(Bhop::App().Config.Get().plugin.locale);
    Engine().Translations.Load("addons/bhop/configs/translations");

    Bhop::App().Bhop.Initialize();

    Log::Info("Loaded v{}.", Info().Version);
    return true;
}

void BhopPlugin::OnPlayerDisconnect(CS2Kit::Player* player)
{
    Bhop::App().Bhop.OnPlayerDisconnect(player);
}

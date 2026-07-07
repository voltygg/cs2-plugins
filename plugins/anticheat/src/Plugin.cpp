#include "Plugin.hpp"

#include "Config.hpp"
#include "Managers.hpp"

#include <CS2Kit/Api.hpp>
#include <CS2Kit/Core/Services.hpp>
#include <CS2Kit/Utils/Log.hpp>

using CS2Kit::Core::Engine;
namespace Log = CS2Kit::Utils::Log;

AnticheatPlugin g_AnticheatPlugin;
PLUGIN_EXPOSE(AnticheatPlugin, g_AnticheatPlugin);

namespace Anticheat
{
Managers& App()
{
    return AnticheatPlugin::App();
}
}  // namespace Anticheat

CS2Kit::PluginInfo AnticheatPlugin::Info() const
{
    return CS2Kit::PluginInfo{
        .Name = "Anticheat",
        .Author = "m9snoi",
        .Description = "Detects blatant cheating (spinbot, aimlock, silent aim) from per-tick input analysis.",
        .Url = "",
        .License = "MIT",
        .Version = "0.1.0",
        .LogTag = "ANTICHEAT",
    };
}

bool AnticheatPlugin::OnLoad(bool /*late*/)
{
    if (!Anticheat::App().Config.Load(Anticheat::SettingsPath))
        return false;

    Engine().Translations.SetLanguage(Anticheat::App().Config.Get().plugin.locale);
    Engine().Translations.Load("addons/anticheat/configs/translations");

    Anticheat::App().Response.Initialize();
    Anticheat::App().AntiCheat.Initialize();

    Log::Info("Loaded v{} (mode={}).", Info().Version, Anticheat::App().Config.Get().anticheat.mode);
    return true;
}

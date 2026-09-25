#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>

VOLTMOD_PLUGIN(Bhop::App);

namespace Bhop
{

bool App::Load()
{
    if (!VoltMod::LoadConfig(Runtime, Config))
    {
        return false;
    }

    Bhop.Initialize();
    return true;
}

}  // namespace Bhop

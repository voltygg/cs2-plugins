#include "App.hpp"

#include <VoltMod/Api.hpp>
#include <VoltMod/App/PluginEntry.hpp>

VOLTMOD_PLUGIN(Bhop::App);

namespace Bhop
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config))
        return false;

    Bhop.Initialize();
    return true;
}

}  // namespace Bhop

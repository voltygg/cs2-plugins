#include "App.hpp"

#include <VoltMod/Api.hpp>

namespace Bhop
{

bool App::Start()
{
    if (!VoltMod::LoadStandardConfig(Runtime, Config, {.Addon = AddonName}))
        return false;

    Bhop.Initialize();
    return true;
}

}  // namespace Bhop

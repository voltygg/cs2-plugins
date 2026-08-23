#include "App.hpp"

#include <CS2Kit/Api.hpp>

namespace Bhop
{

bool App::Start()
{
    if (!CS2Kit::LoadStandardConfig(Config, {.Addon = AddonName}))
        return false;

    Bhop.Initialize();
    return true;
}

}  // namespace Bhop

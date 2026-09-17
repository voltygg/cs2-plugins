#pragma once

#include "App.hpp"

#include <string>
#include <vector>

namespace Anticheat
{

/** The plugin's `status` section as compact JSON text. Text, not a document, so this header
 *  stays clear of the JSON library. */
std::string StatusJson(const App& app);

/** The rule state and per-player evidence, one line each. */
std::vector<std::string> StatusLines(const App& app, double nowSec);

}  // namespace Anticheat

// With the registration macros gone the member name is the JSON key, and unknown keys are errors.
// This is the guard that a rename or a stale key in the shipped file fails here, not on a server.

#include "Config.hpp"

#include <VoltMod/Core/Json.hpp>
#include <doctest/doctest.h>
#include <fstream>
#include <iterator>
#include <string>

using Bhop::Settings;
using VoltMod::Json;

TEST_CASE("The shipped bhop settings.jsonc parses with nothing rejected")
{
    std::ifstream file(BHOP_SETTINGS_JSONC, std::ios::binary);
    REQUIRE_MESSAGE(file.is_open(), "cannot open " BHOP_SETTINGS_JSONC);
    const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto parsed = Json::Read<Settings>(text);
    const std::string why = parsed.has_value() ? std::string{} : parsed.error().Detail;
    REQUIRE_MESSAGE(parsed.has_value(), why);
    CHECK_FALSE(parsed->plugin.locale.empty());
}

TEST_CASE("A misspelled bhop key fails the load instead of reading a default")
{
    CHECK_FALSE(Json::Read<Settings>(R"({"bhop": {"notifyPlayr": true}})").has_value());
}

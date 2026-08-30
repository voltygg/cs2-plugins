// admin-system carries the largest settings schema in the repo and had no parsing coverage. With
// the registration macros gone, the member name *is* the JSON key, so a renamed member silently
// renames a setting - these cases are what catches that, and they are also where the strictness
// the reader gained (unknown keys are errors) is pinned down.
//
// This is the settings *model*, not ConfigManager: resolving a snapshot needs VoltMod::Validation's
// logging, path and duration helpers, which live in voltmod-runtime, and this binary is
// deliberately SDK-free. The validation rules are exercised in game; what cannot be checked there
// cheaply - that every shipped key still binds to a member - is checked here.

#include "Config/Settings.hpp"

#include <VoltMod/Core/Json.hpp>
#include <doctest/doctest.h>
#include <fstream>
#include <iterator>
#include <string>

using AdminSystem::Config::Settings;
using VoltMod::Json;

static std::string ShippedSettings()
{
    std::ifstream file(ADMIN_SETTINGS_JSONC, std::ios::binary);
    REQUIRE_MESSAGE(file.is_open(), "cannot open " ADMIN_SETTINGS_JSONC);
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST_CASE("The shipped settings.jsonc parses with nothing rejected")
{
    // Every key in the file has to name a member, `$schema` included, and every value has to have
    // the type that member declares. A renamed member fails right here.
    auto parsed = Json::Read<Settings>(ShippedSettings());
    const std::string why = parsed.has_value() ? std::string{} : parsed.error().Detail;
    REQUIRE_MESSAGE(parsed.has_value(), why);

    CHECK_FALSE(parsed->server.tag.empty());
    CHECK_FALSE(parsed->plugin.locale.empty());
}

TEST_CASE("The shipped settings.jsonc still names its schema")
{
    CHECK(ShippedSettings().find("$schema") != std::string::npos);
}

TEST_CASE("A misspelled key fails the load instead of reading a default")
{
    auto parsed = Json::Read<Settings>(R"({"sever": {"tag": "x"}})");
    REQUIRE_FALSE(parsed.has_value());
    CHECK(parsed.error().Code == VoltMod::ErrorCode::Invalid);
    CHECK(parsed.error().Detail.find("unknown_key") != std::string::npos);
}

TEST_CASE("A misspelled key inside a section is caught too")
{
    CHECK_FALSE(Json::Read<Settings>(R"({"reports": {"cooldwonSec": 60}})").has_value());
}

TEST_CASE("An omitted section keeps its struct defaults")
{
    auto parsed = Json::Read<Settings>(R"({"server": {"tag": "only-tag"}})");
    REQUIRE(parsed.has_value());
    CHECK(parsed->server.tag == "only-tag");
    CHECK(parsed->reports.cooldownSec == 120);
    // The non-empty in-class defaults the resolvers fall back to must survive reflection.
    CHECK_FALSE(parsed->maps.cycle.empty());
    CHECK_FALSE(parsed->weapons.menu.empty());
    CHECK_FALSE(parsed->reports.reasons.empty());
}

TEST_CASE("A wrong-typed value fails rather than reading as zero")
{
    CHECK_FALSE(Json::Read<Settings>(R"({"reports": {"cooldownSec": "120"}})").has_value());
}

TEST_CASE("Comments are accepted, as every shipped settings file uses them")
{
    auto parsed = Json::Read<Settings>("{\n  // the server's stable identity\n  \"server\": {\"tag\": \"a\"}\n}");
    REQUIRE(parsed.has_value());
    CHECK(parsed->server.tag == "a");
}

TEST_CASE("The database section maps onto the framework's own config type")
{
    // PostgresConfig's members are the JSON keys, so no mapper is defined anywhere for it.
    auto parsed = Json::Read<Settings>(R"({"database": {"host": "db", "port": 6543, "connectTimeoutSec": 9}})");
    REQUIRE(parsed.has_value());
    CHECK(parsed->database.host == "db");
    CHECK(parsed->database.port == 6543);
    CHECK(parsed->database.connectTimeoutSec == 9);
    CHECK(parsed->database.sslMode == "prefer");  // untouched default
}

TEST_CASE("The cheat-check request body stays free-form")
{
    // The body template is operator-authored: its shape belongs to whatever API it points at, so
    // it must survive unconstrained rather than being rejected as unknown keys.
    auto parsed = Json::Read<Settings>(
        R"({"cheatCheck": {"websiteAutoRoom": {"requestBody": {"purpose": "x", "metadata": {"reason": "y"}}}}})");
    REQUIRE(parsed.has_value());

    const auto& body = parsed->cheatCheck.websiteAutoRoom.requestBody;
    REQUIRE(body.is_object());
    CHECK(body.at("purpose").get_string() == "x");
    CHECK(body.at("metadata").at("reason").get_string() == "y");
}

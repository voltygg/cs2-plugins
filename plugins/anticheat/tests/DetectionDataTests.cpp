// The mapping between configs/detections.jsonc and the structs the rule engine consumes. These
// cases exist because the generous nlohmann macros this file deliberately does *not* use would
// accept every malformed input below and silently produce a working-looking table.

#include "Core/DetectionData.hpp"
#include "Detectors/InvalidCvarRules.hpp"

#include <doctest/doctest.h>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

using namespace Anticheat;

namespace
{
DetectionData Parse(const std::string& text)
{
    return nlohmann::json::parse(text).get<DetectionData>();
}

/** A whole file with @p rules substituted in, since both sections are required. */
std::string FileWith(const std::string& rules)
{
    return R"({"dllEventBlacklist": ["player_chat"], "cvarRules": [)" + rules + "]}";
}
}  // namespace

TEST_CASE("Every tier and constraint spelling the shipped file uses round-trips")
{
    const DetectionData data = Parse(FileWith(R"(
        {"name": "a", "tier": "queried",  "constraint": "equals",    "value": 89},
        {"name": "b", "tier": "queried",  "constraint": "max",       "value": 1},
        {"name": "c", "tier": "queried",  "constraint": "range",     "value": 1, "max": 20},
        {"name": "d", "tier": "queried",  "constraint": "minOrZero", "value": 64},
        {"name": "e", "tier": "queried",  "constraint": "off"},
        {"name": "f", "tier": "userinfo", "constraint": "on"}
    )"));

    REQUIRE(data.cvarRules.size() == 6);
    CHECK(data.cvarRules[0].constraint == CvarConstraint::Equals);
    CHECK(data.cvarRules[1].constraint == CvarConstraint::Max);
    CHECK(data.cvarRules[2].constraint == CvarConstraint::Range);
    CHECK(data.cvarRules[3].constraint == CvarConstraint::MinOrZero);
    CHECK(data.cvarRules[4].constraint == CvarConstraint::Off);
    CHECK(data.cvarRules[5].constraint == CvarConstraint::On);
    CHECK(data.cvarRules[0].tier == CvarTier::Queried);
    CHECK(data.cvarRules[5].tier == CvarTier::UserInfo);
}

TEST_CASE("Flags default to off and tier defaults to the queried tier")
{
    const DetectionData data = Parse(FileWith(R"({"name": "a", "constraint": "off"})"));

    REQUIRE(data.cvarRules.size() == 1);
    CHECK(data.cvarRules[0].tier == CvarTier::Queried);
    CHECK_FALSE(data.cvarRules[0].cheatProtected);
    CHECK_FALSE(data.cvarRules[0].kickOnly);
}

TEST_CASE("A misspelled constraint is rejected instead of becoming the first enumerator")
{
    // "minorzero" differs only in case. Mapped generously it would become `equals` with value 64,
    // i.e. "fps_max must be exactly 64" - a detection against every player on the server.
    CHECK_THROWS(Parse(FileWith(R"({"name": "fps_max", "constraint": "minorzero", "value": 64})")));
}

TEST_CASE("A misspelled tier is rejected instead of silently switching tiers")
{
    CHECK_THROWS(Parse(FileWith(R"({"name": "m_yaw", "tier": "userInfo", "constraint": "max", "value": 0.3})")));
}

TEST_CASE("A numeric constraint without its bound is rejected instead of meaning zero")
{
    CHECK_THROWS(Parse(FileWith(R"({"name": "cl_yawspeed", "constraint": "equals"})")));
    CHECK_THROWS(Parse(FileWith(R"({"name": "sensitivity", "constraint": "range", "value": 1})")));
}

TEST_CASE("An inverted range is rejected at parse time")
{
    CHECK_THROWS(Parse(FileWith(R"({"name": "sensitivity", "constraint": "range", "value": 20, "max": 1})")));
}

TEST_CASE("A rule without a usable name is rejected")
{
    CHECK_THROWS(Parse(FileWith(R"({"constraint": "off"})")));
    CHECK_THROWS(Parse(FileWith(R"({"name": "", "constraint": "off"})")));
}

TEST_CASE("An unknown key is a typo, not a preference")
{
    // The likeliest way to disarm a rule by accident: a near-miss on a real key name.
    CHECK_THROWS(Parse(FileWith(R"({"name": "a", "constraint": "max", "value": 1, "kickonly": true})")));
    CHECK_THROWS(nlohmann::json::parse(R"({"dllEventBlacklist": [], "cvarRules": [], "extra": 1})")
                     .get<DetectionData>());
}

TEST_CASE("A renamed section fails the load rather than silently emptying a table")
{
    // This is the case the generous mapping got wrong: an empty table disarms its module, and the
    // load reports success, so the caller keeps no previous table in force.
    CHECK_THROWS(nlohmann::json::parse(R"({"dll_event_blacklist": [], "cvarRules": []})").get<DetectionData>());
    CHECK_THROWS(nlohmann::json::parse(R"({"dllEventBlacklist": []})").get<DetectionData>());
}

TEST_CASE("A section of the wrong shape is rejected")
{
    CHECK_THROWS(nlohmann::json::parse(R"({"dllEventBlacklist": "player_chat", "cvarRules": []})")
                     .get<DetectionData>());
    CHECK_THROWS(Parse(R"("not-an-object")"));
    CHECK_THROWS(Parse(FileWith(R"("not-an-object")")));
}

TEST_CASE("The shipped detections.jsonc parses and loads with nothing rejected")
{
    std::ifstream file(ANTICHEAT_DETECTIONS_JSONC);
    REQUIRE_MESSAGE(file.is_open(), "cannot open " ANTICHEAT_DETECTIONS_JSONC);
    std::ostringstream text;
    text << file.rdbuf();

    DetectionData data;
    // Same call the kit's loader makes, comments and all.
    REQUIRE_NOTHROW(data = nlohmann::json::parse(text.str(), nullptr, true, /*ignore_comments=*/true)
                               .get<DetectionData>());

    CHECK_FALSE(data.dllEventBlacklist.empty());
    CHECK_FALSE(data.cvarRules.empty());

    CvarRuleTable table;
    CHECK(table.Load(data.cvarRules).empty());
    CHECK(table.Size() == data.cvarRules.size());
    // Both tiers must be populated: an empty queried tier stops the poll entirely.
    CHECK(table.Queried().size() > CvarsPerPoll);
    CHECK_FALSE(table.UserInfo().empty());
}

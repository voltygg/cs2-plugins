// The mapping between configs/detections.jsonc and the structs the rule engine consumes. Two of
// these checks now come from the reader itself - an unknown key and an unrecognized token are
// parse errors - and the rest from ValidateDetectionData, which states what a schema cannot:
// that a section is present, that a numeric constraint carries its bound, and that a range's max
// is not below its value. Every case below would otherwise produce a working-looking table.

#include "Core/DetectionData.hpp"
#include "Detectors/InvalidCvarRules.hpp"

#include <VoltMod/Core/Json.hpp>
#include <doctest/doctest.h>
#include <fstream>
#include <sstream>
#include <string>

using Anticheat::CvarConstraint;
using Anticheat::CvarRuleTable;
using Anticheat::CvarsPerPoll;
using Anticheat::CvarTier;
using Anticheat::DetectionData;
using Anticheat::DetectionDocument;

/** The whole load path: read, then validate. */
static VoltMod::Result<DetectionData> Parse(const std::string& text)
{
    auto document = VoltMod::Json::Read<DetectionDocument>(text);
    if (!document)
        return std::unexpected(document.error());
    return Anticheat::ValidateDetectionData(std::move(*document));
}

/** A whole file with @p rules substituted in, since both sections are required. */
static std::string FileWith(const std::string& rules)
{
    return R"({"dllEventBlacklist": ["player_chat"], "cvarRules": [)" + rules + "]}";
}

TEST_CASE("Every tier and constraint spelling the shipped file uses round-trips")
{
    const auto parsed = Parse(FileWith(R"(
        {"name": "a", "tier": "queried",  "constraint": "equals",    "value": 89},
        {"name": "b", "tier": "queried",  "constraint": "max",       "value": 1},
        {"name": "c", "tier": "queried",  "constraint": "range",     "value": 1, "max": 20},
        {"name": "d", "tier": "queried",  "constraint": "minOrZero", "value": 64},
        {"name": "e", "tier": "queried",  "constraint": "off"},
        {"name": "f", "tier": "userinfo", "constraint": "on"}
    )"));
    REQUIRE(parsed.has_value());

    const auto& rules = parsed->cvarRules;
    REQUIRE(rules.size() == 6);
    CHECK(rules[0].constraint == CvarConstraint::Equals);
    CHECK(rules[1].constraint == CvarConstraint::Max);
    CHECK(rules[2].constraint == CvarConstraint::Range);
    CHECK(rules[3].constraint == CvarConstraint::MinOrZero);
    CHECK(rules[4].constraint == CvarConstraint::Off);
    CHECK(rules[5].constraint == CvarConstraint::On);
    CHECK(rules[0].tier == CvarTier::Queried);
    CHECK(rules[5].tier == CvarTier::UserInfo);
}

TEST_CASE("Flags default to off and tier defaults to the queried tier")
{
    const auto parsed = Parse(FileWith(R"({"name": "a", "constraint": "off"})"));
    REQUIRE(parsed.has_value());

    REQUIRE(parsed->cvarRules.size() == 1);
    CHECK(parsed->cvarRules[0].tier == CvarTier::Queried);
    CHECK_FALSE(parsed->cvarRules[0].cheatProtected);
    CHECK_FALSE(parsed->cvarRules[0].kickOnly);
}

TEST_CASE("A misspelled constraint is rejected instead of becoming the first enumerator")
{
    // "minorzero" differs only in case. Mapped generously it would become `equals` with value 64,
    // i.e. "fps_max must be exactly 64" - a detection against every player on the server.
    CHECK_FALSE(Parse(FileWith(R"({"name": "fps_max", "constraint": "minorzero", "value": 64})")).has_value());
}

TEST_CASE("A misspelled tier is rejected instead of silently switching tiers")
{
    CHECK_FALSE(
        Parse(FileWith(R"({"name": "m_yaw", "tier": "userInfo", "constraint": "max", "value": 0.3})")).has_value());
}

TEST_CASE("A numeric constraint without its bound is rejected instead of meaning zero")
{
    const auto noValue = Parse(FileWith(R"({"name": "cl_yawspeed", "constraint": "equals"})"));
    REQUIRE_FALSE(noValue.has_value());
    CHECK(noValue.error().Detail.find("cl_yawspeed") != std::string::npos);

    const auto noMax = Parse(FileWith(R"({"name": "sensitivity", "constraint": "range", "value": 1})"));
    REQUIRE_FALSE(noMax.has_value());
    CHECK(noMax.error().Detail.find("max") != std::string::npos);
}

TEST_CASE("An inverted range is rejected at load time")
{
    const auto parsed = Parse(FileWith(R"({"name": "sensitivity", "constraint": "range", "value": 20, "max": 1})"));
    REQUIRE_FALSE(parsed.has_value());
    CHECK(parsed.error().Detail.find("below its value") != std::string::npos);
}

TEST_CASE("A rule without a usable name or constraint is rejected")
{
    CHECK_FALSE(Parse(FileWith(R"({"constraint": "off"})")).has_value());
    CHECK_FALSE(Parse(FileWith(R"({"name": "", "constraint": "off"})")).has_value());
    CHECK_FALSE(Parse(FileWith(R"({"name": "a"})")).has_value());
}

TEST_CASE("An unknown key is a typo, not a preference")
{
    // The likeliest way to disarm a rule by accident: a near-miss on a real key name.
    CHECK_FALSE(Parse(FileWith(R"({"name": "a", "constraint": "max", "value": 1, "kickonly": true})")).has_value());
    CHECK_FALSE(Parse(R"({"dllEventBlacklist": [], "cvarRules": [], "extra": 1})").has_value());
}

TEST_CASE("A renamed section fails the load rather than silently emptying a table")
{
    // An empty table disarms its module, and a load that reported success would leave no previous
    // table in force. A renamed section is an unknown key *and* a missing one.
    CHECK_FALSE(Parse(R"({"dll_event_blacklist": [], "cvarRules": []})").has_value());

    const auto missing = Parse(R"({"dllEventBlacklist": []})");
    REQUIRE_FALSE(missing.has_value());
    CHECK(missing.error().Detail.find("cvarRules") != std::string::npos);
}

TEST_CASE("A section of the wrong shape is rejected")
{
    CHECK_FALSE(Parse(R"({"dllEventBlacklist": "player_chat", "cvarRules": []})").has_value());
    CHECK_FALSE(Parse(R"("not-an-object")").has_value());
    CHECK_FALSE(Parse(FileWith(R"("not-an-object")")).has_value());
}

TEST_CASE("The shipped detections.jsonc parses and loads with nothing rejected")
{
    std::ifstream file(ANTICHEAT_DETECTIONS_JSONC, std::ios::binary);
    REQUIRE_MESSAGE(file.is_open(), "cannot open " ANTICHEAT_DETECTIONS_JSONC);
    std::ostringstream text;
    text << file.rdbuf();

    // Same loader call, comments and all - the file opens with a comment block before its brace.
    const auto parsed = Parse(text.str());
    const std::string why = parsed.has_value() ? std::string{} : parsed.error().Detail;
    REQUIRE_MESSAGE(parsed.has_value(), why);

    CHECK_FALSE(parsed->dllEventBlacklist.empty());
    CHECK_FALSE(parsed->cvarRules.empty());

    CvarRuleTable table;
    CHECK(table.Load(parsed->cvarRules).empty());
    CHECK(table.Size() == parsed->cvarRules.size());
    // Both tiers must be populated: an empty queried tier stops the poll entirely.
    CHECK(table.Queried().size() > CvarsPerPoll);
    CHECK_FALSE(table.UserInfo().empty());
}

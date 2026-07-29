#include "Detectors/InvalidCvarRules.hpp"

#include <doctest/doctest.h>
#include <vector>

using namespace Anticheat;

namespace
{
constexpr int Slot = 2;
constexpr bool Enforcing = true;
constexpr bool NotEnforcing = false;

/**
 * The rules the plugin ships in configs/detections.jsonc. The engine is what these cases test, so
 * the table is pinned here rather than read from the file - a deployment edit to the data must not
 * be able to turn a rule test green.
 */
std::vector<CvarRule> ShippedRules()
{
    using enum CvarConstraint;
    return {
        {.name = "fps_max", .constraint = MinOrZero, .value = 64.0, .kickOnly = true},
        {.name = "sv_cheats", .constraint = Off, .cheatProtected = true},
        {.name = "cl_showpos", .constraint = Off, .cheatProtected = true},
        {.name = "cam_showangles", .constraint = Off, .cheatProtected = true},
        {.name = "cl_drawhud", .constraint = On, .cheatProtected = true},
        {.name = "cl_pitchdown", .constraint = Equals, .value = 89.0},
        {.name = "cl_pitchup", .constraint = Equals, .value = 89.0},
        {.name = "cl_yawspeed", .constraint = Equals, .value = 210.0},
        {.name = "fov_cs_debug", .constraint = Equals, .value = 0.0, .cheatProtected = true},
        {.name = "sensitivity", .tier = CvarTier::UserInfo, .constraint = Range, .value = 0.0001, .max = 20.0},
        {.name = "m_yaw", .tier = CvarTier::UserInfo, .constraint = Max, .value = 0.3, .kickOnly = true},
    };
}

const CvarRuleTable& Rules()
{
    static const CvarRuleTable table = [] {
        CvarRuleTable loaded;
        loaded.Load(ShippedRules());
        return loaded;
    }();
    return table;
}

InvalidCvarRules MakeRules()
{
    InvalidCvarRules rules;
    rules.LoadRules(ShippedRules());
    return rules;
}

CvarVerdict EvaluateCvar(std::string_view name, std::string_view value, bool enforcing)
{
    return Rules().Evaluate(name, value, enforcing);
}

CvarVerdict EvaluateMissingCvar(std::string_view name, std::string_view statusName, bool enforcing, int replies)
{
    return Rules().EvaluateMissing(name, statusName, enforcing, replies);
}

bool Rejects(std::string_view name, std::string_view value, bool enforcing = Enforcing)
{
    const CvarVerdict verdict = EvaluateCvar(name, value, enforcing);
    return verdict.Checked && verdict.Invalid;
}

bool Accepts(std::string_view name, std::string_view value, bool enforcing = Enforcing)
{
    const CvarVerdict verdict = EvaluateCvar(name, value, enforcing);
    return verdict.Checked && !verdict.Invalid;
}
}  // namespace

TEST_CASE("m_yaw must be a number at or below three tenths and only ever costs a kick")
{
    CHECK(Accepts("m_yaw", "0.022"));
    CHECK(Accepts("m_yaw", "0.3"));
    CHECK(Rejects("m_yaw", "0.31"));
    CHECK(Rejects("m_yaw", "not-a-number"));
    CHECK(EvaluateCvar("m_yaw", "0.31", Enforcing).KickOnly);
    // A non-numeric reply is a client fault rather than a tuned advantage, so it is not kick-only.
    CHECK_FALSE(EvaluateCvar("m_yaw", "garbage", Enforcing).KickOnly);
}

TEST_CASE("fps_max must be unlimited or at least the server tick rate and only ever costs a kick")
{
    CHECK(Accepts("fps_max", "0"));
    CHECK(Accepts("fps_max", "64"));
    CHECK(Accepts("fps_max", "400"));
    CHECK(Rejects("fps_max", "63"));
    CHECK(Rejects("fps_max", "30"));
    CHECK(Rejects("fps_max", ""));
    CHECK(EvaluateCvar("fps_max", "30", Enforcing).KickOnly);
}

TEST_CASE("sensitivity must stay inside the engine range")
{
    CHECK(Accepts("sensitivity", "2.5"));
    CHECK(Accepts("sensitivity", "0.0001"));
    CHECK(Accepts("sensitivity", "20"));
    CHECK(Rejects("sensitivity", "0"));
    CHECK(Rejects("sensitivity", "20.1"));
    CHECK(Rejects("sensitivity", "abc"));
    CHECK_FALSE(EvaluateCvar("sensitivity", "0", Enforcing).KickOnly);
}

TEST_CASE("The pitch limits and the yaw speed must hold their engine defaults")
{
    CHECK(Accepts("cl_pitchdown", "89"));
    CHECK(Accepts("cl_pitchup", "89.0"));
    CHECK(Rejects("cl_pitchdown", "180"));
    CHECK(Rejects("cl_pitchup", "0"));
    CHECK(Rejects("cl_pitchdown", "nope"));
    CHECK(Accepts("cl_yawspeed", "210"));
    CHECK(Rejects("cl_yawspeed", "500"));
}

TEST_CASE("Cheat protected cvars are judged only while the server is enforcing them")
{
    CHECK(Rejects("sv_cheats", "1"));
    CHECK(Accepts("sv_cheats", "0"));
    CHECK(Accepts("sv_cheats", "false"));
    CHECK(Rejects("cl_showpos", "1"));
    CHECK(Accepts("cl_showpos", "0"));
    CHECK(Rejects("cam_showangles", "1"));
    CHECK(Rejects("cl_drawhud", "0"));
    CHECK(Accepts("cl_drawhud", "1"));
    CHECK(Rejects("fov_cs_debug", "90"));
    CHECK(Accepts("fov_cs_debug", "0"));

    for (std::string_view name : {"sv_cheats", "cl_showpos", "cam_showangles", "cl_drawhud", "fov_cs_debug"})
        CHECK_FALSE(EvaluateCvar(name, "1", NotEnforcing).Checked);
}

TEST_CASE("Client controlled cvars are judged even while cheat protected ones are not")
{
    CHECK(Rejects("m_yaw", "0.31", NotEnforcing));
    CHECK(Rejects("fps_max", "30", NotEnforcing));
    CHECK(Rejects("sensitivity", "0", NotEnforcing));
    CHECK(Rejects("cl_yawspeed", "500", NotEnforcing));
}

TEST_CASE("Cvar names are matched without regard to case and unknown names are not covered")
{
    CHECK(Rejects("M_YAW", "0.31"));
    CHECK(Accepts("Sensitivity", "2.5"));
    CHECK_FALSE(EvaluateCvar("cl_interp_ratio", "2", Enforcing).Known);
    CHECK_FALSE(EvaluateCvar("cl_interp_ratio", "2", Enforcing).Checked);
}

TEST_CASE("Cheat cvar enforcement waits out the propagation grace after sv_cheats goes off")
{
    constexpr double now = 1000.0;
    const double graceUntil = now + SvCheatsPropagationGraceSec;
    CHECK_FALSE(ShouldEnforceCheatCvars(false, now, graceUntil));
    CHECK_FALSE(ShouldEnforceCheatCvars(false, graceUntil, graceUntil));
    CHECK(ShouldEnforceCheatCvars(false, graceUntil + 0.1, graceUntil));
    // While sv_cheats is on, nothing is enforced no matter how long ago the grace expired.
    CHECK_FALSE(ShouldEnforceCheatCvars(true, graceUntil + 1000.0, graceUntil));
}

TEST_CASE("A cvar that stays invalid reports once")
{
    InvalidCvarRules rules = MakeRules();
    const std::optional<Finding> first = rules.Observe(Slot, "m_yaw", "0.5", Enforcing);
    REQUIRE(first.has_value());
    CHECK(first->Kind == DetectionKind::InvalidCvar);
    CHECK(first->KickOnly);
    CHECK(rules.IsLatched(Slot, "m_yaw"));

    CHECK_FALSE(rules.Observe(Slot, "m_yaw", "0.5", Enforcing).has_value());
    CHECK_FALSE(rules.Observe(Slot, "m_yaw", "0.6", Enforcing).has_value());
}

TEST_CASE("A cvar that returns to a valid value re-arms the latch")
{
    InvalidCvarRules rules = MakeRules();
    REQUIRE(rules.Observe(Slot, "m_yaw", "0.5", Enforcing).has_value());
    CHECK_FALSE(rules.Observe(Slot, "m_yaw", "0.022", Enforcing).has_value());
    CHECK_FALSE(rules.IsLatched(Slot, "m_yaw"));
    CHECK(rules.Observe(Slot, "m_yaw", "0.5", Enforcing).has_value());
}

TEST_CASE("A skipped cheat protected rule leaves the latch untouched")
{
    InvalidCvarRules rules = MakeRules();
    REQUIRE(rules.Observe(Slot, "cl_showpos", "1", Enforcing).has_value());
    CHECK(rules.IsLatched(Slot, "cl_showpos"));
    // Enforcement stopping must not silently clear the latch and let the next reply fire again.
    CHECK_FALSE(rules.Observe(Slot, "cl_showpos", "0", NotEnforcing).has_value());
    CHECK(rules.IsLatched(Slot, "cl_showpos"));
}

TEST_CASE("Each cvar latches independently and a slot change clears them all")
{
    InvalidCvarRules rules = MakeRules();
    REQUIRE(rules.Observe(Slot, "m_yaw", "0.5", Enforcing).has_value());
    REQUIRE(rules.Observe(Slot, "cl_yawspeed", "500", Enforcing).has_value());
    CHECK(rules.IsLatched(Slot, "m_yaw"));
    CHECK(rules.IsLatched(Slot, "cl_yawspeed"));

    rules.OnSlotChanged(Slot);
    CHECK_FALSE(rules.IsLatched(Slot, "m_yaw"));
    CHECK_FALSE(rules.IsLatched(Slot, "cl_yawspeed"));
    CHECK(rules.Observe(Slot, "m_yaw", "0.5", Enforcing).has_value());
}

TEST_CASE("An unknown cvar name never produces a finding")
{
    InvalidCvarRules rules = MakeRules();
    CHECK_FALSE(rules.Observe(Slot, "cl_interp_ratio", "99999", Enforcing).has_value());
    CHECK_FALSE(rules.Observe(-1, "m_yaw", "0.5", Enforcing).has_value());
}

TEST_CASE("Every cvar the two tiers read is covered by the rule table")
{
    REQUIRE_FALSE(Rules().Empty());
    for (const CvarRule& rule : Rules().All())
        CHECK(EvaluateCvar(rule.name, "0", Enforcing).Known);
    CHECK(Rules().Queried().size() + Rules().UserInfo().size() == Rules().Size());
}

TEST_CASE("A cvar belongs to one tier alone, so one latch never has two sources")
{
    for (std::string_view userInfo : Rules().UserInfo())
        for (std::string_view queried : Rules().Queried())
            CHECK(userInfo != queried);
}

TEST_CASE("A second rule for a cvar already in the table is rejected rather than sharing its latch")
{
    CvarRuleTable table;
    CHECK(table.Load({
              {.name = "m_yaw", .constraint = CvarConstraint::Max, .value = 0.3},
              {.name = "M_YAW", .tier = CvarTier::UserInfo, .constraint = CvarConstraint::Max, .value = 9.0},
          }) == 1);
    REQUIRE(table.Size() == 1);
    // The first rule wins, so a duplicate cannot loosen one already in force.
    CHECK(table.Evaluate("m_yaw", "5", Enforcing).Invalid);
}

TEST_CASE("Malformed rules are dropped and the rest of the table still loads")
{
    CvarRuleTable table;
    CHECK(table.Load({
              {.name = "", .constraint = CvarConstraint::Equals},
              {.name = "sensitivity", .constraint = CvarConstraint::Range, .value = 20.0, .max = 1.0},
              {.name = "cl_yawspeed", .constraint = CvarConstraint::Equals, .value = 210.0},
          }) == 2);
    CHECK(table.Size() == 1);
    CHECK(table.Evaluate("cl_yawspeed", "210", Enforcing).Checked);
}

TEST_CASE("An empty rule table judges nothing at all")
{
    const CvarRuleTable table;
    CHECK(table.Empty());
    CHECK_FALSE(table.Evaluate("m_yaw", "9999", Enforcing).Known);
    CHECK_FALSE(table.EvaluateMissing("sv_cheats", "cvar_protected", Enforcing, 99).Known);
    // The poll rotation must stay in range rather than divide by zero.
    CHECK(table.PollCvarIndex(0, 1) == 0);
}

TEST_CASE("A client that withholds a cheat protected cvar is judged, but only ever for a kick")
{
    for (std::string_view name : {"sv_cheats", "cl_showpos", "cam_showangles", "cl_drawhud", "fov_cs_debug"})
    {
        const CvarVerdict verdict =
            EvaluateMissingCvar(name, "cvar_not_found", Enforcing, MissingRepliesBeforeEvidence);
        CHECK(verdict.Checked);
        CHECK(verdict.Invalid);
        CHECK(verdict.KickOnly);
    }
    CHECK(EvaluateMissingCvar("cl_drawhud", "cvar_protected", Enforcing, MissingRepliesBeforeEvidence).Invalid);
}

TEST_CASE("A cheat protected cvar withheld only once or twice is not yet evidence")
{
    for (int replies = 1; replies < MissingRepliesBeforeEvidence; ++replies)
        CHECK_FALSE(EvaluateMissingCvar("cl_showpos", "cvar_not_found", Enforcing, replies).Checked);
}

TEST_CASE("A withheld cvar is not judged while the cheat rules are not being enforced")
{
    for (std::string_view name : {"sv_cheats", "cl_showpos", "cl_drawhud"})
        CHECK_FALSE(EvaluateMissingCvar(name, "cvar_not_found", NotEnforcing, MissingRepliesBeforeEvidence).Checked);
}

TEST_CASE("A withheld client tunable cvar is no signal at all")
{
    // An engine update that drops one of these must not turn every connected client into a detection.
    for (std::string_view name : {"m_yaw", "fps_max", "sensitivity", "cl_pitchdown", "cl_pitchup", "cl_yawspeed"})
        CHECK_FALSE(EvaluateMissingCvar(name, "cvar_not_found", Enforcing, MissingRepliesBeforeEvidence).Checked);
    CHECK_FALSE(EvaluateMissingCvar("cl_interp_ratio", "not_a_cvar", Enforcing, MissingRepliesBeforeEvidence).Known);
}

TEST_CASE("Only the third refusal in a row for one cvar becomes a finding")
{
    InvalidCvarRules rules = MakeRules();
    CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK_FALSE(rules.IsLatched(Slot, "cl_showpos"));
    CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK(rules.IsLatched(Slot, "cl_showpos"));
}

TEST_CASE("A reply that carries a value restarts the run of refusals")
{
    InvalidCvarRules rules = MakeRules();
    REQUIRE_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    REQUIRE_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK_FALSE(rules.Observe(Slot, "cl_showpos", "0", Enforcing).has_value());

    // The count restarts, so two more refusals are still not enough.
    CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
}

TEST_CASE("Refusals of different cvars are counted apart")
{
    InvalidCvarRules rules = MakeRules();
    // Interleaved refusals, one short of the threshold for each cvar.
    for (int reply = 1; reply < MissingRepliesBeforeEvidence; ++reply)
    {
        CHECK_FALSE(rules.ObserveMissing(Slot, "cl_drawhud", "cvar_not_found", Enforcing).has_value());
        CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    }
    // The next refusal reports for the cvar that received it, and only for that one.
    CHECK(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK_FALSE(rules.IsLatched(Slot, "cl_drawhud"));
}

TEST_CASE("A withheld cvar latches like an invalid value and shares its latch")
{
    InvalidCvarRules rules = MakeRules();
    for (int reply = 1; reply < MissingRepliesBeforeEvidence; ++reply)
        REQUIRE_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    REQUIRE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());
    CHECK(rules.IsLatched(Slot, "cl_showpos"));
    CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_not_found", Enforcing).has_value());

    // A later reply carrying a valid value re-arms the latch and restarts the run, so the refusal
    // threshold must be met again.
    CHECK_FALSE(rules.Observe(Slot, "cl_showpos", "0", Enforcing).has_value());
    CHECK_FALSE(rules.IsLatched(Slot, "cl_showpos"));
    for (int reply = 1; reply < MissingRepliesBeforeEvidence; ++reply)
        CHECK_FALSE(rules.ObserveMissing(Slot, "cl_showpos", "cvar_protected", Enforcing).has_value());
    CHECK(rules.ObserveMissing(Slot, "cl_showpos", "cvar_protected", Enforcing).has_value());
}

TEST_CASE("Successive polls walk the whole cvar table without repeating within a lap")
{
    const size_t total = Rules().Queried().size();
    REQUIRE(total > CvarsPerPoll);
    std::vector<int> seen(total, 0);
    size_t cursor = 0;
    for (size_t poll = 0; poll * CvarsPerPoll < total; ++poll)
    {
        for (size_t offset = 0; offset < CvarsPerPoll; ++offset)
            ++seen[Rules().PollCvarIndex(cursor, offset)];
        cursor = Rules().PollCvarIndex(cursor, CvarsPerPoll);
    }
    for (int count : seen)
        CHECK(count >= 1);
    CHECK(Rules().PollCvarIndex(total - 1, 1) == 0);
}

TEST_CASE("The randomized poll delay stays inside the integrity check interval")
{
    CHECK(PollDelaySec(0.0) == doctest::Approx(PollIntervalMinSec));
    CHECK(PollDelaySec(1.0) == doctest::Approx(PollIntervalMaxSec));
    CHECK(PollDelaySec(0.5) == doctest::Approx(3.0));
    // std::generate_canonical is documented as [0, 1) but is famously allowed to return 1.0.
    CHECK(PollDelaySec(1.5) == doctest::Approx(PollIntervalMaxSec));
    CHECK(PollDelaySec(-0.5) == doctest::Approx(PollIntervalMinSec));
}

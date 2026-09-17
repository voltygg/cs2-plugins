#include "Detect/Finding.hpp"
#include "Response/ResponsePolicy.hpp"

#include <algorithm>
#include <doctest/doctest.h>
#include <iterator>
#include <string_view>

using Anticheat::Decide;
using Anticheat::DetectionCatalog;
using Anticheat::DetectionInfo;
using Anticheat::IssuedPunishments;
using Anticheat::MaxSlots;
using Anticheat::Mode;
using Anticheat::ParseMode;
using Anticheat::PunishmentLevel;
using Anticheat::ResponseDecision;
using Anticheat::ResponseInput;
using Anticheat::ResponseOutcome;

static constexpr int64_t SteamId = 76561198000000000LL;

static ResponseInput Input(Mode mode, bool kickOnly = false, PunishmentLevel issued = PunishmentLevel::None)
{
    return {.SteamId = SteamId, .Whitelisted = false, .CurrentMode = mode, .KickOnly = kickOnly, .Issued = issued};
}

TEST_CASE("Every detection has a distinct display name and a console token free of whitespace")
{
    for (const DetectionInfo& detection : DetectionCatalog)
    {
        CHECK_FALSE(std::string_view(detection.Display).empty());
        const std::string_view token = detection.Token;
        CHECK_FALSE(token.empty());
        // The admin-system bridge takes the detection as one console argument.
        CHECK(token.find(' ') == std::string_view::npos);

        // The catalog is what DisplayName/TokenName answer from, for every kind.
        CHECK(std::string_view(DisplayName(detection.Kind)) == detection.Display);
        CHECK(std::string_view(TokenName(detection.Kind)) == token);

        const auto sameToken = [&](const DetectionInfo& other) {
            return &other != &detection && std::string_view(other.Token) == token;
        };
        CHECK(std::none_of(std::begin(DetectionCatalog), std::end(DetectionCatalog), sameToken));
    }
}

TEST_CASE("ParseMode reads a mode in any casing and falls back to observe otherwise")
{
    CHECK(ParseMode("observe") == Mode::Observe);
    CHECK(ParseMode("alert") == Mode::Alert);
    CHECK(ParseMode("ban") == Mode::Ban);
    CHECK(ParseMode("") == Mode::Observe);
    CHECK(ParseMode("nonsense") == Mode::Observe);
    // Case-insensitive on purpose: a mistyped mode must not silently disable enforcement.
    CHECK(ParseMode("BAN") == Mode::Ban);
    CHECK(ParseMode("Alert") == Mode::Alert);
}

TEST_CASE("A detection without a resolved SteamID is reported but never punished")
{
    ResponseInput input = Input(Mode::Ban);
    input.SteamId = 0;
    const ResponseDecision decision = Decide(input);
    CHECK(decision.Outcome == ResponseOutcome::NoIdentity);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("A whitelisted player is reported but never punished or alerted on")
{
    ResponseInput input = Input(Mode::Ban);
    input.Whitelisted = true;
    const ResponseDecision decision = Decide(input);
    CHECK(decision.Outcome == ResponseOutcome::Whitelisted);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Observe mode stops before the alert")
{
    const ResponseDecision decision = Decide(Input(Mode::Observe));
    CHECK(decision.Outcome == ResponseOutcome::Observed);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Alert mode alerts without punishing")
{
    const ResponseDecision decision = Decide(Input(Mode::Alert));
    CHECK(decision.Outcome == ResponseOutcome::Alerted);
    CHECK(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Alert mode ignores the kick-only flag entirely")
{
    const ResponseDecision decision = Decide(Input(Mode::Alert, true));
    CHECK(decision.Outcome == ResponseOutcome::Alerted);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Ban mode bans a normal finding and only kicks a kick-only one")
{
    const ResponseDecision banned = Decide(Input(Mode::Ban, false));
    CHECK(banned.Outcome == ResponseOutcome::BanIssued);
    CHECK(banned.SendAlert);
    CHECK(banned.Apply == PunishmentLevel::Ban);

    const ResponseDecision kicked = Decide(Input(Mode::Ban, true));
    CHECK(kicked.Outcome == ResponseOutcome::KickIssued);
    CHECK(kicked.Apply == PunishmentLevel::Kick);
}

TEST_CASE("A punishment already issued is never repeated or downgraded, but may be raised")
{
    const ResponseDecision kick = Decide(Input(Mode::Ban, true, PunishmentLevel::Kick));
    CHECK(kick.Outcome == ResponseOutcome::AlreadyPunished);
    CHECK(kick.Apply == PunishmentLevel::None);

    const ResponseDecision ban = Decide(Input(Mode::Ban, false, PunishmentLevel::Ban));
    CHECK(ban.Outcome == ResponseOutcome::AlreadyPunished);
    CHECK(ban.Apply == PunishmentLevel::None);

    // A kick-only finding against an already-banned player must not walk the ban back.
    const ResponseDecision downgrade = Decide(Input(Mode::Ban, true, PunishmentLevel::Ban));
    CHECK(downgrade.Outcome == ResponseOutcome::AlreadyPunished);
    CHECK(downgrade.Apply == PunishmentLevel::None);

    const ResponseDecision upgrade = Decide(Input(Mode::Ban, false, PunishmentLevel::Kick));
    CHECK(upgrade.Outcome == ResponseOutcome::BanIssued);
    CHECK(upgrade.Apply == PunishmentLevel::Ban);
}

TEST_CASE("IssuedPunishments only ever raises a slot's level")
{
    IssuedPunishments issued;
    CHECK(issued.Level(3) == PunishmentLevel::None);
    CHECK(issued.Raise(3, PunishmentLevel::Kick));
    CHECK(issued.Level(3) == PunishmentLevel::Kick);
    CHECK_FALSE(issued.Raise(3, PunishmentLevel::Kick));
    CHECK(issued.Raise(3, PunishmentLevel::Ban));
    CHECK_FALSE(issued.Raise(3, PunishmentLevel::Kick));
    CHECK(issued.Level(3) == PunishmentLevel::Ban);
}

TEST_CASE("IssuedPunishments clears one slot and resets every slot")
{
    IssuedPunishments issued;
    issued.Raise(1, PunishmentLevel::Ban);
    issued.Raise(2, PunishmentLevel::Kick);
    issued.Clear(1);
    CHECK(issued.Level(1) == PunishmentLevel::None);
    CHECK(issued.Level(2) == PunishmentLevel::Kick);
    issued.Reset();
    CHECK(issued.Level(2) == PunishmentLevel::None);
}

TEST_CASE("IssuedPunishments ignores out of range slots instead of writing past its array")
{
    IssuedPunishments issued;
    CHECK_FALSE(issued.Raise(-1, PunishmentLevel::Ban));
    CHECK_FALSE(issued.Raise(MaxSlots, PunishmentLevel::Ban));
    CHECK(issued.Level(-1) == PunishmentLevel::None);
}

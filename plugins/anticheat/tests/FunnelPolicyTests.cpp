#include "Core/Finding.hpp"
#include "Response/FunnelPolicy.hpp"

#include <algorithm>
#include <doctest/doctest.h>
#include <iterator>
#include <string_view>

using namespace Anticheat;

namespace
{
constexpr int64_t SteamId = 76561198000000000LL;

FunnelInput Input(Mode mode, bool kickOnly = false, PunishmentLevel issued = PunishmentLevel::None)
{
    return {.SteamId = SteamId, .Whitelisted = false, .CurrentMode = mode, .KickOnly = kickOnly, .Issued = issued};
}
}  // namespace

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

TEST_CASE("ParseMode falls back to observe for anything it does not recognise")
{
    CHECK(ParseMode("observe") == Mode::Observe);
    CHECK(ParseMode("alert") == Mode::Alert);
    CHECK(ParseMode("ban") == Mode::Ban);
    CHECK(ParseMode("") == Mode::Observe);
    CHECK(ParseMode("BAN") == Mode::Observe);
}

TEST_CASE("A detection without a resolved SteamID is reported but never punished")
{
    FunnelInput input = Input(Mode::Ban);
    input.SteamId = 0;
    const FunnelDecision decision = Decide(input);
    CHECK(decision.Outcome == FunnelOutcome::NoIdentity);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("A whitelisted player is reported but never punished or alerted on")
{
    FunnelInput input = Input(Mode::Ban);
    input.Whitelisted = true;
    const FunnelDecision decision = Decide(input);
    CHECK(decision.Outcome == FunnelOutcome::Whitelisted);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Observe mode stops before the alert")
{
    const FunnelDecision decision = Decide(Input(Mode::Observe));
    CHECK(decision.Outcome == FunnelOutcome::Observed);
    CHECK_FALSE(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Alert mode alerts without punishing")
{
    const FunnelDecision decision = Decide(Input(Mode::Alert));
    CHECK(decision.Outcome == FunnelOutcome::Alerted);
    CHECK(decision.SendAlert);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Alert mode ignores the kick-only flag entirely")
{
    const FunnelDecision decision = Decide(Input(Mode::Alert, true));
    CHECK(decision.Outcome == FunnelOutcome::Alerted);
    CHECK(decision.Apply == PunishmentLevel::None);
}

TEST_CASE("Ban mode bans a normal finding and only kicks a kick-only one")
{
    const FunnelDecision banned = Decide(Input(Mode::Ban, false));
    CHECK(banned.Outcome == FunnelOutcome::BanIssued);
    CHECK(banned.SendAlert);
    CHECK(banned.Apply == PunishmentLevel::Ban);

    const FunnelDecision kicked = Decide(Input(Mode::Ban, true));
    CHECK(kicked.Outcome == FunnelOutcome::KickIssued);
    CHECK(kicked.Apply == PunishmentLevel::Kick);
}

TEST_CASE("A punishment already issued is never repeated at the same level")
{
    const FunnelDecision kick = Decide(Input(Mode::Ban, true, PunishmentLevel::Kick));
    CHECK(kick.Outcome == FunnelOutcome::AlreadyPunished);
    CHECK(kick.Apply == PunishmentLevel::None);

    const FunnelDecision ban = Decide(Input(Mode::Ban, false, PunishmentLevel::Ban));
    CHECK(ban.Outcome == FunnelOutcome::AlreadyPunished);
    CHECK(ban.Apply == PunishmentLevel::None);
}

TEST_CASE("A banned player is never downgraded to a kick but a kicked one can still be banned")
{
    const FunnelDecision downgrade = Decide(Input(Mode::Ban, true, PunishmentLevel::Ban));
    CHECK(downgrade.Outcome == FunnelOutcome::AlreadyPunished);
    CHECK(downgrade.Apply == PunishmentLevel::None);

    const FunnelDecision upgrade = Decide(Input(Mode::Ban, false, PunishmentLevel::Kick));
    CHECK(upgrade.Outcome == FunnelOutcome::BanIssued);
    CHECK(upgrade.Apply == PunishmentLevel::Ban);
}

TEST_CASE("PunishmentLatch only ever raises a slot's level")
{
    PunishmentLatch latch;
    CHECK(latch.Level(3) == PunishmentLevel::None);
    CHECK(latch.Raise(3, PunishmentLevel::Kick));
    CHECK(latch.Level(3) == PunishmentLevel::Kick);
    CHECK_FALSE(latch.Raise(3, PunishmentLevel::Kick));
    CHECK(latch.Raise(3, PunishmentLevel::Ban));
    CHECK_FALSE(latch.Raise(3, PunishmentLevel::Kick));
    CHECK(latch.Level(3) == PunishmentLevel::Ban);
}

TEST_CASE("PunishmentLatch clears one slot and resets every slot")
{
    PunishmentLatch latch;
    latch.Raise(1, PunishmentLevel::Ban);
    latch.Raise(2, PunishmentLevel::Kick);
    latch.Clear(1);
    CHECK(latch.Level(1) == PunishmentLevel::None);
    CHECK(latch.Level(2) == PunishmentLevel::Kick);
    latch.Reset();
    CHECK(latch.Level(2) == PunishmentLevel::None);
}

TEST_CASE("PunishmentLatch ignores out of range slots instead of writing past its array")
{
    PunishmentLatch latch;
    CHECK_FALSE(latch.Raise(-1, PunishmentLevel::Ban));
    CHECK_FALSE(latch.Raise(MaxSlots, PunishmentLevel::Ban));
    CHECK(latch.Level(-1) == PunishmentLevel::None);
}

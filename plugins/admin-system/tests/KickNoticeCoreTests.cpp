#include "Punishments/KickNoticeCore.hpp"

#include <doctest/doctest.h>

using AdminSystem::Punishments::BanNoticeParts;
using AdminSystem::Punishments::JoinBanNotice;

TEST_CASE("JoinBanNotice renders every configured piece")
{
    BanNoticeParts parts{.Reason = "Cheating", .Expiry = "expires in 2h", .Appeal = "Appeal: https://x/a"};
    CHECK_EQ(JoinBanNotice(parts), std::string("Cheating | expires in 2h | Appeal: https://x/a"));
}

TEST_CASE("JoinBanNotice drops an unconfigured appeal without a dangling separator")
{
    BanNoticeParts parts{.Reason = "Cheating", .Expiry = "Permanent ban", .Appeal = ""};
    CHECK_EQ(JoinBanNotice(parts), std::string("Cheating | Permanent ban"));
}

TEST_CASE("JoinBanNotice drops a suppressed expiry from the middle")
{
    BanNoticeParts parts{.Reason = "Cheating", .Expiry = "", .Appeal = "Appeal: https://x/a"};
    CHECK_EQ(JoinBanNotice(parts), std::string("Cheating | Appeal: https://x/a"));
}

TEST_CASE("JoinBanNotice degrades to the bare reason when nothing else is configured")
{
    CHECK_EQ(JoinBanNotice({.Reason = "Cheating"}), std::string("Cheating"));
}

TEST_CASE("JoinBanNotice tolerates an empty reason")
{
    // A ban row with no reason still has to produce a readable notice rather than a lone separator.
    CHECK_EQ(JoinBanNotice({.Reason = "", .Expiry = "Permanent ban"}), std::string("Permanent ban"));
    CHECK_EQ(JoinBanNotice({}), std::string(""));
}

TEST_CASE("JoinBanNotice honours a custom separator")
{
    BanNoticeParts parts{.Reason = "Cheating", .Expiry = "Permanent ban"};
    CHECK_EQ(JoinBanNotice(parts, " - "), std::string("Cheating - Permanent ban"));
}

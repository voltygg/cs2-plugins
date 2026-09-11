#include "Maps/VoteMath.hpp"

#include <doctest/doctest.h>

using AdminSystem::Maps::VotePassed;
using AdminSystem::Maps::VoteThreshold;

TEST_CASE("VoteThreshold needs a strict majority of the configured share")
{
    // 60% of 10 is 6, and one more makes it a majority of that share rather than a tie.
    CHECK_EQ(VoteThreshold(10, 0.6), 7u);
    CHECK_EQ(VoteThreshold(5, 0.6), 4u);
    CHECK_EQ(VoteThreshold(1, 0.6), 1u);
    CHECK_EQ(VoteThreshold(0, 0.6), 0u);
}

TEST_CASE("VoteThreshold stays between one ballot and every ballot cast")
{
    // Otherwise a unanimous vote could sit at a threshold nobody can reach, or a mis-set ratio
    // could make the vote free.
    CHECK_EQ(VoteThreshold(4, 1.0), 4u);
    CHECK_EQ(VoteThreshold(10, 5.0), 10u);
    CHECK_EQ(VoteThreshold(10, -1.0), 1u);
}

TEST_CASE("VotePassed compares yes ballots against the threshold")
{
    CHECK_FALSE(VotePassed(6, 10, 0.6));
    CHECK(VotePassed(7, 10, 0.6));
    // One ballot carries a vote nobody else answered, but zero out of zero is not agreement.
    CHECK(VotePassed(1, 1, 0.6));
    CHECK_FALSE(VotePassed(0, 0, 0.6));
}

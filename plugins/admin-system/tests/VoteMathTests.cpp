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
}

TEST_CASE("VoteThreshold never exceeds the ballots cast")
{
    // Otherwise a unanimous vote could sit at a threshold nobody can reach.
    CHECK_EQ(VoteThreshold(4, 1.0), 4u);
    CHECK_EQ(VoteThreshold(10, 1.0), 10u);
}

TEST_CASE("VoteThreshold is zero when nothing was cast")
{
    CHECK_EQ(VoteThreshold(0, 0.6), 0u);
}

TEST_CASE("VoteThreshold clamps a mis-set ratio")
{
    // A negative or above-one ratio must not make the vote free or impossible.
    CHECK_EQ(VoteThreshold(10, -1.0), 1u);
    CHECK_EQ(VoteThreshold(10, 5.0), 10u);
}

TEST_CASE("VotePassed compares yes ballots against the threshold")
{
    CHECK_FALSE(VotePassed(6, 10, 0.6));
    CHECK(VotePassed(7, 10, 0.6));
    CHECK(VotePassed(8, 10, 0.6));
}

TEST_CASE("VotePassed never passes when nothing was cast")
{
    // Zero yes out of zero ballots must not read as unanimous agreement.
    CHECK_FALSE(VotePassed(0, 0, 0.6));
}

TEST_CASE("A single yes ballot carries a vote nobody else answered")
{
    CHECK(VotePassed(1, 1, 0.6));
}

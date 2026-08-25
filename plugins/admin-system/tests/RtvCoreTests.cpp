#include "Maps/RtvCore.hpp"

#include <doctest/doctest.h>

using AdminSystem::Maps::RtvPassed;
using AdminSystem::Maps::RtvThreshold;

TEST_CASE("RtvThreshold needs a strict majority of the configured share")
{
    // 60% of 10 is 6, and one more makes it a majority of that share rather than a tie.
    CHECK_EQ(RtvThreshold(10, 0.6), 7u);
    CHECK_EQ(RtvThreshold(5, 0.6), 4u);
    CHECK_EQ(RtvThreshold(1, 0.6), 1u);
}

TEST_CASE("RtvThreshold never exceeds the players present")
{
    // Otherwise a full server could sit at a threshold nobody can reach.
    CHECK_EQ(RtvThreshold(4, 1.0), 4u);
    CHECK_EQ(RtvThreshold(10, 1.0), 10u);
}

TEST_CASE("RtvThreshold is zero on an empty server")
{
    CHECK_EQ(RtvThreshold(0, 0.6), 0u);
}

TEST_CASE("RtvThreshold clamps a mis-set ratio")
{
    // A negative or above-one ratio must not make the vote free or impossible.
    CHECK_EQ(RtvThreshold(10, -1.0), 1u);
    CHECK_EQ(RtvThreshold(10, 5.0), 10u);
}

TEST_CASE("RtvPassed compares votes against the threshold")
{
    CHECK_FALSE(RtvPassed(6, 10, 0.6));
    CHECK(RtvPassed(7, 10, 0.6));
    CHECK(RtvPassed(8, 10, 0.6));
}

TEST_CASE("RtvPassed never passes on an empty server")
{
    // Zero votes out of zero players must not read as unanimous agreement.
    CHECK_FALSE(RtvPassed(0, 0, 0.6));
}

TEST_CASE("A lone player can rock the vote")
{
    CHECK(RtvPassed(1, 1, 0.6));
}

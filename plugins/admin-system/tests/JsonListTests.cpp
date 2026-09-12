#include "Database/JsonList.hpp"

#include <doctest/doctest.h>

using AdminSystem::Database::ReadJsonList;

TEST_CASE("ReadJsonList parses a JSON array of strings")
{
    auto values = ReadJsonList(R"(["a","b"])");
    REQUIRE(values.has_value());
    REQUIRE_EQ(values->size(), 2);
    CHECK_EQ((*values)[0], "a");
    CHECK_EQ((*values)[1], "b");
}

TEST_CASE("ReadJsonList returns an empty list for an empty array and for empty input")
{
    CHECK(ReadJsonList("[]")->empty());
    CHECK(ReadJsonList("")->empty());
}

TEST_CASE("ReadJsonList returns nullopt for malformed JSON")
{
    CHECK_FALSE(ReadJsonList("not json").has_value());
}

TEST_CASE("ReadJsonList returns nullopt when an element is not a string")
{
    CHECK_FALSE(ReadJsonList(R"([1, "a"])").has_value());
}

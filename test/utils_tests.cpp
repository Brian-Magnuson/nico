#include <string_view>
#include <unordered_set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/shared/sets.h"
#include "nico/shared/utils.h"

TEST_CASE("Utility break message", "[utils]") {
    SECTION("Short sentence") {
        std::string_view message = "This is a short message.";
        auto lines = nico::break_message(message);
        REQUIRE(
            lines == std::vector<std::string_view>{"This is a short message."}
        );
    }

    SECTION("Long sentences") {
        std::string_view message =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do "
            "eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
            "enim ad minim veniam, quis nostrud exercitation ullamco laboris "
            "nisi ut aliquip ex ea commodo consequat.";
        auto lines = nico::break_message(message, 20);
        REQUIRE(
            lines == std::vector<std::string_view>{
                         "Lorem ipsum dolor",
                         "sit amet,",
                         "consectetur",
                         "adipiscing elit, sed",
                         "do eiusmod tempor",
                         "incididunt ut labore",
                         "et dolore magna",
                         "aliqua. Ut enim ad",
                         "minim veniam, quis",
                         "nostrud exercitation",
                         "ullamco laboris nisi",
                         "ut aliquip ex ea",
                         "commodo consequat."
                     }
        );
    }

    SECTION("Long word") {
        std::string_view message =
            "ThisIsAnExtremelyLongWordThatExceedsTheMaxLengthLimit";
        auto lines = nico::break_message(message, 10);
        REQUIRE(
            lines == std::vector<std::string_view>{
                         "ThisIsAnEx",
                         "tremelyLon",
                         "gWordThatE",
                         "xceedsTheM",
                         "axLengthLi",
                         "mit"
                     }
        );
    }

    SECTION("Short and long word") {
        std::string_view message =
            "ShortWord ThisIsAnExtremelyLongWordThatExceedsTheMaxLengthLimit "
            "End";
        auto lines = nico::break_message(message, 15);
        REQUIRE(
            lines == std::vector<std::string_view>{
                         "ShortWord",
                         "ThisIsAnExtreme",
                         "lyLongWordThatE",
                         "xceedsTheMaxLen",
                         "gthLimit End"
                     }
        );
    }

    SECTION("Respect newlines") {
        std::string_view message =
            "This is a line.\nThis is another line that is quite long and "
            "should be broken into multiple lines.\nShort line.";
        auto lines = nico::break_message(message, 25);
        REQUIRE(
            lines == std::vector<std::string_view>{
                         "This is a line.",
                         "This is another line that",
                         "is quite long and should",
                         "be broken into multiple",
                         "lines.",
                         "Short line."
                     }
        );
    }

    SECTION("Max length lower bound") {
        std::string_view message = "The max length cannot be less than 10.";
        auto lines = nico::break_message(message, 3);
        REQUIRE(
            lines == std::vector<std::string_view>{
                         "The max",
                         "length",
                         "cannot be",
                         "less than",
                         "10."
                     }
        );
    }

    SECTION("Empty message") {
        std::string_view message = "";
        auto lines = nico::break_message(message);
        REQUIRE(lines.empty());
    }
}

TEST_CASE("Utility set operations", "[utils]") {
    SECTION("Set equals") {
        std::unordered_set<int> set1 = {1, 2, 3};
        std::unordered_set<int> set2 = {3, 2, 1};
        std::unordered_set<int> set3 = {1, 2, 4};

        REQUIRE(nico::sets::equals(set1, set2));
        REQUIRE(!nico::sets::equals(set1, set3));
    }

    SECTION("Strict subset") {
        std::unordered_set<std::string> setA = {"a", "b"};
        std::unordered_set<std::string> setB = {"a", "b", "c"};
        std::unordered_set<std::string> setC = {"a", "b"};

        REQUIRE(nico::sets::subset(setA, setB));
        REQUIRE(!nico::sets::subset(setB, setA));
        REQUIRE(!nico::sets::subset(setA, setC));
    }

    SECTION("Subset or equals") {
        std::unordered_set<char> setX = {'x', 'y'};
        std::unordered_set<char> setY = {'x', 'y', 'z'};
        std::unordered_set<char> setZ = {'x', 'y'};

        REQUIRE(nico::sets::subseteq(setX, setY));
        REQUIRE(!nico::sets::subseteq(setY, setX));
        REQUIRE(nico::sets::subseteq(setX, setZ));
    }

    SECTION("Set difference") {
        std::unordered_set<int> set1 = {1, 2, 3, 4};
        std::unordered_set<int> set2 = {3, 4, 5};

        auto diff = nico::sets::difference(set1, set2);
        REQUIRE(diff == std::unordered_set<int>({1, 2}));

        auto diff2 = nico::sets::difference(set2, set1);
        REQUIRE(diff2 == std::unordered_set<int>({5}));
    }
}

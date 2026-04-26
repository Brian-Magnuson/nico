#include <string_view>
#include <unordered_set>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "nico/shared/dictionary.h"
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

TEST_CASE("Utility dictionary", "[utils]") {
    using nico::Dictionary;

    SECTION("Default construction") {
        Dictionary<int, std::string> dict;
        REQUIRE(dict.size() == 0);
        REQUIRE(dict.empty());
    }

    SECTION("Construction with initializer list") {
        Dictionary<int, std::string> dict({{1, "one"}, {2, "two"}});
        REQUIRE(dict.size() == 2);
        REQUIRE(dict.contains(1));
        REQUIRE(dict.contains(2));
        REQUIRE(dict[1] == "one");
        REQUIRE(dict[2] == "two");
    }

    SECTION("Insertion and update") {
        Dictionary<int, std::string> dict;
        dict.insert(1, "one");
        REQUIRE(dict.size() == 1);
        REQUIRE(dict[1] == "one");

        dict.insert(1, "uno");
        REQUIRE(dict.size() == 1);
        REQUIRE(dict[1] == "uno");
    }

    SECTION("Insertion using operator[]") {
        Dictionary<int, std::string> dict;
        dict[1] = "one";
        REQUIRE(dict.size() == 1);
        REQUIRE(dict[1] == "one");

        dict[1] = "uno";
        REQUIRE(dict.size() == 1);
        REQUIRE(dict[1] == "uno");

        REQUIRE(dict[2] == ""); // Default value for std::string
        REQUIRE(dict.size() == 2);
    }

    SECTION("Access with operator[] and at()") {
        Dictionary<int, std::string> dict;
        dict.insert(1, "one");
        REQUIRE(dict[1] == "one");
        REQUIRE(dict.at(1).value() == "one");
        REQUIRE_FALSE(dict.at(2).has_value());
    }

    SECTION("Containment checks") {
        Dictionary<int, std::string> dict;
        dict.insert(1, "one");
        REQUIRE(dict.contains(1));
        REQUIRE_FALSE(dict.contains(2));
    }

    SECTION("Index retrieval") {
        Dictionary<int, std::string> dict;
        dict.insert(1, "one");
        REQUIRE(dict.get_index(1) == 0);
        REQUIRE(dict.get_index(2) == -1);
    }

    SECTION("Iteration") {
        Dictionary<int, std::string> dict({{1, "one"}, {2, "two"}});
        auto it = dict.begin();
        REQUIRE(it->first == 1);
        REQUIRE(it->second == "one");
        ++it;
        REQUIRE(it->first == 2);
        REQUIRE(it->second == "two");
    }

    SECTION("Longer iteration") {
        Dictionary<int, std::string> dict(
            {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}, {5, "five"}}
        );
        std::vector<std::pair<int, std::string>> expected =
            {{1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}, {5, "five"}};
        size_t index = 0;
        for (const auto& pair : dict) {
            REQUIRE(pair.first == expected[index].first);
            REQUIRE(pair.second == expected[index].second);
            ++index;
        }
    }

    SECTION("Iteration different order") {
        Dictionary<int, std::string> dict(
            {{4, "four"}, {2, "two"}, {5, "five"}, {1, "one"}, {3, "three"}}
        );
        std::vector<std::pair<int, std::string>> expected =
            {{4, "four"}, {2, "two"}, {5, "five"}, {1, "one"}, {3, "three"}};
        size_t index = 0;
        for (const auto& pair : dict) {
            REQUIRE(pair.first == expected[index].first);
            REQUIRE(pair.second == expected[index].second);
            ++index;
        }
    }

    SECTION("Equality checks") {
        Dictionary<int, std::string> dict1({{1, "one"}, {2, "two"}});
        Dictionary<int, std::string> dict2({{1, "one"}, {2, "two"}});
        Dictionary<int, std::string> dict3({{1, "uno"}, {2, "dos"}});
        REQUIRE(dict1 == dict2);
        REQUIRE_FALSE(dict1 == dict3);
    }

    SECTION("Edge cases") {
        Dictionary<int, std::string> dict;
        REQUIRE(dict.at(1) == std::nullopt);

        dict.insert(1, "one");
        REQUIRE(dict.size() == 1);
        REQUIRE(dict[1] == "one");
    }

    SECTION("Get pair at index") {
        Dictionary<int, std::string> dict(
            {{4, "four"}, {2, "two"}, {5, "five"}, {1, "one"}, {3, "three"}}
        );
        std::vector<std::pair<int, std::string>> vec =
            {{4, "four"}, {2, "two"}, {5, "five"}, {1, "one"}, {3, "three"}};

        for (size_t i = 0; i < vec.size(); ++i) {
            auto pair_opt = dict.get_pair_at(i);
            REQUIRE(pair_opt.has_value());
            REQUIRE(pair_opt.value() == vec[i]);
        }
    }

    SECTION("Get index of key") {
        Dictionary<std::string, int> dict(
            {{"apple", 1},
             {"banana", 2},
             {"cherry", 3},
             {"strawberry", 4},
             {"grape", 5}}
        );
        REQUIRE(dict.get_index("apple") == 0);
        REQUIRE(dict.get_index("banana") == 1);
        REQUIRE(dict.get_index("cherry") == 2);
        REQUIRE(dict.get_index("strawberry") == 3);
        REQUIRE(dict.get_index("grape") == 4);

        REQUIRE(dict.get_index("date") == -1);
    }
}

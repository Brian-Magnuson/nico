#ifndef NICO_SETS_H
#define NICO_SETS_H

#include <unordered_set>

namespace nico {

namespace sets {

/**
 * @brief Checks if two sets are equal.
 * @tparam T Type of the elements in the sets.
 * @param a The first set.
 * @param b The second set.
 * @return True if the sets are equal, false otherwise.
 */
template <typename T>
bool equals(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    return a == b;
}

/**
 * @brief Checks if set a is a strict subset of set b.
 *
 * Note: If the cardinality of set a is greater than or equal to that of set b,
 * this function immediately returns false.
 *
 * @tparam T Type of the elements in the sets.
 * @param a The first set.
 * @param b The second set.
 * @return True if a is a strict subset of b, false otherwise.
 */
template <typename T>
bool subset(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    if (a.size() >= b.size()) {
        return false;
    }
    for (const auto& item : a) {
        if (b.find(item) == b.end()) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Checks if set a is a subset of set b or equal to b.
 * @tparam T Type of the elements in the sets.
 * @param a The first set.
 * @param b The second set.
 * @return True if a is a subset of b or equal to b, false otherwise.
 */
template <typename T>
bool subseteq(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    return equals(a, b) || subset(a, b);
}

/**
 * @brief Generates a set containing elements of a that are not in b.
 * @tparam T Type of the elements in the sets.
 * @param a The first set.
 * @param b The second set.
 * @return A set containing elements of a that are not in b.
 */
template <typename T>
std::unordered_set<T>
difference(const std::unordered_set<T>& a, const std::unordered_set<T>& b) {
    std::unordered_set<T> result;
    for (const auto& item : a) {
        if (b.find(item) == b.end()) {
            result.insert(item);
        }
    }
    return result;
}

} // namespace sets

} // namespace nico

#endif // NICO_SETS_H

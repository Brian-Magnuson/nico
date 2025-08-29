#ifndef NICO_DICTIONARY_H
#define NICO_DICTIONARY_H

#include <algorithm>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

/**
 * @brief A dictionary class that maps keys to values.
 *
 * The dictionary is implemented using an unordered map and a vector of keys.
 * Unlike std::map, this class preserves the order of insertion.
 *
 * @tparam K The key type. Must be hashable.
 * @tparam V The value type.
 * @tparam Hash The hash function for the keys. Defaults to std::hash<K>.
 */
template <typename K, typename V, typename Hash = std::hash<K>>
class Dictionary {
    // A map of keys to their indices in the keys vector.
    std::unordered_map<K, size_t, Hash> map;
    // A list of key-value pairs in order of insertion.
    std::vector<std::pair<K, V>> keys;

public:
    /**
     * @brief Default constructs an empty dictionary.
     */
    Dictionary() = default;

    /**
     * @brief Copy constructs a dictionary.
     */
    Dictionary(const Dictionary& other) = default;

    /**
     * @brief Constructs a dictionary from a list of keys.
     *
     * Enables the use of initializer lists to construct a dictionary.
     *
     * @param initial_keys A vector of key-value pairs to initialize the dictionary.
     */
    Dictionary(std::vector<std::pair<K, V>> initial_keys) {
        for (const auto& pair : initial_keys) {
            insert(pair.first, pair.second);
        }
    }

    /**
     * @brief Insert a key-value pair into the dictionary.
     *
     * If the key does not exist, it is also added to the end of the keys vector.
     * If the key already exists, the value is updated.
     *
     * @param key The key.
     * @param value The value.
     */
    void insert(K key, V value) {
        auto it = map.find(key);
        if (it == map.end()) {
            map[key] = keys.size();
            keys.push_back({key, value});
        }
        else {
            keys.at(it->second).second = value;
        }
    }

    /**
     * @brief Access the value associated with a key.
     *
     * If the key does not exist, it is added to the dictionary with a default value.
     *
     * The value type must have a default constructor, even if this operator is used to set the value.
     *
     * @param key The key.
     * @return A reference to the value.
     */
    V& operator[](K key) {
        auto it = map.find(key);
        if (it == map.end()) {
            map[key] = keys.size();
            keys.push_back({key, V()});
            return keys.back().second;
        }
        else {
            return keys.at(it->second).second;
        }
    }

    /**
     * @brief Access the value associated with a key.
     *
     * @param key The key.
     * @return A const reference to the value.
     */
    const V& operator[](K key) const {
        return keys.at(map.at(key)).second;
    }

    /**
     * @brief Access the value associated with a key.
     *
     * @param key The key.
     * @return The value associated with the key, or std::nullopt if the key is not in the dictionary.
     */
    std::optional<V> at(K key) {
        auto it = map.find(key);
        if (it != map.end()) {
            return keys.at(it->second).second;
        }
        return std::nullopt;
    }

    /**
     * @brief Access the value associated with a key.
     *
     * @param key The key.
     * @return The value associated with the key, or std::nullopt if the key is not in the dictionary.
     */
    std::optional<const V> at(K key) const {
        auto it = map.find(key);
        if (it != map.end()) {
            return keys.at(it->second).second;
        }
        return std::nullopt;
    }

    /**
     * @brief Get the index of a key in the dictionary.
     *
     * If the key is not in the dictionary, returns -1.
     *
     * @param key The key.
     * @return int The index of the key. -1 if the key is not in the dictionary.
     */
    int get_index(K key) {
        auto it = map.find(key);
        if (it == map.end()) {
            return -1;
        }
        else {
            return (int)(it->second);
        }
    }

    /**
     * @brief Get the key-value pair at an index.
     *
     * @param index The index.
     * @return The key-value pair, or std::nullopt if the index is out of bounds.
     */
    std::optional<std::pair<K, V>> get_pair_at(size_t index) {
        if (index < keys.size()) {
            return keys.at(index);
        }
        return std::nullopt;
    }

    /**
     * @brief Check if the dictionary contains a key.
     *
     * @param key The key to check.
     * @return True if the key is in the dictionary. False otherwise.
     */
    bool contains(K key) const {
        return map.find(key) != map.end();
    }

    /**
     * @brief Gets the size of the dictionary.
     *
     * @return size_t The number of keys in the dictionary.
     */
    size_t size() const {
        return map.size();
    }

    /**
     * @brief Check if the dictionary is empty.
     *
     * @return True if the dictionary is empty. False otherwise.
     */
    bool empty() const {
        return map.empty();
    }

    /**
     * @brief Clear the dictionary.
     */
    void clear() {
        map.clear();
        keys.clear();
    }

    /**
     * @brief Get an iterator to the beginning of the dictionary.
     * @return An iterator to the beginning of the dictionary.
     */
    typename std::vector<std::pair<K, V>>::iterator begin() {
        return keys.begin();
    }

    /**
     * @brief Get an iterator to the end of the dictionary.
     * @return An iterator to the end of the dictionary.
     */
    typename std::vector<std::pair<K, V>>::iterator end() {
        return keys.end();
    }

    /**
     * @brief Get a const iterator to the beginning of the dictionary.
     * @return A const iterator to the beginning of the dictionary.
     */
    typename std::vector<std::pair<K, V>>::const_iterator begin() const {
        return keys.begin();
    }

    /**
     * @brief Get a const iterator to the end of the dictionary.
     * @return A const iterator to the end of the dictionary.
     */
    typename std::vector<std::pair<K, V>>::const_iterator end() const {
        return keys.end();
    }

    /**
     * @brief Find an iterator to a key in the dictionary.
     *
     * @param key The key to find.
     * @return An iterator to the key in the dictionary. If the key is not in the dictionary, returns `end()`.
     */
    typename std::vector<std::pair<K, V>>::iterator find(K key) {
        auto iter = map.find(key);
        if (iter == map.end()) {
            return keys.end();
        }
        return keys.begin() + iter->second;
    }

    /**
     * @brief Find a const iterator to a key in the dictionary.
     *
     * @param key The key to find.
     * @return A const iterator to the key in the dictionary. If the key is not in the dictionary, returns `end()`.
     */
    typename std::vector<std::pair<K, V>>::const_iterator find(K key) const {
        auto iter = map.find(key);
        if (iter == map.end()) {
            return keys.end();
        }
        return keys.begin() + iter->second;
    }

    /**
     * @brief Checks if two dictionaries are equal.
     *
     * Two dictionaries are equal if they have the same key-value pairs in the same order.
     *
     * @param other The other dictionary.
     * @return True if the dictionaries are equal. False otherwise.
     */
    bool operator==(const Dictionary& other) const {
        return keys == other.keys;
    }
};

#endif // NICO_DICTIONARY_H

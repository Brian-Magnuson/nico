#ifndef NICO_SYMBOL_TABLE_H
#define NICO_SYMBOL_TABLE_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../parser/type.h"

/**
 * @brief A symbol table used to store identifiers and their types.
 */
class SymbolTable {
    // The table of identifiers and their types.
    std::unordered_map<std::string, std::vector<std::shared_ptr<Type>>> table;
    // The previous symbol table.
    std::unique_ptr<SymbolTable> previous;

public:
    /**
     * @brief Inserts an identifier and its type into the symbol table.
     *
     * If the identifier already exists in the table, the type will be appended to the list of types.
     *
     * @param identifier The identifier to insert.
     * @param type The type of the identifier.
     */
    void insert(const std::string& identifier, const std::shared_ptr<Type>& type);

    /**
     * @brief Retrieves the type of an identifier from the symbol table.
     *
     * If the identifier was inserted multiple times, the last type inserted will be returned.
     *
     * @param identifier The identifier to retrieve.
     * @return The last type inserted for the identifier, or std::nullopt if the identifier was not found.
     */
    std::optional<std::shared_ptr<Type>> get(const std::string& identifier) const;

    /**
     * @brief Increases the scope of the symbol table.
     *
     * `current` will be modified to point to the new symbol table, which will contain a pointer to the previous symbol table.
     *
     * @param current The current symbol table.
     */
    static void increase_scope(std::unique_ptr<SymbolTable>& current);

    /**
     * @brief Decreases the scope of the symbol table.
     *
     * `current` will be modified to point to the previous symbol table.
     *
     * It is the caller's responsibility to ensure that scope is decreased correctly.
     * If there is no previous symbol table, `abort` will be called.
     *
     * @param current The current symbol table.
     *
     * @warning If there is no previous symbol table, `abort` will be called.
     */
    static void decrease_scope(std::unique_ptr<SymbolTable>& current);
};

#endif // NICO_SYMBOL_TABLE_H

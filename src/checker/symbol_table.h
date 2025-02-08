#ifndef NICO_SYMBOL_TABLE_H
#define NICO_SYMBOL_TABLE_H

#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "../lexer/token.h"
#include "../parser/type.h"

/**
 * @brief A symbol table used to store identifiers and their types.
 */
class SymbolTable {
public:
    /**
     * @brief A struct representing an entry in the symbol table.
     *
     * Contains the type of the identifier and whether it is declared with `var` or not.
     */
    struct Entry {
        // Whether the identifier is declared with `var` or not.
        bool is_var;
        // The token representing the identifier.
        const std::shared_ptr<Token> token;
        // The type of the identifier.
        const std::shared_ptr<Type> type;
    };

private:
    // The table of identifiers and their types.
    std::unordered_map<std::string, std::vector<Entry>> table;
    // The previous symbol table.
    std::unique_ptr<SymbolTable> previous;

public:
    /**
     * @brief Inserts an identifier and its type into the symbol table.
     *
     * If the identifier already exists in the table, the type will be appended to the list of types.
     *
     * @param identifier The identifier to insert.
     * @param is_var Whether the identifier is declared with `var` or not.
     * @param token The token representing the identifier.
     * @param type The type of the identifier.
     */
    void insert(
        const std::string& identifier,
        bool is_var,
        const std::shared_ptr<Token>& token,
        const std::shared_ptr<Type>& type
    );

    /**
     * @brief Retrieves the entry of an identifier from the symbol table.
     *
     * If the identifier was inserted multiple times, the last entry inserted will be returned.
     *
     * @param identifier The identifier to retrieve.
     * @return The last entry inserted for the identifier, or std::nullopt if the identifier was not found.
     */
    std::optional<Entry> get(const std::string& identifier) const;

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

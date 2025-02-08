#include "symbol_table.h"

void SymbolTable::insert(
    const std::string& identifier,
    bool is_var,
    const std::shared_ptr<Token>& token,
    const std::shared_ptr<Type>& type
) {
    table[identifier].push_back({is_var, token, type});
}

std::optional<SymbolTable::Entry> SymbolTable::get(const std::string& identifier) const {
    if (table.find(identifier) == table.end()) {
        if (previous) {
            return previous->get(identifier);
        }
        return std::nullopt;
    }
    return table.at(identifier).back();
}

void SymbolTable::increase_scope(std::unique_ptr<SymbolTable>& current) {
    auto new_table = std::make_unique<SymbolTable>();
    new_table->previous = std::move(current);
    current = std::move(new_table);
}

void SymbolTable::decrease_scope(std::unique_ptr<SymbolTable>& current) {
    if (!current->previous) {
        std::cerr << "SymbolTable::decrease_scope: No previous scope to decrease to." << std::endl;
        std::abort();
    }
    current = std::move(current->previous);
}

#ifndef NICO_SYMBOL_TREE_H
#define NICO_SYMBOL_TREE_H

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "../logger/error_code.h"
#include "../parser/ident.h"
#include "../parser/type.h"

/**
 * @brief A symbol tree that represents the structure of the program's symbols.
 *
 * All scopes/declarations are stored in this tree, which is used to resolve identifiers and types.
 *
 * The tree structure enables identifiers with multiple parts to be resolved by searching upward and downward through different scopes.
 */
class SymbolTree {
    // The root scope of the symbol tree, which is the top-level scope that contains all other scopes.
    std::shared_ptr<Node::RootScope> root_scope;
    // The current scope in the symbol tree, which is the scope that is currently being modified or accessed.
    std::shared_ptr<Node::IScope> current_scope;

public:
    SymbolTree() {
        reset();
    }

    /**
     * @brief Resets the symbol tree to its initial state.
     *
     * This function will reset the root scope to a new instance of Node::RootScope and set the current scope to the root scope.
     * It also installs primitive types into the root scope.
     *
     * This function should be called before starting a new type-checking pass or when reinitializing the symbol tree.
     */
    void reset() {
        root_scope = std::make_shared<Node::RootScope>();
        current_scope = std::static_pointer_cast<Node::IScope>(root_scope);

        install_primitive_types();
    }

    /**
     * @brief Installs primitive types into the root scope of the symbol tree.
     */
    void install_primitive_types();

    /**
     * @brief Adds a namespace to the symbol tree, then enters the namespace scope.
     *
     * If the current scope does not allow namespaces, this function does not add the namespace and returns a pair with nullptr and an error.
     *
     * @param name The name of the namespace.
     * @return std::pair<std::shared_ptr<Node::Namespace>, Err> The namespace if added successfully (first), or nullptr and an error (second).
     */
    std::pair<std::shared_ptr<Node::Namespace>, Err> add_namespace(const std::string& name);

    /**
     * @brief Adds a struct definition to the symbol tree, then enters the struct definition scope.
     *
     * If the struct definition already exists, or the current scope does not allow structs, this function does not add the struct and returns a pair with nullptr and an error.
     *
     * @param name The name of the struct.
     * @return std::pair<std::shared_ptr<Node::StructDef>, Err> The struct definition if added successfully (first), or nullptr and an error (second).
     */
    std::pair<std::shared_ptr<Node::StructDef>, Err> add_struct_def(const std::string& name, bool is_class = false);

    /**
     * @brief Adds a new local scope to the symbol tree, then enters the local scope.
     *
     * Currently, this function has no restrictions on where local scopes can be added.
     *
     * @return std::pair<std::shared_ptr<Node::LocalScope>, Err> The local scope if added successfully (first), or nullptr and an error (second).
     */
    std::pair<std::shared_ptr<Node::LocalScope>, Err> add_local_scope();

    /**
     * @brief Exits the current scope and returns to the parent scope.
     *
     * If the current scope is the root scope, this function does nothing and returns std::nullopt.
     *
     * If the current scope is a local scope, its set of declared variables is cleared upon exit.
     *
     * @return std::optional<std::shared_ptr<Node::IScope>> The parent scope if it exists, or std::nullopt if there is no parent scope (i.e., if the current scope is the root scope).
     */
    std::optional<std::shared_ptr<Node::IScope>> exit_scope();

    /**
     * @brief Searches the symbol tree for a node with the matching name.
     *
     * The search algorithm comes in two parts: upward search and downward search.
     *
     * Upward search: Search from the current scope upward until the first part of the Name matches.
     * Downward search: Search from the matched scope downward for the remaining parts of the Name.
     * If downward search fails, resume upward search until the next match is found or the root scope is reached.
     *
     * Note: If the desired node is a FieldEntry, this function does not reveal whether the field entry is currently declared. Consider adding a check in the type checker.
     *
     * @param name The name to search for.
     * @return std::optional<std::shared_ptr<Node>> The node if found, or
     * std::nullopt if not found.
     */
    std::optional<std::shared_ptr<Node>> search_name(const Name& name) const;

    /**
     * @brief Adds a field entry to the symbol tree in the current scope.
     *
     * If the field name already exists in the current scope, this function does not add the field and returns std::nullopt.
     *
     * Because a field entry carries a type object, the field's type must be resolved before being added to the symbol tree.
     *
     * @param field The field to add.
     * @return std::optional<std::shared_ptr<Node::FieldEntry>> The field entry if added successfully, or std::nullopt if the field name already exists in the current scope.
     */
    std::optional<std::shared_ptr<Node::FieldEntry>> add_field_entry(const Field& field);
};

#endif // NICO_SYMBOL_TREE_H

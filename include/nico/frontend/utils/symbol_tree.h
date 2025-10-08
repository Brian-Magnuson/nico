#ifndef NICO_SYMBOL_TREE_H
#define NICO_SYMBOL_TREE_H

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "nico/frontend/utils/nodes.h"
#include "nico/frontend/utils/type_node.h"
#include "nico/shared/dictionary.h"
#include "nico/shared/error_code.h"

/**
 * @brief A symbol tree that represents the structure of the program's symbols.
 *
 * All scopes/declarations are stored in this tree, which is used to resolve
 * identifiers and types.
 *
 * The tree structure enables identifiers with multiple parts to be resolved by
 * searching upward and downward through different scopes.
 */
class SymbolTree {
    // Whether or not the symbol tree has been modified since this flag was
    // cleared or the tree was created/reset.
    bool modified = false;

    /**
     * @brief Installs primitive types into the root scope of the symbol tree.
     */
    void install_primitive_types();

    /**
     * @brief Helper function to search a name, starting from a specific
     * location.
     *
     * The search algorithm comes in two parts: upward search and downward
     * search.
     *
     * Upward search: Search from the current scope upward until the first part
     * of the Name matches. Downward search: Search from the matched scope
     * downward for the remaining parts of the Name. If downward search fails,
     * resume upward search until the next match is found or the root scope is
     * reached.
     *
     * @param name The name to search for.
     * @param scope The scope to start the search from.
     * @return std::optional<std::shared_ptr<Node>> The node if found, or
     * std::nullopt if not found.
     */
    std::optional<std::shared_ptr<Node>> search_name_from_scope(
        const Name& name, std::shared_ptr<Node::IScope> scope
    ) const;

public:
    // The root scope of the symbol tree, which is the top-level scope that
    // contains all other scopes.
    std::shared_ptr<Node::RootScope> root_scope;
    // The current scope in the symbol tree, which is the scope that is
    // currently being modified or accessed.
    std::shared_ptr<Node::IScope> current_scope;
    // A special scope for reserved names. Reserved names cannot be shadowed in
    // any scope. This scope is searched first, regardless of what scope is
    // currently active.
    std::shared_ptr<Node::RootScope> reserved_scope;

    /**
     * @brief Constructs a symbol tree with a root scope and installs primitive
     * types.
     */
    SymbolTree() { reset(); }

    /**
     * @brief Resets the symbol tree to its initial state.
     *
     * This function will reset the root scope to a new instance of
     * Node::RootScope and set the current scope to the root scope. It also
     * installs primitive types into the root scope.
     *
     * This function should be called before starting a new type-checking pass
     * or when reinitializing the symbol tree.
     */
    void reset() {
        root_scope = std::make_shared<Node::RootScope>();
        current_scope = root_scope;
        reserved_scope = std::make_shared<Node::RootScope>();
        modified = false;

        install_primitive_types();
    }

    /**
     * @brief Checks if the symbol tree has been modified since the last
     * reset or since this flag was last cleared.
     *
     * @return True if the symbol tree has been modified, false otherwise.
     */
    bool was_modified() const { return modified; }

    /**
     * @brief Clears the modified flag, indicating that the symbol tree is
     * considered unmodified.
     */
    void clear_modified() { modified = false; }

    /**
     * @brief Enters the namespace with the name contained in token, adding it
     * if it does not exist.
     *
     * If the current scope does not allow namespaces, this function does not
     * add the namespace and returns a pair with nullptr and an error.
     *
     * If the namespace already exists, the namespace will not be added, and the
     * existing namespace will be entered. The existing namespace will be
     * returned.
     *
     * If the name already exists in the current scope and does not correspond
     * to a namespace, this function does not add the namespace and returns a
     * pair with the conflicting node and an error.
     *
     * @param name The name of the namespace.
     * @return std::pair<std::shared_ptr<Node>, Err> The result of adding the
     * namespace (see description).
     */
    std::pair<std::shared_ptr<Node>, Err>
    add_namespace(std::shared_ptr<Token> token);

    /**
     * @brief Adds a struct definition to the symbol tree, then enters the
     * struct definition scope.
     *
     * If the struct definition already exists, or the current scope does not
     * allow structs, this function does not add the struct and returns a pair
     * with the existing node and an error.
     *
     * @param name The name of the struct.
     * @return std::pair<std::shared_ptr<Node>, Err> The struct definition if
     * added successfully (first), or the existing node and an error (second).
     */
    std::pair<std::shared_ptr<Node>, Err>
    add_struct_def(std::shared_ptr<Token> token, bool is_class = false);

    /**
     * @brief Adds a new function scope to the symbol tree, then enters the
     * function scope.
     *
     * Function scopes may not be added if the current scope is a local scope.
     * This may change in the future if support for closures is added.
     *
     * @param token The token representing the function; should be the
     * function's name.
     * @return std::pair<std::shared_ptr<Node::FunctionScope>, Err> The
     * function scope if added successfully (first), or nullptr and an error.
     */
    std::pair<std::shared_ptr<Node::FunctionScope>, Err>
    add_function_scope(std::shared_ptr<Token> token);

    /**
     * @brief Adds a new local scope to the symbol tree, then enters the local
     * scope.
     *
     * Currently, this function has no restrictions on where local scopes can be
     * added.
     *
     * @return std::pair<std::shared_ptr<Node::LocalScope>, Err> The local scope
     * if added successfully (first), or nullptr and an error (second).
     */
    std::pair<std::shared_ptr<Node::LocalScope>, Err> add_local_scope();

    /**
     * @brief Exits the current scope and returns to the parent scope.
     *
     * If the current scope is the root scope, this function does nothing and
     * returns std::nullopt.
     *
     * If the current scope is a local scope, its set of declared variables is
     * cleared upon exit.
     *
     * @return std::optional<std::shared_ptr<Node::IScope>> The parent scope if
     * it exists, or std::nullopt if there is no parent scope (i.e., if the
     * current scope is the root scope).
     */
    std::optional<std::shared_ptr<Node::IScope>> exit_scope();

    /**
     * @brief Searches the symbol tree for a node with the matching name.
     *
     * First, the search is performed starting from the reserved scope.
     * Then, if the node wasn't found, the search is performed starting from the
     * current scope.
     *
     * Note: If the desired node is a FieldEntry, this function does not reveal
     * whether the field entry is currently declared. Consider adding a check in
     * the type checker.
     *
     * @param name The name to search for.
     * @return std::optional<std::shared_ptr<Node>> The node if found, or
     * std::nullopt if not found.
     */
    std::optional<std::shared_ptr<Node>> search_name(const Name& name) const;

    /**
     * @brief Adds a field entry to the symbol tree in the current scope.
     *
     * If the field name already exists in the current scope, this function does
     * not add the field and returns the existing node and an error.
     *
     * Because a field entry carries a type object, the field's type must be
     * resolved before being added to the symbol tree.
     *
     * @param field The field to add.
     * @return std::pair<std::shared_ptr<Node>, Err> The field entry if added
     * successfully, or the existing node and an error (second).
     */
    std::pair<std::shared_ptr<Node>, Err> add_field_entry(const Field& field);
};

#endif // NICO_SYMBOL_TREE_H

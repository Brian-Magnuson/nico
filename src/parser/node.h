#ifndef NICO_NODE_H
#define NICO_NODE_H

#include <memory>
#include <string>

#include "dictionary.h"
#include "type.h"

class Node {
public:
    class Scope;
    class GlobalScope;
    class Namespace;
    class StructDef;
    class FieldEntry;

    std::weak_ptr<Node> parent;
    std::string unique_name;

    virtual ~Node() = default;
};

class Node::Scope : public Node {
public:
    Dictionary<std::string, Node> children;
};

class Node::GlobalScope : public Node::Scope {};

class Node::Namespace : public Node::GlobalScope {};

class Node::StructDef : public Node::GlobalScope {
public:
    bool is_class = false;
    Dictionary<std::string, Field> properties;
    Dictionary<std::string, std::shared_ptr<Type::Function>> methods;
};

class Node::FieldEntry {
public:
    Field field;
};

#endif // NICO_NODE_H

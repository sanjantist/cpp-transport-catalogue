#pragma once
#include <optional>

#include "json.h"

namespace json {

class DictItemContext;
class ArrayItemContext;
class KeyItemContext;
class Builder;

class BaseContext {
   public:
    explicit BaseContext(Builder& builder) : builder_(builder) {}

   protected:
    Builder& builder_;
};

class Builder {
   public:
    Builder() = default;

    KeyItemContext Key(std::string key);
    Builder& Value(Node::Value value);
    DictItemContext StartDict();
    Builder& EndDict();
    ArrayItemContext StartArray();
    Builder& EndArray();
    Node Build();

   private:
    friend class DictItemContext;
    friend class ArrayItemContext;
    friend class KeyItemContext;

    enum class ContextType { Dict, Array };

    struct Context {
        ContextType type;
        Node* node;
        Node* last_inserted_node = nullptr;
        bool expecting_key = true;
        std::string key = "";
    };

    void CheckNotBuilt() const;

    void AddValueToCurrentContext(Node value);

    Node* GetLastNode();

    Node root_;
    std::vector<Context> stack_;
    bool is_built = false;
};

class KeyItemContext : public BaseContext {
   public:
    explicit KeyItemContext(Builder& builder);

    DictItemContext Value(Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();
};

class DictItemContext : public BaseContext {
   public:
    explicit DictItemContext(Builder& builder);

    KeyItemContext Key(std::string key);

    Builder& EndDict();
};

class ArrayItemContext : public BaseContext {
   public:
    explicit ArrayItemContext(Builder& builder);

    ArrayItemContext Value(Node::Value value);

    DictItemContext StartDict();

    ArrayItemContext StartArray();

    Builder& EndArray();
};

}  // namespace json
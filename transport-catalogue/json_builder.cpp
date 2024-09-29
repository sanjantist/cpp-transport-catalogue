#include "json_builder.h"

#include <cmath>
#include <stdexcept>

#include "json.h"

namespace json {

KeyItemContext Builder::Key(std::string key) {
    CheckNotBuilt();
    if (stack_.empty() || stack_.back().type != ContextType::Dict) {
        throw std::logic_error("Key() is called outside if a dict");
    }
    Context& ctx = stack_.back();
    if (!ctx.expecting_key) {
        throw std::logic_error("Key() is called without corresponding Value()");
    }
    ctx.expecting_key = false;
    ctx.key = std::move(key);
    return KeyItemContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    CheckNotBuilt();
    if (stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Root value has already been set");
        }
        root_ = Node(std::move(value));
    } else {
        Context& ctx = stack_.back();
        if (ctx.type == ContextType::Array) {
            Array& arr = const_cast<Array&>(ctx.node->AsArray());
            arr.emplace_back(std::move(value));
        } else if (ctx.type == ContextType::Dict) {
            if (ctx.expecting_key) {
                throw std::logic_error(
                    "Value() is called without corresponding Key()");
            }
            Dict& dict = const_cast<Dict&>(ctx.node->AsMap());
            dict.emplace(std::move(ctx.key), std::move(value));
            ctx.expecting_key = true;
        } else {
            throw std::logic_error("Invalid context for Value()");
        }
    }
    return *this;
}

DictItemContext Builder::StartDict() {
    CheckNotBuilt();
    Node dict{Dict()};
    if (stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Root value has already been set");
        }
        root_ = std::move(dict);
        stack_.push_back({ContextType::Dict, &root_});
    } else {
        if (stack_.back().expecting_key) {
            throw std::logic_error(
                "StartDict() is called without corresponding Key()");
        }
        AddValueToCurrentContext(std::move(dict));
        Node* node_ptr = GetLastNode();
        stack_.push_back({ContextType::Dict, node_ptr});
    }
    return DictItemContext(*this);
}

Builder& Builder::EndDict() {
    CheckNotBuilt();
    if (stack_.empty() || stack_.back().type != ContextType::Dict) {
        throw std::logic_error(
            "EndDict() is called without corresponding StartDict()");
    }
    stack_.pop_back();
    return *this;
}

ArrayItemContext Builder::StartArray() {
    CheckNotBuilt();
    Node array{Array()};
    if (stack_.empty()) {
        if (!root_.IsNull()) {
            throw std::logic_error("Root value has already been set");
        }
        root_ = std::move(array);
        stack_.push_back({ContextType::Array, &root_});
    } else {
        if (stack_.back().expecting_key) {
            throw std::logic_error(
                "StartArray() is called without corresponding Key()");
        }
        AddValueToCurrentContext(std::move(array));
        Node* node_ptr = GetLastNode();
        stack_.push_back({ContextType::Array, node_ptr});
    }
    stack_.back().expecting_key = false;
    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    CheckNotBuilt();
    if (stack_.empty() || stack_.back().type != ContextType::Array) {
        throw std::logic_error(
            "EndArray() is called without corresponding StartArray()");
    }
    stack_.pop_back();
    return *this;
}

Node Builder::Build() {
    if (!stack_.empty()) {
        throw std::logic_error("Unmatched start/end");
    }
    if (root_.IsNull()) {
        throw std::logic_error("Root value is not set");
    }

    is_built = true;
    return root_;
}

void Builder::CheckNotBuilt() const {
    if (is_built) {
        throw std::logic_error("JSON is already built");
    }
}

void Builder::AddValueToCurrentContext(Node node) {
    Context& ctx = stack_.back();
    if (ctx.type == ContextType::Array) {
        Array& arr = const_cast<Array&>(ctx.node->AsArray());
        arr.emplace_back(std::move(node));
        ctx.last_inserted_node = &arr.back();
    } else if (ctx.type == ContextType::Dict) {
        Dict& dict = const_cast<Dict&>(ctx.node->AsMap());
        auto [it, _] = dict.emplace(std::move(ctx.key), std::move(node));
        ctx.expecting_key = true;
        ctx.last_inserted_node = &(it->second);
    } else {
        throw std::logic_error("Invalid context for adding value");
    }
}

Node* Builder::GetLastNode() {
    if (stack_.empty()) {
        return &root_;
    }
    Context& ctx = stack_.back();
    if (ctx.last_inserted_node) {
        return ctx.last_inserted_node;
    }
    throw std::logic_error("No node has been inserted in the current context");
}

KeyItemContext::KeyItemContext(Builder& builder) : BaseContext(builder) {}

DictItemContext KeyItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return DictItemContext(builder_);
}

DictItemContext KeyItemContext::StartDict() { return builder_.StartDict(); }

ArrayItemContext KeyItemContext::StartArray() { return builder_.StartArray(); }

DictItemContext::DictItemContext(Builder& builder) : BaseContext(builder) {}

KeyItemContext DictItemContext::Key(std::string key) {
    return builder_.Key(std::move(key));
}

Builder& DictItemContext::EndDict() { return builder_.EndDict(); }

ArrayItemContext::ArrayItemContext(Builder& builder) : BaseContext(builder) {}

ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.Value(std::move(value));
    return *this;
}

DictItemContext ArrayItemContext::StartDict() { return builder_.StartDict(); }

ArrayItemContext ArrayItemContext::StartArray() {
    return builder_.StartArray();
}

Builder& ArrayItemContext::EndArray() { return builder_.EndArray(); }

}  // namespace json
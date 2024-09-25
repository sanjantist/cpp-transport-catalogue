#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using Value =
    std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

class ParsingError : public std::runtime_error {
   public:
    using runtime_error::runtime_error;
};

class Node {
   public:
    const Value GetValue() const;

    Node() = default;
    template <typename T>
    Node(T value) : value_(std::move(value)) {}

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    [[nodiscard]] bool operator==(const Node& other) const;
    [[nodiscard]] bool operator!=(const Node& other) const;

   private:
    Value value_;
};

class Document {
   public:
    explicit Document(Node root);

    const Node& GetRoot() const;

   private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

void PrintNode(const Node& node, const PrintContext& ctx);

bool operator==(const Document& lhs, const Document& rhs);

}  // namespace json
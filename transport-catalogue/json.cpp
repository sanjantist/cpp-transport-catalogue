#include "json.h"

#include <cctype>
#include <cstddef>
#include <ostream>
#include <stdexcept>
#include <string>
#include <variant>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;
    char c;

    while (input >> c) {
        if (c == ']') {
            return Node(std::move(result));
        }

        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }

    throw ParsingError("Array is not closed");
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s +
                                       escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }

    return Node(std::move(s));
}

Node LoadDict(istream& input) {
    Dict result;

    for (char c; input >> c;) {
        if (c == '}') {
            return Node(std::move(result));
        }
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({std::move(key), LoadNode(input)});
    }

    throw ParsingError("Map is not closed"s);
}

Node LoadBool(istream& input) {
    string bool_str;

    while (isalpha(input.peek())) {
        bool_str += input.get();
    }

    if (bool_str == "true"s) {
        return Node(true);
    } else if (bool_str == "false"s) {
        return Node(false);
    } else {
        throw ParsingError("Unexpected symbols"s);
    }
}

Node LoadNull(istream& input) {
    string null_str;

    while (isalpha(input.peek())) {
        null_str += input.get();
    }

    if (null_str == "null"s) {
        return {};
    } else {
        throw ParsingError("Unexpected symbols"s);
    }
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace

const Value Node::GetValue() const { return value_; }

bool Node::IsInt() const { return holds_alternative<int>(value_); }

bool Node::IsDouble() const { return IsPureDouble() || IsInt(); }

bool Node::IsPureDouble() const { return holds_alternative<double>(value_); }

bool Node::IsBool() const { return holds_alternative<bool>(value_); }

bool Node::IsString() const { return holds_alternative<string>(value_); }

bool Node::IsNull() const { return holds_alternative<nullptr_t>(value_); }

bool Node::IsArray() const { return holds_alternative<Array>(value_); }

bool Node::IsMap() const { return holds_alternative<Dict>(value_); }

int Node::AsInt() const {
    if (!IsInt()) {
        throw logic_error("invalid type"s);
    }

    return get<int>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw logic_error("invalid type"s);
    }

    return get<bool>(value_);
}

double Node::AsDouble() const {
    if (!IsDouble()) {
        throw logic_error("invalid type"s);
    }

    if (IsPureDouble()) {
        return get<double>(value_);
    }

    return static_cast<double>(get<int>(value_));
}

const std::string& Node::AsString() const {
    if (!IsString()) {
        throw logic_error("invalid type"s);
    }

    return get<string>(value_);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw logic_error("invalid type"s);
    }

    return get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw logic_error("invalid type"s);
    }

    return get<Dict>(value_);
}

[[nodiscard]] bool Node::operator==(const Node& other) const {
    return GetValue() == other.GetValue();
}

[[nodiscard]] bool Node::operator!=(const Node& other) const {
    return !(*this == other);
}

Document::Document(Node root) : root_(std::move(root)) {}

const Node& Document::GetRoot() const { return root_; }

Document Load(istream& input) { return Document{LoadNode(input)}; }

template <typename Value>
void PrintValue(const Value& value, const PrintContext& ctx) {
    ctx.out << value;
}

void PrintValue(bool value, const PrintContext& ctx) {
    ctx.out << (value ? "true"sv : "false"sv);
}

void PrintValue(nullptr_t, const PrintContext& ctx) { ctx.out << "null"sv; }

void PrintValue(const string& str, const PrintContext& ctx) {
    ctx.out << "\"";
    for (const char c : str) {
        switch (c) {
            case '"':
                ctx.out << "\\\"";
                break;
            case '\\':
                ctx.out << "\\\\";
                break;
            case '\r':
                ctx.out << "\\r";
                break;
            case '\n':
                ctx.out << "\\n";
                break;
            case '\t':
                ctx.out << "\\t";
                break;
            default:
                ctx.out << c;
                break;
        }
    }
    ctx.out << "\"";
}

void PrintValue(const Array& arr, const PrintContext& ctx) {
    ctx.out << "[\n";

    bool is_first = true;
    PrintContext indented_ctx = ctx.Indented();

    for (const auto& node : arr) {
        if (!is_first) {
            ctx.out << ",\n";
        }
        indented_ctx.PrintIndent();
        PrintNode(node, indented_ctx);
        is_first = false;
    }

    ctx.out << "\n";
    ctx.PrintIndent();
    ctx.out << "]";
}

void PrintValue(const Dict& dict, const PrintContext& ctx) {
    ctx.out << "{\n";

    bool is_first = true;
    PrintContext indented_ctx = ctx.Indented();

    for (const auto& [key, node] : dict) {
        if (!is_first) {
            indented_ctx.out << ",\n";
        }
        indented_ctx.PrintIndent();
        indented_ctx.out << "\""s << key << "\": "s;
        PrintNode(node, indented_ctx);
        is_first = false;
    }

    ctx.out << "\n";
    ctx.PrintIndent();
    ctx.out << "}";
}

void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit([&ctx](const auto& value) { PrintValue(value, ctx); },
               node.GetValue());
}

void Print(const Document& doc, ostream& output) {
    auto node = doc.GetRoot(); 
    PrintNode(node, {output});
}

bool operator==(const Document& lhs, const Document& rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}

}  // namespace json
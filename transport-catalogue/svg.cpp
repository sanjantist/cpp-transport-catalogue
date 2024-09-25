#include "svg.h"

namespace svg {

std::ostream &operator<<(std::ostream &out, Color color) {
    return std::visit(ColorRenderer{out}, color);
}

std::ostream &operator<<(std::ostream &out, StrokeLineCap slc) {
    switch (slc) {
        case svg::StrokeLineCap::BUTT:
            out << "butt";
            break;
        case svg::StrokeLineCap::ROUND:
            out << "round";
            break;
        case svg::StrokeLineCap::SQUARE:
            out << "square";
            break;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, StrokeLineJoin slj) {
    switch (slj) {
        case svg::StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case svg::StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case svg::StrokeLineJoin::MITER:
            out << "miter";
            break;
        case svg::StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case svg::StrokeLineJoin::ROUND:
            out << "round";
            break;
    }
    return out;
}

void Object::Render(const RenderContext &context) const {
    auto new_context = context.Indented();
    new_context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(new_context);

    new_context.out << std::endl;
}

// ---------- Circle ------------------

Circle &Circle::SetCenter(Point center) {
    center_ = center;
    return *this;
}

Circle &Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext &context) const {
    context.out << "<circle cx=\"" << center_.x << "\" cy=\"" << center_.y
                << "\" r=\"" << radius_ << "\"";
    RenderAttrs(context.out);
    context.out << "/>";
}

// ---------- Polyline ------------------

Polyline &Polyline::AddPoint(Point point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const {
    {
        context.out << "<polyline points=\"";

        bool is_first = true;
        for (const auto &point : points_) {
            if (is_first) {
                context.out << point.x << "," << point.y;
                is_first = false;
            } else {
                context.out << " " << point.x << "," << point.y;
            }
        }

        context.out << "\"";
        RenderAttrs(context.out);
        context.out << "/>";
    }
}

// ---------- Text ------------------

Text &Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

Text &Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text &Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text &Text::SetFontFamily(std::string font_family) {
    font_family_ = std::move(font_family);
    return *this;
}

Text &Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::move(font_weight);
    return *this;
}

Text &Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext &context) const {
    context.out << "<text";
    RenderAttrs(context.out);

    context.out << " x=\"" << position_.x << "\" y=\"" << position_.y << "\" "
                << "dx=\"" << offset_.x << "\" dy=\"" << offset_.y << "\" "
                << "font-size=\"" << size_ << "\"";

    if (font_family_) {
        context.out << " font-family=\"" << *font_family_ << "\"";
    }
    if (font_weight_) {
        context.out << " font-weight=\"" << *font_weight_ << "\"";
    }

    context.out << ">";

    for (const char c : data_) {
        switch (c) {
            case '"':
                context.out << "&quot;";
                break;
            case '\'':
                context.out << "&apos;";
                break;
            case '<':
                context.out << "&lt;";
                break;
            case '>':
                context.out << "&gt;";
                break;
            case '&':
                context.out << "&amp;";
                break;
            default:
                context.out << c;
                break;
        }
    }

    context.out << "</text>";
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object> &&obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream &out) const {
    out << R"(<?xml version="1.0" encoding="UTF-8" ?>)" << "\n";
    out << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.1">)"
        << "\n";
    for (const auto &object : objects_) {
        object->Render({out});
    }
    out << R"(</svg>)";
}
}  // namespace svg
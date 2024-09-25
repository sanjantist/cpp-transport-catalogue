#pragma once

#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <variant>

namespace svg {

struct Rgb {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

struct ColorRenderer {
    std::ostream &operator()(std::monostate) {
        out << "none";
        return out;
    }

    std::ostream &operator()(std::string color) {
        out << color;
        return out;
    }

    std::ostream &operator()(Rgb rgb) {
        out << "rgb(" << +rgb.red << "," << +rgb.green << "," << +rgb.blue
            << ")";
        return out;
    }

    std::ostream &operator()(Rgba rgba) {
        out << "rgba(" << +rgba.red << "," << +rgba.green << "," << +rgba.blue
            << "," << rgba.opacity << ")";
        return out;
    }

    std::ostream &out;
};

std::ostream &operator<<(std::ostream &out, Color color);

inline const Color NoneColor{"none"};

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream &operator<<(std::ostream &out, StrokeLineCap slc);

std::ostream &operator<<(std::ostream &out, StrokeLineJoin slj);

template <typename Owner>
class PathProps {
   public:
    Owner &SetFillColor(Color color) {
        fill_color_ = color;
        return AsOwner();
    }

    Owner &SetStrokeColor(Color color) {
        stroke_color_ = color;
        return AsOwner();
    }

    Owner &SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }

    Owner &SetStrokeLineCap(StrokeLineCap line_cap) {
        line_cap_ = line_cap;
        return AsOwner();
    }

    Owner &SetStrokeLineJoin(StrokeLineJoin line_join) {
        line_join_ = line_join;
        return AsOwner();
    }

   protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream &out) const {
        using namespace std::literals;

        if (!std::holds_alternative<std::monostate>(fill_color_)) {
            out << " fill=\""sv;
            std::visit(ColorRenderer{out}, fill_color_) << "\""sv;
        }
        if (!std::holds_alternative<std::monostate>(stroke_color_)) {
            out << " stroke=\""sv;
            std::visit(ColorRenderer{out}, stroke_color_) << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (line_cap_) {
            out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
        }
        if (line_join_) {
            out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
        }
    }

   private:
    Owner &AsOwner() { return static_cast<Owner &>(*this); }

    Color fill_color_;
    Color stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> line_cap_;
    std::optional<StrokeLineJoin> line_join_;
};

struct Point {
    Point() = default;
    Point(double x, double y) : x(x), y(y) {}
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с
 * отступами. Хранит ссылку на поток вывода, текущее значение и шаг отступа при
 * выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream &out) : out(out) {}

    RenderContext(std::ostream &out, int indent_step, int indent = 0)
        : out(out), indent_step(indent_step), indent(indent) {}

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream &out;
    int indent_step = 2;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
   public:
    virtual void Render(const RenderContext &context) const;

    virtual ~Object() = default;

   private:
    virtual void RenderObject(const RenderContext &context) const = 0;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
   public:
    Circle() = default;

    Circle &SetCenter(Point center);
    Circle &SetRadius(double radius);

   private:
    void RenderObject(const RenderContext &context) const override;

    Point center_ = {0.0, 0.0};
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
   public:
    Polyline() = default;

    Polyline &AddPoint(Point point);

   private:
    void RenderObject(const RenderContext &context) const override;

    std::list<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
   public:
    Text() = default;

    Text &SetPosition(Point pos);

    Text &SetOffset(Point offset);

    Text &SetFontSize(uint32_t size);

    Text &SetFontFamily(std::string font_family);

    Text &SetFontWeight(std::string font_weight);

    Text &SetData(std::string data);

   private:
    void RenderObject(const RenderContext &context) const override;

    Point position_ = {0.0, 0.0};
    Point offset_ = {0.0, 0.0};
    uint32_t size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_;
};

class ObjectContainer {
   public:
    template <typename T>
    void Add(T object) {
        objects_.push_back(std::make_unique<T>(std::move(object)));
    }
    virtual void AddPtr(std::unique_ptr<Object> &&obj) = 0;

   protected:
    std::list<std::unique_ptr<Object>> objects_;
};

class Drawable {
   public:
    virtual void Draw(ObjectContainer &container) const = 0;
    virtual ~Drawable() = default;
};

class Document : public ObjectContainer {
   public:
    void AddPtr(std::unique_ptr<Object> &&obj) override;
    void Render(std::ostream &out) const;
};

}  // namespace svg
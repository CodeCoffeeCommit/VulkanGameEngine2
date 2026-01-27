// LibreUI/include/LibreUI/Types.h
// Core types for LibreUI - no external dependencies except glm

#pragma once

#include <glm/glm.hpp>

namespace LibreUI {

    // ============================================================================
    // BASIC TYPES
    // ============================================================================

    struct Vec2 {
        float x = 0, y = 0;

        Vec2() = default;
        Vec2(float x, float y) : x(x), y(y) {}

        Vec2 operator+(const Vec2& o) const { return { x + o.x, y + o.y }; }
        Vec2 operator-(const Vec2& o) const { return { x - o.x, y - o.y }; }
        Vec2 operator*(float s) const { return { x * s, y * s }; }
    };

    struct Rect {
        float x = 0, y = 0, w = 0, h = 0;

        Rect() = default;
        Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}

        float right() const { return x + w; }
        float bottom() const { return y + h; }
        Vec2 center() const { return { x + w * 0.5f, y + h * 0.5f }; }

        bool contains(float px, float py) const {
            return px >= x && px < right() && py >= y && py < bottom();
        }

        Rect shrink(float amount) const {
            return { x + amount, y + amount, w - amount * 2, h - amount * 2 };
        }
    };

    struct Color {
        float r = 1, g = 1, b = 1, a = 1;

        Color() = default;
        Color(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}

        Color lighter(float amount = 0.1f) const {
            return { r + amount, g + amount, b + amount, a };
        }
        Color darker(float amount = 0.1f) const {
            return { r - amount, g - amount, b - amount, a };
        }

        glm::vec4 toVec4() const { return { r, g, b, a }; }
    };

    struct Padding {
        float top = 0, right = 0, bottom = 0, left = 0;

        Padding() = default;
        Padding(float all) : top(all), right(all), bottom(all), left(all) {}
        Padding(float vertical, float horizontal)
            : top(vertical), right(horizontal), bottom(vertical), left(horizontal) {
        }
        Padding(float t, float r, float b, float l)
            : top(t), right(r), bottom(b), left(l) {
        }
    };

} // namespace LibreUI